
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

#include <ioarena.h>

struct iaprivate {
	int dummy;
};

struct iacontext {
	int dummy;
};

static int ia_dummy_open(const char *datadir)
{
	iadriver *drv = ioarena.driver;
	(void) drv;
	(void) datadir;
	return 0;
}

static int ia_dummy_close(void)
{
	iadriver *drv = ioarena.driver;
	(void) drv;
	return 0;
}

static iacontext* ia_dummy_thread_new()
{
	iacontext* ctx = calloc(1, sizeof(iacontext));
	return ctx;
}

void ia_dummy_thread_dispose(iacontext *ctx)
{
	free(ctx);
}

static int ia_dummy_begin(iacontext *ctx, iabenchmark step)
{
	int rc;

	(void) ctx;

	switch(step) {
	case IA_SET:
	case IA_BATCH:
	case IA_CRUD:
	case IA_DELETE:
		rc = 0;
		break;

	case IA_ITERATE:
	case IA_GET:
		rc = 0;
		break;

	default:
		assert(0);
		rc = -1;
	}

	return rc;
}

static int ia_dummy_done(iacontext* ctx, iabenchmark step)
{
	int rc;

	(void) ctx;

	switch(step) {
	case IA_SET:
	case IA_BATCH:
	case IA_CRUD:
	case IA_DELETE:
	case IA_ITERATE:
	case IA_GET:
		rc = 0;
		break;

	default:
		assert(0);
		rc = -1;
	}

	return rc;
}

static int ia_dummy_next(iacontext* ctx, iabenchmark step, iakv *kv)
{
	int rc;

	(void) ctx;
	(void) kv;

	switch(step) {
	case IA_SET:
	case IA_DELETE:
	case IA_ITERATE:
	case IA_GET:
		rc = 0;
		break;
	default:
		assert(0);
		rc = -1;
	}

	return rc;
}

iadriver ia_dummy =
{
	.name  = "dummy",
	.priv  = NULL,
	.open  = ia_dummy_open,
	.close = ia_dummy_close,

	.thread_new = ia_dummy_thread_new,
	.thread_dispose = ia_dummy_thread_dispose,
	.begin	= ia_dummy_begin,
	.next	= ia_dummy_next,
	.done	= ia_dummy_done
};
