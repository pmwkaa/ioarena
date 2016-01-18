
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) Dmitry Simonenko
 * Copyright (c) Leonid Yuriev
 * BSD License
*/

#include <ioarena.h>

struct iaprivate {
	int debug;
};

struct iacontext {
	int debug;
};

static int ia_debug_open(const char *datadir)
{
	iadriver *drv = ioarena.driver;
	printf("%s.open(%s)\n", drv->name, datadir);
	return 0;
}

static int ia_debug_close(void)
{
	iadriver *drv = ioarena.driver;
	printf("%s.close()\n", drv->name);
	return 0;
}

static iacontext* ia_debug_thread_new()
{
	iadriver *drv = ioarena.driver;
	iacontext* ctx = calloc(1, sizeof(iacontext));
	printf("%s.thread_new() = %p\n", drv->name, ctx);
	return ctx;
}

void ia_debug_thread_dispose(iacontext *ctx)
{
	iadriver *drv = ioarena.driver;
	printf("%s.thread_dispose(%p)\n", drv->name, ctx);
	free(ctx);
}

static int ia_debug_begin(iacontext *ctx, iabenchmark step)
{
	iadriver *drv = ioarena.driver;
	int rc;

	printf("%s.begin(%p, %s)\n", drv->name, ctx, ia_benchmarkof(step));

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

static int ia_debug_done(iacontext* ctx, iabenchmark step)
{
	iadriver *drv = ioarena.driver;
	int rc;

	printf("%s.done(%p, %s)\n", drv->name, ctx, ia_benchmarkof(step));

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

static int ia_debug_next(iacontext* ctx, iabenchmark step, iakv *kv)
{
	iadriver *drv = ioarena.driver;
	int rc = 0;

	switch(step) {
	case IA_SET:
		printf("%s.next(%p, %s, %s -> %s)\n",
			   drv->name, ctx, ia_benchmarkof(step),
			   kv->k, kv->v);
		break;
	case IA_GET:
	case IA_DELETE:
		printf("%s.next(%p, %s, %s)\n",
			   drv->name, ctx, ia_benchmarkof(step),
			   kv->k );
		break;
	case IA_ITERATE:
		printf("%s.next(%p, %s)\n",
			   drv->name, ctx, ia_benchmarkof(step));
		break;
	default:
		assert(0);
		rc = -1;
	}

	return rc;
}

iadriver ia_debug =
{
	.name  = "debug",
	.priv  = NULL,
	.open  = ia_debug_open,
	.close = ia_debug_close,

	.thread_new = ia_debug_thread_new,
	.thread_dispose = ia_debug_thread_dispose,
	.begin	= ia_debug_begin,
	.next	= ia_debug_next,
	.done	= ia_debug_done
};
