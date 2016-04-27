#ifndef IA_HISTOGRAM_H_
#define IA_HISTOGRAM_H_

/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

typedef struct iahistogram iahistogram;

#define ST_HISTOGRAM_COUNT 167

struct iastat {
	uintmax_t latency_sum_ns, latency_sum_square;
	uintmax_t n, volume_sum;
};

struct iahistogram {
	ia_timestamp_t checkpoint_ns, begin_ns, end_ns;
	iabenchmark bench;
	char enabled;
	ia_timestamp_t whole_min, whole_max;
	struct iastat last;
	ia_timestamp_t min, max;
	struct iastat acc;
	uintmax_t buckets[ST_HISTOGRAM_COUNT];
	int merge_evo;
};

void ia_histogram_init(iahistogram *h);
void ia_histogram_reset(iahistogram *h, iabenchmark bench);
void ia_histogram_destroy(iahistogram *h);
void ia_histogram_add(iahistogram *h, ia_timestamp_t t0, size_t volume);
void ia_histogram_merge(iahistogram *src);

int ia_histogram_checkpoint(ia_timestamp_t now);
void ia_histogram_print(const iaconfig *config);
void ia_histogram_csvopen(const iaconfig *config);
void ia_histogram_csvclose(void);
void ia_histogram_enable(iabenchmark bench);

void ia_histogram_rusage(const iaconfig *config, const iarusage *start, const iarusage *fihish);

#endif /* IA_HISTOGRAM_H_ */
