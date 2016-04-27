/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) Dmitry Simonenko
 * Copyright (c) Leonid Yuriev
 * BSD License
*/

#include <ioarena.h>

#define INTERVAL_STAT	S
#define INTERVAL_MERGE	(S/100)

static const uintmax_t
ia_histogram_buckets[ST_HISTOGRAM_COUNT] =
{
	#define LINE_12_100(M) \
		M*12, M*14, M*16, M*18, M*20, M*25, M*30, M*35, \
		M*40, M*45, M*50, M*60, M*70, M*80, M*90, M*100

	9,
	LINE_12_100(1),  LINE_12_100(10),    LINE_12_100(100),
	LINE_12_100(US), LINE_12_100(US*10), LINE_12_100(US*100),
	LINE_12_100(MS), LINE_12_100(MS*10), LINE_12_100(MS*100),
	LINE_12_100(S),  S * 5 * 60,         S * 30 * 60,
	S * 3600 * 4,    S * 3600 * 8,       S * 3600 * 24,
	~0ull

	#undef LINE_12_100
};

struct global {
	pthread_mutex_t mutex;
	ia_timestamp_t starting_point;
	ia_timestamp_t checkpoint_ns;
	long enable_mask;
	iahistogram per_bench[IA_MAX];
	volatile int doers_active, doers_merged;
	int merge_evo;
	FILE *csv_timeline;
};

static struct global global;

void ia_global_init(void)
{
	global.starting_point = ia_timestamp_ns();
	global.checkpoint_ns = global.starting_point;
	pthread_mutex_init(&global.mutex, NULL);
}

void
ia_histogram_reset(iahistogram *h, iabenchmark bench)
{
	int merge_evo = h->merge_evo;
	memset(h, 0, sizeof(*h));
	h->min           = ~0ull;
	h->whole_min     = ~0ull;
	h->checkpoint_ns = ia_timestamp_ns();
	h->bench         = bench;
	h->enabled		 = 42;
	h->merge_evo     = merge_evo;
}

void
ia_histogram_init(iahistogram *h)
{
	memset(h, 0, sizeof(*h));
	h->merge_evo = global.merge_evo;
	__sync_fetch_and_add(&global.doers_active, 1);
}

void ia_histogram_destroy(iahistogram *h)
{
	if (h->enabled == 42) {
		h->enabled = 0;
		if (h->merge_evo == global.merge_evo + 1) {
			if (pthread_mutex_lock(&global.mutex))
				ia_fatal(__FUNCTION__);
			if (h->merge_evo == global.merge_evo + 1)
				global.doers_merged -= 1;
			__sync_fetch_and_add(&global.doers_active, -1);
			pthread_mutex_unlock(&global.mutex);
		} else
			__sync_fetch_and_add(&global.doers_active, -1);
	}
}

static FILE* csv_create(const iaconfig *config, const char* item)
{
	FILE* f = NULL;

	if(config->csv_prefix) {
		char path[PATH_MAX];
		snprintf(path, sizeof(path),
				 "%s%s_%s_%s.csv", config->csv_prefix, config->driver, ia_syncmode2str(config->syncmode), item);
		f = fopen(path, "w");
		if (! f)
			ia_log("error: %s, %s (%d)", path, strerror(errno), errno);
	}

	return f;
}

void ia_histogram_csvopen(const iaconfig *config)
{
	if (config->csv_prefix && !global.csv_timeline)
		global.csv_timeline = csv_create(config, "timeline");
}

void
ia_histogram_csvclose(void)
{
	if (global.csv_timeline) {
		fclose(global.csv_timeline);
		global.csv_timeline = NULL;
	}
}

void ia_histogram_enable(iabenchmark bench)
{
	if (! global.per_bench[bench].enabled)
		ia_histogram_reset(&global.per_bench[bench], bench);
}

static int snpf_val(char *buf, size_t len, double val, const char *unit)
{
	const char scale[] = "yzafpnum KMGTPEZY";
	const char *suffix = scale + 8;
	assert(*suffix == ' ');

	while (val > 995 && *suffix) {
		val *= 1e-3;
		++suffix;
	}
	while (val < 1 && suffix > scale) {
		val *= 1e3;
		--suffix;
	}

	if (*suffix == ' ')
		return snprintf(buf, len, " %7.3f%s ", val, unit);
	return snprintf(buf, len, " %7.3f%c%s", val, *suffix, unit);
}

