
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

#include <ioarena.h>
#include <wiredtiger.h>

struct iaprivate {
	WT_CONNECTION *conn;
	const char* session_config;
	const char* cursor_config;
	const char* transaction_config;
	const char* table_name;
	char need_checkpoints;
};

struct iacontext {
	WT_SESSION *session;
	WT_CURSOR *cursor;
	int transaction;
};

static int ia_wt_open(const char *datadir)
{
	iadriver *drv = ioarena.driver;
	drv->priv = calloc(1, sizeof(iaprivate));
	if (drv->priv == NULL)
		return -1;

	iaprivate *self = drv->priv;
	WT_SESSION *session = NULL;

	/* LY: suggestions are welcome */
	const char *durability_config = NULL;
	switch(ioarena.conf.syncmode) {
	case IA_SYNC:
		durability_config = "transaction_sync=(enabled=true),checkpoint_sync=true";
		break;
	case IA_LAZY:
		durability_config = "transaction_sync=(enabled=true,method=none),checkpoint_sync=true";
		break;
	case IA_NOSYNC:
		durability_config = "transaction_sync=(enabled=false),checkpoint_sync=false";
		break;
	default:
		ia_log("error: %s(): unsupported syncmode %s",
			__func__, ia_syncmode2str(ioarena.conf.syncmode));
		return -1;
	}

	const char *wal_config = NULL;
	switch(ioarena.conf.walmode) {
	case IA_WAL_INDEF:
		wal_config = "";
		break;
	case IA_WAL_ON:
		wal_config = "log=(enabled=true),";
		break;
	case IA_WAL_OFF:
		wal_config = "log=(enabled=false),";
		break;
	default:
		ia_log("error: %s(): unsupported walmode %s",
			__func__, ia_walmode2str(ioarena.conf.walmode));
		return -1;
	}

	if (ioarena.conf.syncmode == IA_SYNC && ioarena.conf.walmode != IA_WAL_ON)
		self->need_checkpoints = 1;

	char config[256];
	snprintf(config, sizeof(config), "create,cache_size=4GB, %s%s%s", wal_config, durability_config, ",session_max=300,");
	int rc = wiredtiger_open(datadir, NULL, config, &self->conn);
	if (rc != 0)
		goto bailout;

	rc = self->conn->open_session(self->conn, NULL, NULL, &session);
	if (rc != 0)
		goto bailout;

	self->table_name = "table:test";
	snprintf(config, sizeof(config),
			"split_pct=100,leaf_item_max=1KB,"
			"type=lsm,internal_page_max=4KB,leaf_page_max=4KB");
	rc = session->create(session, self->table_name, config);
	if (rc != 0)
		goto bailout;

	rc = session->close(session, NULL);
	if (rc != 0)
		goto bailout;
	return 0;

bailout:
	ia_log("error: %s, %s (%d)", __func__, wiredtiger_strerror(rc), rc);
	if (session) {
		(void) session->rollback_transaction(session, NULL);
		session->close(session, NULL);
	}
	return -1;
}

static int ia_wt_close(void)
{
	iaprivate *self = ioarena.driver->priv;
	if (self) {
		ioarena.driver->priv = NULL;
		if (self->conn)
			self->conn->close(self->conn, NULL);
		free(self);
	}
	return 0;
}

void ia_wiredtiger_thread_dispose(iacontext *ctx)
{
	if (ctx->cursor)
		ctx->cursor->close(ctx->cursor);
	if (ctx->session) {
		if (ctx->transaction)
			(void) ctx->session->rollback_transaction(ctx->session, NULL);
		ctx->session->close(ctx->session, NULL);
	}
	free(ctx);
}

static iacontext* ia_wiredtiger_thread_new(void)
{
	iaprivate *self = ioarena.driver->priv;
	iacontext* ctx = calloc(1, sizeof(iacontext));
	if (! ctx)
		return NULL;

	int rc = self->conn->open_session(self->conn, NULL, self->session_config, &ctx->session);
	if (rc != 0)
		goto bailout;

	rc = ctx->session->open_cursor(ctx->session, self->table_name, NULL, self->cursor_config, &ctx->cursor);
	if (rc != 0)
		goto bailout;

	return ctx;

bailout:
	ia_log("error: %s, %s (%d)", __func__, wiredtiger_strerror(rc), rc);
	ia_wiredtiger_thread_dispose(ctx);
	return NULL;
}

