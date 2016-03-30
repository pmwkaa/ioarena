
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

#include <ioarena.h>

const long bench_mask_read = 0
	| 1ull << IA_BATCH
	| 1ull << IA_CRUD
	| 1ull << IA_ITERATE
	| 1ull << IA_GET;

const long bench_mask_write = 0
	| 1ull << IA_SET
	| 1ull << IA_BATCH
	| 1ull << IA_CRUD
	| 1ull << IA_DELETE;

static void ia_keynotfound(iadoer *doer, const char *op, iakv *k)
{
	ia_log("error: key %s not found (%s, #%d, %d+%d)",
		   k->k, op, doer->nth, doer->key_space, doer->key_sequence);
}

static int ia_quadruple(iadoer *doer, iakv *a, iakv *b)
{
	int rc = ioarena.driver->next(doer->ctx, IA_SET, b);
	if (!rc)
		rc = ioarena.driver->next(doer->ctx, IA_SET, a);
	if (!rc) {
		rc = ioarena.driver->next(doer->ctx, IA_DELETE, b);
		if (rc == ENOENT) {
			ia_keynotfound(doer, "crud.del", b);
			if (ioarena.conf.ignore_keynotfound)
				rc = 0;
		}
	}
	if (!rc) {
		rc = ioarena.driver->next(doer->ctx, IA_GET, a);
		if (rc == ENOENT) {
			ia_keynotfound(doer, "crud.get", a);
			if (ioarena.conf.ignore_keynotfound)
				rc = 0;
		}
	}
	return rc;
}

static int ia_run_benchmark(iadoer *doer, iabenchmark bench)
{
	int rc = 0, rc2;
	uintmax_t i;

	//const char *name = ia_benchmarkof(bench);
	//ia_log("<< %s.%s-%d", ioarena.conf.driver, name, doer->nth);

	ia_histogram_reset(&doer->hg, bench);
	ia_kvgen_rewind(doer->gen);

	for (i = 0; rc == 0 && i < ioarena.conf.count; ) {
		ia_timestamp_t t0;
		iakv a, b;
		int j;

		switch(bench) {
		case IA_SET:
		case IA_DELETE:
		case IA_GET:
			if (ia_kvgen_getcouple(doer->gen, &a, NULL, (bench != IA_SET)))
				goto bailout;

			t0 = ia_timestamp_ns();
			rc = ioarena.driver->begin(doer->ctx, bench);
			if (!rc)
				rc = ioarena.driver->next(doer->ctx, bench, &a);
			rc2 = ioarena.driver->done(doer->ctx, bench);
			ia_histogram_add(&doer->hg, t0,
				bench == IA_DELETE ? a.ksize : a.ksize + a.vsize);
			if (rc == ENOENT) {
				ia_keynotfound(doer, ia_benchmarkof(bench), &a);
				if (ioarena.conf.ignore_keynotfound)
					rc = 0;
			}
			if (! rc)
				rc = rc2;
			if (rc)
				goto bailout;
			++i;
			break;

		case IA_CRUD:
			if (ia_kvgen_getcouple(doer->gen, &a, &b, 0))
				goto bailout;
			t0 = ia_timestamp_ns();
			rc = ioarena.driver->begin(doer->ctx, IA_CRUD);
			if (!rc)
				rc = ia_quadruple(doer, &a, &b);
			if (!rc)
				rc = ioarena.driver->done(doer->ctx, IA_CRUD);
			ia_histogram_add(&doer->hg, t0,
				a.ksize + a.vsize +
				b.ksize + b.vsize +
				a.ksize +
				b.ksize + b.vsize );
			if (rc)
				goto bailout;
			++i;
			break;

		case IA_BATCH:
			t0 = ia_timestamp_ns();
			rc = ioarena.driver->begin(doer->ctx, IA_BATCH);
			for(j = 0; j < ioarena.conf.batch_length; ++j) {
				if (ia_kvgen_getcouple(doer->gen, &a, &b, 0))
					goto bailout;
				rc = ia_quadruple(doer, &a, &b);
				if (rc || ++i == ioarena.conf.count)
					break;
			}
			if (!rc)
				rc = ioarena.driver->done(doer->ctx, IA_BATCH);
			ia_histogram_add(&doer->hg, t0,
				(a.ksize + a.vsize +
				b.ksize + b.vsize +
				a.ksize +
				b.ksize + b.vsize) * ioarena.conf.batch_length);
			if (rc)
				goto bailout;
			break;

		case IA_ITERATE:
			t0 = ia_timestamp_ns();
			rc = ioarena.driver->begin(doer->ctx, IA_ITERATE);
			while(rc == 0) {
				a.k = a.v = NULL;
				a.ksize = a.vsize = 0;
				rc = ioarena.driver->next(doer->ctx, IA_ITERATE, &a);
				ia_histogram_add(&doer->hg, t0, a.ksize + a.vsize);
				if (++i == ioarena.conf.count)
					break;
				t0 = ia_timestamp_ns();
			}
			if (rc == ENOENT)
				rc = 0;
			break;

		default:
			assert(0);
			rc = -1;
		}
	}

bailout:
	ia_histogram_merge(&doer->hg);
	//ia_log(">> %s.%s-%d", ioarena.conf.driver, name, doer->nth);
	return rc;
}