static int snpf_lat(char *buf, size_t len, ia_timestamp_t ns)
{
#if 1
	return snpf_val(buf, len, ns / (double) S, "s");
#else
	const char* suffix = "us";
	double scale = 1.0 / US;

	if (ns >= S || ns % MS == 0) {
		suffix = " s";
		scale = 1.0 / S;
	} else if (ns >= MS || ns % US == 0) {
		suffix = "ms";
		scale = 1.0 / MS;
	}

	return snprintf(buf, len, " %7.3f%s", ns * scale, suffix);
#endif
}

int ia_histogram_checkpoint(ia_timestamp_t now)
{
	if (now) {
		if (now - global.checkpoint_ns < INTERVAL_STAT)
			return -1;
		if (global.doers_active > ++global.doers_merged)
			return 0;
	}

	if (global.doers_active != global.doers_merged)
		ia_fatal(__FUNCTION__);

	if (! now)
		now = ia_timestamp_ns();

	iahistogram *h;
	char line[1024], *s;

	if (global.checkpoint_ns == global.starting_point) {
		s = line;

		s += snprintf(s, line + sizeof(line) - s, "     time");
		if (global.csv_timeline)
			fprintf(global.csv_timeline, "\ttime" );

		for (h = global.per_bench; h < global.per_bench + IA_MAX; ++h) {
			if (! h->enabled)
				continue;

			const char *name = ia_benchmarkof(h->bench);
			s += snprintf(s, line + sizeof(line) - s,
				" | bench      rps      min       avg       rms       max       vol           #N");
			if (global.csv_timeline)
				fprintf(global.csv_timeline, ",\t%s_rps,\t%s_min,\t%s_avg,\t%s_rms,\t%s_max", name, name, name, name, name);
		}

		if (global.csv_timeline)
			fprintf(global.csv_timeline, "\n");
		ia_log("%s", line);
	}

	s = line;
	const double timepoint = (now - global.starting_point) / (double) S;
	s += snprintf(s, line + sizeof(line) - s, "%9.3f", timepoint);
	if (global.csv_timeline)
		fprintf(global.csv_timeline, "%e", timepoint);

	const ia_timestamp_t wall_ns = now - global.checkpoint_ns;
	const double wall = wall_ns / (double) S;
	global.checkpoint_ns = now;

	for (h = global.per_bench; h < global.per_bench + IA_MAX; ++h) {
		if (! h->enabled)
			continue;

		const char *name = ia_benchmarkof(h->bench);
		const uintmax_t n = h->acc.n - h->last.n;
		const uintmax_t vol = h->acc.volume_sum - h->last.volume_sum;

		s += snprintf(s, line + sizeof(line) - s, " | %5s", name );
		if (n) {
			const ia_timestamp_t rms = sqrt((h->acc.latency_sum_square - h->last.latency_sum_square) / n);
			const ia_timestamp_t avg = (h->acc.latency_sum_ns - h->last.latency_sum_ns) / n;
			const double rps = n / wall;
			const double bps = vol / wall;

			if (global.csv_timeline) {
				fprintf(global.csv_timeline, ",\t%e,\t%e,\t%e,\t%e,\t%e",
					rps, h->min / (double) S, avg / (double) S, rms / (double) S, h->max / (double) S);
			}

			s += snprintf(s, line + sizeof(line) - s, ":");
			s += snpf_val(s, line + sizeof(line) - s, rps, "");
			s += snpf_lat(s, line + sizeof(line) - s, h->min);
			s += snpf_lat(s, line + sizeof(line) - s, avg);
			s += snpf_lat(s, line + sizeof(line) - s, rms);
			s += snpf_lat(s, line + sizeof(line) - s, h->max);

			s += snpf_val(s, line + sizeof(line) - s, bps, "bps");
			s += snpf_val(s, line + sizeof(line) - s, h->acc.n, "");
		}
		else {
			s += snprintf(s, line + sizeof(line) - s,
				"        -        -         -         -         -         -           - ");
			if (global.csv_timeline) {
				fprintf(global.csv_timeline, ",\t\t,\t\t,\t\t,\t\t");
			}
		}

		if (h->whole_min > h->min)
			h->whole_min = h->min;
		h->min = ~0ull;

		if (h->whole_max < h->max)
			h->whole_max = h->max;
		h->max = 0;

		h->last = h->acc;
	}

	if (global.csv_timeline) {
		fprintf(global.csv_timeline, "\n");
		fflush(global.csv_timeline);
	}
	ia_log("%s", line);

	assert(global.doers_active == global.doers_merged);
	global.doers_merged = 0;
	global.merge_evo += 1;
	return 1;
}