static int ia_wiredtiger_begin(iacontext *ctx, iabenchmark step)
{
	iaprivate *self = ioarena.driver->priv;
	int rc = 0;

	switch(step) {
	case IA_BATCH:
	case IA_CRUD:
		rc = ctx->session->begin_transaction(ctx->session, self->transaction_config);
		if (rc != 0)
			goto bailout;
		ctx->transaction = 1;

	case IA_SET:
	case IA_DELETE:
	case IA_GET:
		break;

	case IA_ITERATE:
		rc = ctx->cursor->reset(ctx->cursor);
		if (rc != 0)
			goto bailout;
		break;

	default:
		assert(0);
		rc = -1;
	}

	return rc;

bailout:
	ia_log("error: %s, %s, %s (%d)", __func__,
		ia_benchmarkof(step), wiredtiger_strerror(rc), rc);
	return -1;
}

static int ia_wiredtiger_done(iacontext* ctx, iabenchmark step)
{
	iaprivate *self = ioarena.driver->priv;
	int rc = 0;

	switch(step) {
	case IA_BATCH:
	case IA_CRUD:
		if (ctx->transaction) {
			rc = ctx->session->commit_transaction(ctx->session, NULL);
			if (rc != 0)
				goto bailout;
			ctx->transaction = 0;
		}

	case IA_DELETE:
	case IA_SET:
		if (self->need_checkpoints && rc == 0)
			rc = ctx->session->checkpoint(ctx->session, NULL);

	case IA_ITERATE:
	case IA_GET:
		break;

	default:
		assert(0);
		rc = -1;
	}

	return rc;

bailout:
	ia_log("error: %s, %s, %s (%d)", __func__,
		ia_benchmarkof(step), wiredtiger_strerror(rc), rc);
	if(ctx->transaction) {
		(void) ctx->session->rollback_transaction(ctx->session, NULL);
		ctx->transaction = 0;
	}
	return -1;
}

static int ia_wiredtiger_next(iacontext* ctx, iabenchmark step, iakv *kv)
{
	WT_ITEM k, v;
	int rc = 0;

	switch(step) {
	case IA_SET:
		k.data = kv->k;
		k.size = kv->ksize;
		v.data = kv->v;
		v.size = kv->vsize;
		ctx->cursor->set_key(ctx->cursor, &k);
		ctx->cursor->set_value(ctx->cursor, &v);
		rc = ctx->cursor->insert(ctx->cursor);
		if (rc != 0)
			goto bailout;
		break;

	case IA_DELETE:
		k.data = kv->k;
		k.size = kv->ksize;
		ctx->cursor->set_key(ctx->cursor, &k);
		rc = ctx->cursor->remove(ctx->cursor);
		if (rc != 0)
			goto bailout;
		break;

	case IA_GET:
		k.data = kv->k;
		k.size = kv->ksize;
		ctx->cursor->set_key(ctx->cursor, &k);
		rc = ctx->cursor->search(ctx->cursor);
		if (rc) {
			if (rc != WT_NOTFOUND)
				goto bailout;
			rc = ENOENT;
			break;
		}
		ctx->cursor->get_value(ctx->cursor, &v);
		kv->v = (char*) v.data;
		kv->vsize = v.size;
		break;

	case IA_ITERATE:
		rc = ctx->cursor->next(ctx->cursor);
		if (rc) {
			if (rc != WT_NOTFOUND)
				goto bailout;
			rc = ENOENT;
			break;
		}
		rc = ctx->cursor->get_key(ctx->cursor, &k);
		if (rc != 0)
			goto bailout;
		rc = ctx->cursor->get_value(ctx->cursor, &v);
		if (rc != 0)
			goto bailout;
		kv->k = (char*) k.data;
		kv->ksize = k.size;
		kv->v = (char*) v.data;
		kv->vsize = v.size;
		break;

	default:
		assert(0);
		rc = -1;
	}

	return rc;

bailout:
	ia_log("error: %s, %s, %s (%d)", __func__,
		ia_benchmarkof(step), wiredtiger_strerror(rc), rc);
	return -1;
}

iadriver ia_wt =
{
	.name  = "wiredtiger",
	.priv  = NULL,
	.open  = ia_wt_open,
	.close = ia_wt_close,

	.thread_new = ia_wiredtiger_thread_new,
	.thread_dispose = ia_wiredtiger_thread_dispose,
	.begin	= ia_wiredtiger_begin,
	.next	= ia_wiredtiger_next,
	.done	= ia_wiredtiger_done
};
