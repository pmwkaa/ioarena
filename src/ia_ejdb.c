
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

#include <ioarena.h>
#include <ejdb.h>

struct iaprivate {
	EJDB *jb;
	EJQ *jq;
};

static int ia_ejdb_open(const char *datadir)
{	
	int rc = 0;
	const char *db_name = "/test.ejdb";
  	int db_path_sz = strlen(datadir) + strlen(db_name) + 1;
  	char db_path[db_path_sz];
	memset(db_path, 0, db_path_sz); 
	strcat(db_path, datadir);
	strcat(db_path, db_name);
	printf("db_path %s\n", db_path);

	iadriver *drv = ioarena.driver;

	drv->priv = calloc(1, sizeof(iaprivate));
	if (drv->priv == NULL)
		return -1;

	iaprivate *self = drv->priv;
	(void)self;
	if(rc)

	return 0;
goto bailout;
bailout:
	ia_log("error: %s", __func__);
	return -1;
}


static int ia_ejdb_close(void)
{
	iaprivate *self = ioarena.driver->priv;
	if (self) {
		ioarena.driver->priv = NULL;
		free(self);
	}
	return 0;
}

static iacontext* ia_ejdb_thread_new(void)
{
	iacontext* ctx = calloc(1, sizeof(ctx));
	return ctx;
}

void ia_ejdb_thread_dispose(iacontext *ctx)
{
	free(ctx);
}

static int ia_ejdb_begin(iacontext *ctx, iabenchmark step)
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

static int ia_ejdb_done(iacontext* ctx, iabenchmark step)
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

static int ia_ejdb_next(iacontext* ctx, iabenchmark step, iakv *kv)
{
	int rc = 0;

	(void) ctx;
	(void) kv;
	iaprivate *self = ioarena.driver->priv;
	(void) self;

	switch(step) {
	case IA_SET:
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

iadriver ia_ejdb =
{
	.name  = "ejdb",
	.priv  = NULL,
	.open  = ia_ejdb_open,
	.close = ia_ejdb_close,

	.thread_new = ia_ejdb_thread_new,
	.thread_dispose = ia_ejdb_thread_dispose,
	.begin	= ia_ejdb_begin,
	.next	= ia_ejdb_next,
	.done	= ia_ejdb_done
};