static void ia_histogram_merge_locked(iahistogram *src, ia_timestamp_t now)
{
	iahistogram *dst = &global.per_bench[src->bench];
	if (dst->enabled && src->acc.n != src->last.n) {
		dst->acc.latency_sum_ns += src->acc.latency_sum_ns - src->last.latency_sum_ns;
		dst->acc.latency_sum_square += src->acc.latency_sum_square - src->last.latency_sum_square;
		dst->acc.volume_sum += src->acc.volume_sum - src->last.volume_sum;
		dst->acc.n += src->acc.n - src->last.n;

		int i;
		for (i = 0; i < ST_HISTOGRAM_COUNT; i++)
			dst->buckets[i] += src->buckets[i];

		if (! dst->begin_ns || dst->begin_ns > src->begin_ns)
			dst->begin_ns = src->begin_ns;
		if (dst->end_ns < src->end_ns)
			dst->end_ns = src->end_ns;
		if (dst->min > src->min)
			dst->min = src->min;
		if (dst->max < src->max)
			dst->max = src->max;

		if (src->merge_evo == global.merge_evo && ia_histogram_checkpoint(now) >= 0)
			src->merge_evo += 1;
	}
}

void ia_histogram_merge(iahistogram *src)
{
	if (src->acc.n - src->last.n) {
		ia_timestamp_t now = ia_timestamp_ns();
		if (pthread_mutex_lock(&global.mutex))
			ia_fatal(__FUNCTION__);

		ia_histogram_merge_locked(src, now);
		pthread_mutex_unlock(&global.mutex);
	}
}

void
ia_histogram_add(iahistogram *h, ia_timestamp_t t0, size_t volume)
{
	uintmax_t now = ia_timestamp_ns();
	ia_timestamp_t latency = now - t0;

	if (! h->begin_ns)
		h->begin_ns = t0;
	h->end_ns = now;
	h->acc.latency_sum_ns += latency;
	h->acc.latency_sum_square += latency * latency;
	h->acc.n++;
	h->acc.volume_sum += volume;
	if (h->min > latency)
		h->min = latency;
	if (h->max < latency)
		h->max = latency;

	size_t begin = 0;
	size_t end = ST_HISTOGRAM_COUNT - 1;
	size_t mid;
	while (1) {
		mid = begin / 2 + end / 2;
		if (mid == begin) {
			for (mid = end; mid > begin; mid--) {
				if (ia_histogram_buckets[mid - 1] < latency)
					break;
			}
			break;
		}
		if (ia_histogram_buckets[mid - 1] < latency)
			begin = mid;
		else
			end = mid;
	}
	h->buckets[mid]++;

	if (now - h->checkpoint_ns >= INTERVAL_MERGE
			&& h->merge_evo == global.merge_evo
			&& pthread_mutex_trylock(&global.mutex) == 0) {

		ia_histogram_merge_locked(h, now);
		pthread_mutex_unlock(&global.mutex);

		h->checkpoint_ns = now;
		h->min = ~0ull;
		h->max = 0;
		h->last = h->acc;

		int i;
		for (i = 0; i < ST_HISTOGRAM_COUNT; i++)
			h->buckets[i] = 0;
	}
}

