
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

#include <ioarena.h>
#include <vedis.h>

struct iaprivate {
	vedis *db;   /* vedisdb main handle */
};

__attribute__((unused))
static void diag_put(vedis *pStore) {
	const char *zBuf;
	int iLen;
	/* Something goes wrong, extract database error log */
	vedis_config(
			pStore,
			VEDIS_CONFIG_ERR_LOG,
			&zBuf,
			&iLen
		    );
	if( iLen > 0 ){
		puts(zBuf);
	}
}

__attribute__((unused))
static int data_consumer_callback(const void *pData,unsigned int nDatalen,void *pUserData /* Unused */) {

	(void)pUserData;
        ssize_t nWr;
        nWr = write(STDOUT_FILENO,pData,nDatalen);
        if( nWr < 0 ){
                /* Abort processing */
                return VEDIS_ABORT;
        }
        /* All done, data was redirected to STDOUT */
        return VEDIS_OK;
}

static int ia_vedisdb_open(const char *datadir)
{	
	int rc = 0;
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
	case IA_LAZY:
	case IA_NOSYNC:
		ia_log("VedisDB: %s(): not support any configurable options for manage syncmode. Current: %s\n", __func__, ia_syncmode2str(ioarena.conf.syncmode));
		break;
	default:
		ia_log("error: %s(): unsupported syncmode %s",
			__func__, ia_syncmode2str(ioarena.conf.syncmode));
		goto bailout;
	}
	switch(ioarena.conf.walmode) {
	case IA_WAL_INDEF:
	case IA_WAL_ON:
	case IA_WAL_OFF:
		ia_log("VedisDB: %s(): not support any configurable options for manage walmode. Current: %s\n", __func__, ia_syncmode2str(ioarena.conf.syncmode));
		break;
	default:
		ia_log("error: %s(): unsupported walmode %s",
			__func__, ia_walmode2str(ioarena.conf.walmode));
		goto bailout;
	}

	rc = vedis_lib_init();
	if(rc)
		goto bailout;
	
	ia_log("vedislib is threadsafe ? %s\n", vedis_lib_is_threadsafe() ? "YES" : "NO");

	ia_log("let's open VedisDB with db_path %s\n", db_path);
	rc = vedis_open(&self->db, db_path);
	
	return rc;
bailout:
	ia_log("error: %s", __func__);
	return -1;
}


static int ia_vedisdb_close(void)
{
	iaprivate *self = ioarena.driver->priv;
	if (self) {
		/* Auto-commit and close the vedis handle */
		vedis_close(self->db);
		vedis_lib_shutdown();
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
		rc = vedis_begin(self->db);
		rc = (rc == VEDIS_OK ? 0 : -1);
		break;
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
		rc = vedis_commit(self->db);
		if(rc == VEDIS_OK) {
			rc = 0;
		} else if( rc == VEDIS_BUSY) {
			vedis_rollback(self->db);
			ia_log("rollback\n");
			rc = -1;
		} else {
			ia_log("another error, rc = %d\n", rc);
			rc = -1;
		}
		break;
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
	iaprivate *self = ioarena.driver->priv;
/*
	char key[kv->ksize + 1];
	memcpy(key, kv->k, kv->ksize);
	key[kv->ksize] = 0;
	char value[kv->vsize + 1];
	memcpy(value, kv->v, kv->vsize);
	value[kv->vsize] = 0;
	
	ia_log("KEY = \"%s\", VALUE = \"%s\"\n", key, value);
*/
	char buf[ioarena.conf.vsize];
	buf[0] = 0;
	vedis_int64 size = 0;
	switch(step) {
	case IA_SET:
		rc = vedis_kv_store(self->db, kv->k, kv->ksize, kv->v, kv->vsize);
		if(rc != VEDIS_OK) ia_log("error vedis_kv_store, rc = %d\n", rc);
		rc = (rc == VEDIS_OK ? 0 : -1);
		break;
	case IA_DELETE:
		rc = vedis_kv_delete(self->db, kv->k, kv->ksize);
		rc = (rc == VEDIS_OK ? 0 : -1);
		if(rc == VEDIS_OK) {
			//ia_log("delete ok\n", buf);
			rc = 0;
		} else if(rc == VEDIS_NOTFOUND) {
			//ia_log("key not exists\n");
			rc = 0;
		} else if(rc == VEDIS_BUSY) {
			ia_log("busy\n");
			rc = 0;
		} else {
			ia_log("other error (OS specific), rc = %d\n", rc);
			rc = -1;
		}
		break;
	case IA_ITERATE:
	case IA_GET:
		size = ioarena.conf.vsize;
		rc = vedis_kv_fetch(self->db, kv->k, kv->ksize, buf, &size);
		//rc = vedis_kv_fetch_callback(self->db,kv->k,kv->ksize,data_consumer_callback,0);
		if(rc == VEDIS_OK) {
			//ia_log("found value: %s\n", buf);
			rc = 0;
		} else if(rc == VEDIS_NOTFOUND) {
			ia_log("value not found\n");
			rc = 0;
		} else if(rc == VEDIS_BUSY) {
			ia_log("busy\n");
			rc = 0;
		} else if(rc == VEDIS_ABORT) {
			ia_log("VEDIS_ABORT, rc = %d, \"%s\" \"%s\"\n", rc, kv->k, buf);
			rc = -1;		
		} else {
			ia_log("other error (OS specific), rc = %d\n", rc);
			rc = -1;
		}
		break;
	default:
		assert(0);
		rc = -1;
	}
	//diag_put(self->db);
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
