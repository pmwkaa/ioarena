
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

#include <ioarena.h>
#include <ness.h>

struct iaprivate {
	void *env;
	void *db;
};

static int ia_nessdb_open(const char *datadir)
{
	char dbname[] = "test.nessdb";
	iadriver *drv = ioarena.driver;

	drv->priv = calloc(1, sizeof(iaprivate));
	if (drv->priv == NULL)
		return -1;

	iaprivate *self = drv->priv;
	self->env = ness_env_open(datadir, -1);
	self->db = ness_db_open(self->env, dbname);
	if (!self->db) {
		fprintf(stderr, "open db error, see ness.event for details\n");
		goto bailout;
	}

	if (!ness_env_set_cache_size(self->env, 1024*1024*1024)) {
		fprintf(stderr, "set cache size error, see ness.event for details\n");
		goto bailout;
	}

	return 0;

bailout:
	ia_log("error: %s", __func__);
	return -1;
}


static int ia_nessdb_close(void)
{
	iaprivate *self = ioarena.driver->priv;
	if (self) {
		ioarena.driver->priv = NULL;
		ness_db_close(self->db);
		ness_env_close(self->env);
		free(self);
	}
	return 0;
}

static iacontext* ia_nessdb_thread_new(void)
{
	iacontext* ctx = calloc(1, sizeof(ctx));
	return ctx;
}

void ia_nessdb_thread_dispose(iacontext *ctx)
{
	free(ctx);
}

static int ia_nessdb_begin(iacontext *ctx, iabenchmark step)
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

static int ia_nessdb_done(iacontext* ctx, iabenchmark step)
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

static int ia_nessdb_next(iacontext* ctx, iabenchmark step, iakv *kv)
{
	int rc;

	(void) ctx;
	iaprivate *self = ioarena.driver->priv;

	switch(step) {
	case IA_SET:
		rc = ness_db_set(self->db, kv->k, kv->ksize, kv->v, kv->vsize);
		if (rc != 1)
			goto bailout;
		rc = 0;
		break;
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

bailout:
	ia_log("error: %s, %s, rc= (%d)", __func__,
		ia_benchmarkof(step), rc);
	return -1;
}

iadriver ia_nessdb =
{
	.name  = "nessdb",
	.priv  = NULL,
	.open  = ia_nessdb_open,
	.close = ia_nessdb_close,

	.thread_new = ia_nessdb_thread_new,
	.thread_dispose = ia_nessdb_thread_dispose,
	.begin	= ia_nessdb_begin,
	.next	= ia_nessdb_next,
	.done	= ia_nessdb_done
};
