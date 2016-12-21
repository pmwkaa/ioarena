
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

#include <ioarena.h>
#include <vedis.h>

//#define EJDB_DEBUG 1

struct iaprivate {
	void *db;   /* vedisdb main handle */
};

static int ia_vedisdb_open(const char *datadir)
{	
	//int rc = 0;
	int open_mode = 0;
	const char *db_name = "/test.vedisdb";
  	int db_path_sz = strlen(datadir) + strlen(db_name) + 1;
  	char db_path[db_path_sz];
	memset(db_path, 0, db_path_sz); 
	strcat(db_path, datadir);
	strcat(db_path, db_name);

	iadriver *drv = ioarena.driver;

	drv->priv = calloc(1, sizeof(iaprivate));
	if (drv->priv == NULL)
		goto bailout;

	iaprivate *self = drv->priv;

	
	switch(ioarena.conf.syncmode) {
	case IA_SYNC:
		break;
	case IA_LAZY:
		ia_log("error: %s(): not support syncmode %s", __func__, ia_syncmode2str(ioarena.conf.syncmode));
		goto bailout;
	case IA_NOSYNC:
		break;
	default:
		ia_log("error: %s(): unsupported syncmode %s",
			__func__, ia_syncmode2str(ioarena.conf.syncmode));
		goto bailout;
	}
	switch(ioarena.conf.walmode) {
	case IA_WAL_INDEF:
		break;
	case IA_WAL_ON:
		break;
	case IA_WAL_OFF:
		break;
	default:
		ia_log("error: %s(): unsupported walmode %s",
			__func__, ia_walmode2str(ioarena.conf.walmode));
		goto bailout;
	}
	ia_log("let's open VedisDB with db_path %s\n", db_path);

	return 0;
bailout:
	ia_log("error: %s", __func__);
	return -1;
}


static int ia_vedisdb_close(void)
{
	int rc = 0;
	iaprivate *self = ioarena.driver->priv;
	if (self) {
	 	rc = (0 ? 0 : -1);
		ioarena.driver->priv = NULL;
		free(self);
	}
	return 0;
}

static iacontext* ia_vedisdb_thread_new(void)
{
	iacontext* ctx = calloc(1, sizeof(ctx));
	return ctx;
}

void ia_vedisdb_thread_dispose(iacontext *ctx)
{
	free(ctx);
}

static int ia_vedisdb_begin(iacontext *ctx, iabenchmark step)
{
	(void) ctx;
	int rc;

	iaprivate *self = ioarena.driver->priv;

	switch(step) {
	case IA_BATCH:
	case IA_CRUD:
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

static int ia_vedisdb_done(iacontext* ctx, iabenchmark step)
{
	int rc;

	(void) ctx;

	iaprivate *self = ioarena.driver->priv;

	switch(step) {
	case IA_BATCH:
	case IA_CRUD:
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

static int ia_vedisdb_next(iacontext* ctx, iabenchmark step, iakv *kv)
{
	int rc = 0;
	(void) ctx;
	char key[kv->ksize + 1];
	memcpy(key, kv->k, kv->ksize);
	key[kv->ksize] = 0;
	char value[kv->vsize + 1];
	memcpy(value, kv->v, kv->vsize);
	value[kv->vsize] = 0;
	iaprivate *self = ioarena.driver->priv;
	
	ia_log("KEY = \"%s\", VALUE = \"%s\"\n", key, value);
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

iadriver ia_vedisdb =
{
	.name  = "vedisdb",
	.priv  = NULL,
	.open  = ia_vedisdb_open,
	.close = ia_vedisdb_close,

	.thread_new = ia_vedisdb_thread_new,
	.thread_dispose = ia_vedisdb_thread_dispose,
	.begin	= ia_vedisdb_begin,
	.next	= ia_vedisdb_next,
	.done	= ia_vedisdb_done
};
