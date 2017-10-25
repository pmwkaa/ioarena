
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

#include <ioarena.h>

void ia_fatal(const char *msg)
{
	fprintf(stderr, "\n*** IOARENA: fatal %s\n", msg);
	fflush(NULL);
	abort();
}

int ia_init(ia *a, int argc, char **argv)
{
	int rc;

	ia_log("IOARENA (embedded storage benchmarking)\n");

	rc = ia_configinit(&a->conf);
	if (rc == -1)
		return -1;
	rc = ia_configparse(&a->conf, argc, argv);
	if (rc == -1 || rc == 1)
		return rc;
	ia_configprint(&a->conf);
	a->driver = a->conf.driver_if;

	mkdir(ioarena.conf.path, 0755);
	snprintf(a->datadir, sizeof(a->datadir), "%s/%s", ioarena.conf.path, a->driver->name);
	mkdir(a->datadir, 0755);

	iarusage before_open;
	rc = ia_get_rusage(&before_open, a->datadir);
	if (rc)
		return -1;

	a->before_open_ram = before_open.ram;
	rc = a->driver->open(a->datadir);
	if (rc == -1)
		return -1;
	return 0;
}

void ia_free(ia *a)
{
	if (a->driver)
		a->driver->close();
	ia_configfree(&a->conf);
}

static void ia_sync_start(ia *a)
{
#ifdef _POSIX_PRIORITY_SCHEDULING
	sched_yield();
#elif defined(__APPLE__) || defined(__MACH__)
	pthread_yield_np();
#else
	pthread_yield();
#endif
	int rc = pthread_barrier_wait(&a->barrier_start);
	if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
		ia_log("error: pthread_barrier_wait %s (%d)", strerror(rc), rc);
		ia_fatal(__func__);
	}
}

static void ia_sync_fihish(ia *a)
{
	int rc = pthread_barrier_wait(&a->barrier_fihish);
	if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
		ia_log("error: pthread_barrier_wait %s (%d)", strerror(rc), rc);
		ia_fatal(__func__);
	}
}

static void* ia_doer_thread(void *arg)
{
	iadoer *doer = (iadoer *) arg;
	ia_sync_start(&ioarena);
	int rc = ia_doer_fulfil(doer);
	if (rc)
		ioarena.failed = rc;
	ia_doer_destroy(doer);
	ia_sync_fihish(&ioarena);
	free(doer);
	return NULL;
}

static int ia_spread(int count, int *nth, long *rotator, long set, int *key_space)
{
	int n;

	for(n = 0; n < count; n++) {
		assert(set != 0);

		iadoer* doer = calloc(1, sizeof(iadoer));
		if (!doer)
			return -1;

		if (*rotator == 0)
			*rotator = set;

		long mask = *rotator;
		if (ioarena.conf.separate) {
			iabenchmark order = IA_SET;
			for(mask = 0; ! mask; order = (order + 1) % IA_MAX)
				mask = *rotator & (1l << order);
		}

		assert(mask != 0);
		if (mask & bench_mask_write) {
			*key_space += 1;
			if (mask & bench_mask_2keyspace)
				*key_space += 1;
		}

		*nth += 1;
		if (ia_doer_init(doer, *nth, mask, *key_space, *nth))
			return -1;

		pthread_t thread;
		int rc = pthread_create(&thread, NULL, ia_doer_thread, doer);
		if (rc)
			return rc;

		*rotator &= ~mask;
	}
	return 0;
}

int ia_run(ia *a)
{
	long set_rd = 0;
	long set_wr = 0;
	iabenchmark bench;
	for (bench = IA_SET; bench < IA_MAX; bench++) {
		if (! a->conf.benchmark_list[bench])
			continue;

		ia_histogram_enable(bench);
		if (bench == IA_ITERATE || bench == IA_GET || bench == IA_MIX_70_30 || bench == IA_MIX_50_50 || bench == IA_MIX_30_70)
			set_rd |= 1l << bench;
		else
			set_wr |= 1l << bench;
	}

	if ((set_rd | set_wr) == 0)
		return 0;
	if (a->conf.rthr && set_rd == 0)
		a->conf.rthr = 0;
	if (a->conf.wthr && set_wr == 0)
		a->conf.wthr = 0;

	int key_nsectors = 1;
	if (key_nsectors < a->conf.rthr)
		key_nsectors = a->conf.rthr;
	if (key_nsectors < a->conf.wthr)
		key_nsectors = a->conf.wthr;

	int key_nspaces = 1;
	if (key_nspaces < a->conf.wthr)
		key_nspaces = a->conf.wthr;
	if (set_wr & bench_mask_2keyspace)
		key_nspaces	+= key_nspaces;

	int rc = ia_kvgen_setup(!ioarena.conf.binary, ioarena.conf.ksize,
		key_nspaces, key_nsectors, ioarena.conf.keys, ioarena.conf.kvseed);
	if (rc) {
		ia_log("error: key-value generator setup failed, the options are correct?");
		return rc;
	}

	rc = pthread_barrier_init(&a->barrier_start, NULL,
		a->conf.rthr + a->conf.wthr + 1);
	if (!rc)
		rc = pthread_barrier_init(&a->barrier_fihish, NULL,
			a->conf.rthr + a->conf.wthr + 1);
	if (rc) {
		ia_log("error: pthread_barrier_init %s (%d)", strerror(errno), errno);
		return rc;
	}

	ia_histogram_csvopen(&a->conf);

	int nth = 0;
	int key_space = 0;
	rc = ia_spread(a->conf.rthr, &nth, &set_rd, set_rd, &key_space);
	if (rc)
		goto bailout;

	rc = ia_spread(a->conf.wthr, &nth, &set_wr, set_wr, &key_space);
	if (rc)
		goto bailout;

	iarusage rusage_start, rusage_fihish;
	if (set_wr | set_rd) {
		iadoer here;
		if (ia_doer_init(&here, 0, set_wr | set_rd, 0, 0))
			goto bailout;

		rc = ia_get_rusage(&rusage_start, a->datadir);
		if (rc)
			goto bailout;

		ia_sync_start(a);
		rc = ia_doer_fulfil(&here);
		ia_sync_fihish(a);

		if (rc)
			goto bailout;

		rc = ia_get_rusage(&rusage_fihish, a->datadir);
		if (rc)
			goto bailout;

		ia_doer_destroy(&here);
	} else {
		rc = ia_get_rusage(&rusage_start, a->datadir);
		if (rc)
			goto bailout;

		ia_sync_start(a);
		ia_sync_fihish(a);

		rc = ia_get_rusage(&rusage_fihish, a->datadir);
		if (rc)
			goto bailout;
	}

	if (a->failed)
		goto bailout;

	ia_histogram_checkpoint(0);
	ia_log("complete.");
	ia_histogram_print(&a->conf);

	rusage_start.ram = a->before_open_ram;
	rusage_start.disk = 0;
	ia_histogram_rusage(&a->conf, &rusage_start, &rusage_fihish);
	ia_histogram_csvclose();

	return 0;

bailout:
	exit(EXIT_FAILURE);
}