int ia_doer_fulfil(iadoer *doer)
{
	if (doer->ctx == NULL) {
		doer->ctx = ioarena.driver->thread_new();
		if (doer->ctx == NULL)
			return -1;
	}

	int count = 0, rc = 0;
	for (count = 0; count < ioarena.conf.nrepeat
			|| (ioarena.conf.continuous_completing
				&& ioarena.doers_done < ioarena.doers_count); ) {
		iabenchmark bench;
		for (bench = IA_SET; ! rc && bench < IA_MAX; bench++) {
			if (doer->benchmask & (1l << bench))
				rc = ia_run_benchmark(doer, bench);
		}

		if (++count == ioarena.conf.nrepeat)
			__sync_fetch_and_add(&ioarena.doers_done, 1);

		if (rc || ioarena.failed)
			break;
	}

	if (doer->ctx) {
		ioarena.driver->thread_dispose(doer->ctx);
		doer->ctx = NULL;
	}

	return rc;
}

int ia_doer_init(iadoer *doer, int nth, long benchmask, int key_space, int key_sequence)
{
	assert(benchmask);
	doer->ctx = NULL;
	doer->nth = nth;
	doer->benchmask = benchmask;
	doer->key_space = key_space;
	doer->key_sequence = key_sequence;

	if (benchmask) {
		char line[1024], *s;
		iabenchmark bench;
		s = line;
		for(bench = IA_SET; bench < IA_MAX; ++bench)
			if (benchmask & (1l << bench))
				s += snprintf(s, line + sizeof(line) - s, "%s%s",
				s != line ? ", " : "",
				ia_benchmarkof(bench));

		ia_log("doer.%d: {%s}, key-space %d, key-sequence %d",
			doer->nth, line, key_space, key_sequence);
	}

	if (ia_kvgen_init(&doer->gen, !ioarena.conf.binary, ioarena.conf.ksize, ioarena.conf.vsize,
			doer->key_space, doer->key_sequence, ioarena.conf.count)) {
		ia_log("doer.%d: key-value generator failed, the options are correct?", doer->nth);
		return -1;
	}

	ia_histogram_init(&doer->hg);
	__sync_fetch_and_add(&ioarena.doers_count, 1);
	return 0;
}

void ia_doer_destroy(iadoer *doer)
{
	__sync_fetch_and_add(&ioarena.doers_count, -1);
	ia_histogram_destroy(&doer->hg);
	ia_kvgen_destory(&doer->gen);
}