void ia_histogram_print(const iaconfig *config)
{
	iahistogram *h;

	for (h = global.per_bench; h < global.per_bench + IA_MAX; ++h) {
		if (! h->enabled || ! h->acc.n)
			continue;

		const char *name = ia_benchmarkof(h->bench);
		printf("\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> %s(%ju)\n", name, h->acc.n);
		FILE* csv = csv_create(config, name);

		printf("[%8s   %8s   ]%16s%8s%10s\n",
			"ltn_from", "ltn_to", "ops_count", "%", "p%");
		if (csv)
			fprintf(csv, "%s,\t%s,\t%s,\t%s\t%s\n",
				"ltn_open", "ltn_close", "ops_count", "%", "p%");
		printf("----------------------------------------------------------\n");

		int i;
		char line[1024], *s;
		uintmax_t n = 0;

		for (i = 0; i < ST_HISTOGRAM_COUNT; i++) {
			if (! h->buckets[i])
				continue;

			n += h->buckets[i];
			s = line;
			s += snprintf(s, line + sizeof(line) - s, "[");
			s += snpf_lat(s, line + sizeof(line) - s,
				(i > 0) ? ia_histogram_buckets[i - 1] : 0);
			s += snprintf(s, line + sizeof(line) - s, ",");
			s += snpf_lat(s, line + sizeof(line) - s,
				ia_histogram_buckets[i] - 1);
			s += snprintf(s, line + sizeof(line) - s,
						  " ]%16zu%7.2f%%%9.4f%%", h->buckets[i],
						  h->buckets[i] * 1e2 / h->acc.n,
						  n * 1e2 / h->acc.n );

			printf("%s\n", line);

			if(csv)
				fprintf(csv, "%e,\t%e,\t%ju,\t%e,\t%e\n",
						((i > 0) ? ia_histogram_buckets[i - 1] : 0) / (double) S,
						(ia_histogram_buckets[i] - 1) / (double) S,
						h->buckets[i],
						h->buckets[i] * 1e2 / h->acc.n,
						n * 1e2 / h->acc.n
					);
		}
		printf("----------------------------------------------------------\n");

		snpf_lat(line, sizeof(line), h->acc.latency_sum_ns);
		printf("total:%16s  %16zu\n", line, n);
		snpf_lat(line, sizeof(line), h->whole_min);
		printf("min latency:%s/op\n", line);
		const ia_timestamp_t avg = h->acc.latency_sum_ns / h->acc.n;
		snpf_lat(line, sizeof(line), avg);
		printf("avg latency:%s/op\n", line);
		const ia_timestamp_t rms = sqrt(h->acc.latency_sum_square / h->acc.n);
		snpf_lat(line, sizeof(line), rms);
		printf("rms latency:%s/op\n", line);
		snpf_lat(line, sizeof(line), h->whole_max);
		printf("max latency:%s/op\n", line);

		const ia_timestamp_t wall_ns = h->end_ns - h->begin_ns;
		const double wall = wall_ns / (double) S;
		const double rps = h->acc.n / wall;
		snpf_val(line, sizeof(line), rps, "");
		printf(" throughput:%sops/s\n", line);

		if (csv) {
			fprintf(csv, "\n%s,\t%s,\t%s,\t%s,\t%s\n",
				"ltn_min", "ltn_avg", "ltn_rms", "ltn_max", "throughput");
			fprintf(csv, "%e,\t%e\t%e\t%e,\t%e\n",
					h->whole_min / (double) S,
					avg / (double) S,
					rms / (double) S,
					h->whole_max / (double) S,
					rps
				);
			fclose(csv);
		}
	}
}

void ia_histogram_rusage(const iaconfig *config, const iarusage *start, const iarusage *fihish)
{
	printf("\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> rusage\n");
	FILE* csv = csv_create(config, "rusage");

	printf("iops: read %ld, write %ld, page %ld\n",
			fihish->iops_read - start->iops_read,
			fihish->iops_write - start->iops_write,
			fihish->iops_page - start->iops_page
		);

	printf("cpu: user %f, system %f\n",
		   (fihish->cpu_user_ns - start->cpu_user_ns) / (double) S,
		   (fihish->cpu_kernel_ns - start->cpu_kernel_ns) / (double) S
		);

	const double mb = 1ul << 20;
	printf("space: disk %f, ram %f\n",
		   (fihish->disk - start->disk) / mb,
		   (fihish->ram - start->ram) / mb);

	if (csv) {
		fprintf(csv, "%s,\t%s,\t%s,\t%s,\t%s,\t%s,\t%s\n",
				"iops_read",
				"iops_write",
				"iops_page",
				"cpu_user_ns",
				"cpu_kernel_ns",
				"disk",
				"ram"
			);
		fprintf(csv, "%ju,\t%ju,\t%ju,\t%e,\t%e,\t%e,\t%e\n",
				fihish->iops_read - start->iops_read,
				fihish->iops_write - start->iops_write,
				fihish->iops_page - start->iops_page,
				(fihish->cpu_user_ns - start->cpu_user_ns) * 1e-9,
				(fihish->cpu_kernel_ns - start->cpu_kernel_ns) * 1e-9,
				(fihish->disk - start->disk) / mb,
				(fihish->ram - start->ram) / mb
			);
		fclose(csv);
	}
}
