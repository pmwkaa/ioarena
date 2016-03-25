
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

#include <ioarena.h>
#include <forestdb.h>

struct iaprivate {
	fdb_config config;
	fdb_kvs_config kvs_config;
	char *data_filename;
	bool needsCompact;
};

struct iacontext {
	fdb_file_handle *db;
	fdb_kvs_handle *kvs;
	void* result;
	fdb_iterator *it;
	fdb_doc *doc;
	int transaction;
};

#define MANUAL_COMPACT

static int ia_forestdb_open(const char *datadir)
{
	iadriver *drv = ioarena.driver;
	drv->priv = calloc(1, sizeof(iaprivate));
	if (drv->priv == NULL)
		return -1;

	iaprivate *self = drv->priv;
	if (asprintf(&self->data_filename, "%s/io.arena", datadir) < 0)
		return -1;

	self->config = fdb_get_default_config();
	self->config.seqtree_opt             = FDB_SEQTREE_NOT_USE;
	self->config.compress_document_body  = false;
#ifdef MANUAL_COMPACT
	self->config.compaction_mode         = FDB_COMPACTION_MANUAL;
	self->needsCompact = true;
#else
	self->config.compaction_mode         = FDB_COMPACTION_AUTO;
	self->config.compaction_threshold    = 50;
	self->config.compactor_sleep_duration = 1; // otherwise compactor won't get time to start
#endif /* MANUAL_COMPACT */

	/* LY: suggestions are welcome */
	switch(ioarena.conf.syncmode) {
	case IA_SYNC:
		self->config.wal_flush_before_commit = 1;
		self->config.durability_opt          = FDB_DRB_ODIRECT;
		break;
	case IA_LAZY:
		self->config.wal_flush_before_commit = 1;
		self->config.durability_opt          = FDB_DRB_ASYNC;
		break;
	case IA_NOSYNC:
		self->config.wal_flush_before_commit = 0;
		self->config.durability_opt          = FDB_DRB_ASYNC;
		break;
	default:
		ia_log("error: %s(): unsupported syncmode %s",
			__FUNCTION__, ia_syncmode2str(ioarena.conf.syncmode));
		return -1;
	}

	switch(ioarena.conf.walmode) {
	case IA_WAL_INDEF:
	case IA_WAL_ON:
		break;
	case IA_WAL_OFF:
		self->config.wal_threshold           = 0; /* ? */
		break;
	default:
		ia_log("error: %s(): unsupported walmode %s",
			__FUNCTION__, ia_walmode2str(ioarena.conf.walmode));
		return -1;
	}

	fdb_status status = fdb_init(&self->config);
	if (status != FDB_RESULT_SUCCESS)
		goto bailout;

	self->kvs_config = fdb_get_default_kvs_config();
	return 0;

bailout:
	ia_log("error: %s, %s", __FUNCTION__, fdb_error_msg(status));
	fdb_shutdown();
	return -1;
}

static int ia_forestdb_close(void)
{
	iaprivate *self = ioarena.driver->priv;
	if (self) {
		ioarena.driver->priv = NULL;
		fdb_shutdown();
		free(self->data_filename);
		free(self);
	}
	return 0;
}

static iacontext* ia_forestdb_thread_new(void)
{
	fdb_status status;
	iaprivate *self = ioarena.driver->priv;
	iacontext* ctx = calloc(1, sizeof(iacontext));
	if (!ctx)
		return NULL;

	status = fdb_open(&ctx->db, self->data_filename, &self->config);
	if (status != FDB_RESULT_SUCCESS)
		goto bailout;

	status = fdb_kvs_open_default(ctx->db, &ctx->kvs, &self->kvs_config);
	if (status != FDB_RESULT_SUCCESS)
		goto bailout;
	return ctx;

bailout:
	ia_log("error: %s, %s", __FUNCTION__, fdb_error_msg(status));
	return NULL;
}

void ia_forestdb_thread_dispose(iacontext *ctx)
{
	if (ctx->result)
		fdb_free_block(ctx->result);
	if (ctx->it)
		(void) fdb_iterator_close(ctx->it);
	if (ctx->transaction)
		(void) fdb_abort_transaction(ctx->db);
	if (ctx->doc)
		fdb_doc_free(ctx->doc);

	if (ctx->kvs)
		fdb_kvs_close(ctx->kvs);
	if (ctx->db)
		fdb_close(ctx->db);

	free(ctx);
}

static int ia_forestdb_begin(iacontext *ctx, iabenchmark step)
{
	fdb_status status;
	int rc = 0;

	if (ioarena.driver->priv->needsCompact) {
		/* Compact before the first operation after opening the file */
		ioarena.driver->priv->needsCompact = false;
		status = fdb_compact(ctx->db, NULL);
		if (status != FDB_RESULT_SUCCESS)
			goto bailout;
	}

	switch(step) {
	case IA_CRUD:
		status = fdb_begin_transaction(ctx->db, FDB_ISOLATION_READ_COMMITTED);
		if (status != FDB_RESULT_SUCCESS)
			goto bailout;
		ctx->transaction = 1;
		break;

	case IA_BATCH:
	case IA_SET:
	case IA_DELETE:
	case IA_GET:
		break;

	case IA_ITERATE:
		status = fdb_iterator_init(ctx->kvs, &ctx->it,
			NULL, 0, NULL, 0, FDB_ITR_NO_DELETES);
		if (status != FDB_RESULT_SUCCESS)
			goto bailout;
		status = fdb_iterator_seek_to_min(ctx->it);
		if (status != FDB_RESULT_SUCCESS)
			goto bailout;
		break;

	default:
		assert(0);
		rc = -1;
	}

	return rc;

bailout:
	ia_log("error: %s, %s, %s", __FUNCTION__,
		ia_benchmarkof(step), fdb_error_msg(status));
	return -1;
}

static int ia_forestdb_done(iacontext* ctx, iabenchmark step)
{
	fdb_status status;
	int rc = 0;

	switch(step) {
	case IA_CRUD:
		if (ctx->transaction) {
			status = fdb_end_transaction(ctx->db, FDB_COMMIT_NORMAL);
			ctx->transaction = 0;
			if (status != FDB_RESULT_SUCCESS)
				goto bailout;
		}
        break;

	case IA_BATCH:
	case IA_DELETE:
	case IA_SET:
		status = fdb_commit(ctx->db, FDB_COMMIT_NORMAL);
		if (status != FDB_RESULT_SUCCESS)
			goto bailout;
		break;

	case IA_GET:
		if (ctx->result) {
			fdb_free_block(ctx->result);
			ctx->result = NULL;
		}
		break;

	case IA_ITERATE:
		if (ctx->doc) {
			fdb_doc_free(ctx->doc);
			ctx->doc = NULL;
		}
		if (ctx->it) {
			status = fdb_iterator_close(ctx->it);
			ctx->it = NULL;
			if (status != FDB_RESULT_SUCCESS)
				goto bailout;
		}
		break;

	default:
		assert(0);
		rc = -1;
	}

	return rc;

bailout:
	ia_log("error: %s, %s, %s", __FUNCTION__,
		ia_benchmarkof(step), fdb_error_msg(status));
	return -1;
}

static int ia_forestdb_next(iacontext* ctx, iabenchmark step, iakv *kv)
{
	fdb_status status;
	int rc = 0;

	switch(step) {
	case IA_SET:
		status = fdb_set_kv(ctx->kvs, kv->k, kv->ksize, kv->v, kv->vsize);
		if (status != FDB_RESULT_SUCCESS)
			goto bailout;
		break;

	case IA_DELETE:
		status = fdb_del_kv(ctx->kvs, kv->k, kv->ksize);
		if (status != FDB_RESULT_SUCCESS)
			goto bailout;
		break;

	case IA_GET:
		if (ctx->result) {
			fdb_free_block(ctx->result);
			ctx->result = NULL;
		}
		status = fdb_get_kv(ctx->kvs, kv->k, kv->ksize, &ctx->result, &kv->vsize);
		if (status != FDB_RESULT_SUCCESS) {
			if (status != FDB_RESULT_KEY_NOT_FOUND)
				goto bailout;
			if (! ctx->transaction) /* TODO: rework to avoid*/ {
				rc = ENOENT;
				break;
			}
		}
		kv->v = ctx->result;
		break;

	case IA_ITERATE:
		/* if (ctx->doc) {
			fdb_doc_free(ctx->doc);
			ctx->doc = NULL;
		} */
		status = fdb_iterator_get(ctx->it, &ctx->doc);
		if (status != FDB_RESULT_SUCCESS)
			goto bailout;
		status = fdb_iterator_next(ctx->it);
		if (status != FDB_RESULT_SUCCESS) {
			if (status != FDB_RESULT_ITERATOR_FAIL)
				goto bailout;
			rc = ENOENT;
		}
		break;

	default:
		assert(0);
		rc = -1;
	}

	return rc;

bailout:
	ia_log("error: %s, %s, %s", __FUNCTION__,
		ia_benchmarkof(step), fdb_error_msg(status));
	return -1;
}

iadriver ia_forestdb =
{
	.name  = "forestdb",
	.priv  = NULL,
	.open  = ia_forestdb_open,
	.close = ia_forestdb_close,

	.thread_new = ia_forestdb_thread_new,
	.thread_dispose = ia_forestdb_thread_dispose,
	.begin	= ia_forestdb_begin,
	.next	= ia_forestdb_next,
	.done	= ia_forestdb_done
};
