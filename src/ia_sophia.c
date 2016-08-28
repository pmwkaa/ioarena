
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

#include <ioarena.h>
#include <sophia.h>

struct iaprivate {
	void *env;
	void *db;
};

struct iacontext {
	void *cursor;
	void *entry;
	void *txn;
};

static void
ia_sophia_on_log(char *trace, void *arg)
{
	ia_log("sophia: %s", trace);
	(void)arg;
}

static int ia_sophia_open(const char *datadir)
{
	char *error;
	iadriver *drv = ioarena.driver;
	drv->priv = calloc(1, sizeof(iaprivate));
	if (drv->priv == NULL)
		return -1;

	iaprivate *self = drv->priv;
	self->env = sp_env();
	if (self->env == NULL)
		return -1;
	sp_setstring(self->env, "sophia.path", datadir, 0);
	sp_setstring(self->env, "sophia.on_log",
	             (void*)(uintptr_t)ia_sophia_on_log, 0);
	sp_setstring(self->env, "db", "test", 0);
	sp_setstring(self->env, "db.test.scheme", "key", 0);
	sp_setstring(self->env, "db.test.scheme.key", "string, key(0)", 0);
	sp_setstring(self->env, "db.test.scheme", "value", 0);
	sp_setstring(self->env, "db.test.scheme.value", "string", 0);

	/* LY: suggestions are welcome */
	switch(ioarena.conf.syncmode) {
	case IA_SYNC:
		sp_setint(self->env, "log.sync", 1);
		sp_setint(self->env, "db.test.sync", 1);
		break;
	case IA_LAZY:
		sp_setint(self->env, "log.sync", 0);
		sp_setint(self->env, "log.rotate_sync", 0);
		sp_setint(self->env, "db.test.sync", 1);
	case IA_NOSYNC:
		sp_setint(self->env, "log.sync", 0);
		sp_setint(self->env, "log.rotate_sync", 0);
		sp_setint(self->env, "db.test.sync", 0);
		break;
	default:
		ia_log("error: %s(): unsupported syncmode %s",
			__func__, ia_syncmode2str(ioarena.conf.syncmode));
		return -1;
	}

	switch(ioarena.conf.walmode) {
	case IA_WAL_INDEF:
		break;
	case IA_WAL_ON:
		sp_setint(self->env, "log.enable", 1);
		break;
	case IA_WAL_OFF:
		sp_setint(self->env, "log.enable", 0);
		break;
	default:
		ia_log("error: %s(): unsupported walmode %s",
			__func__, ia_walmode2str(ioarena.conf.walmode));
		return -1;
	}

	int rc = sp_open(self->env);
	if (rc == -1)
		goto bailout;

	self->db = sp_getobject(self->env, "db.test");
	assert(self->db != NULL);

	return 0;

bailout:
	error = sp_getstring(self->env, "sophia.error", 0);
	ia_log("error: %s, %s (%d)", __func__,
		error, (int) sp_getint(self->env, "sophia.error"));
	free(error);
	return -1;
}

static int ia_sophia_close(void)
{
	int rc = 0;
	iaprivate *self = ioarena.driver->priv;
	if (self) {
		ioarena.driver->priv = NULL;
		if (self->env)
			rc = sp_destroy(self->env);
		free(self);
	}
	return rc;
}

static iacontext* ia_sophia_thread_new(void)
{
	iacontext* ctx = calloc(1, sizeof(iacontext));
	return ctx;
}

void ia_sophia_thread_dispose(iacontext *ctx)
{
	if (ctx->cursor)
		sp_destroy(ctx->cursor);
	if (ctx->txn)
		sp_destroy(ctx->txn);
	if (ctx->entry)
		sp_destroy(ctx->entry);
	free(ctx);
}

static int ia_sophia_begin(iacontext *ctx, iabenchmark step)
{
	iaprivate *self = ioarena.driver->priv;
	char *error;
	int rc = 0;

	switch(step) {
	case IA_SET:
	case IA_DELETE:
	case IA_GET:
		ctx->txn = self->db;
		break;

	case IA_BATCH:
	case IA_CRUD:
		ctx->txn = sp_begin(self->env);
		if (ctx->txn == NULL)
			goto bailout;
		break;

	case IA_ITERATE:
		ctx->cursor = sp_cursor(self->env);
		if (ctx->cursor == NULL)
			goto bailout;
		ctx->entry = sp_document(self->db);
		break;

	default:
		assert(0);
		rc = -1;
	}

	return rc;

bailout:
	error = sp_getstring(self->env, "sophia.error", 0);
	ia_log("error: %s, %s, %s (%d)", __func__,
		ia_benchmarkof(step), error, (int) sp_getint(self->env, "sophia.error"));
	free(error);
	return -1;
}

static int ia_sophia_done(iacontext* ctx, iabenchmark step)
{
	iaprivate *self = ioarena.driver->priv;
	char *error;
	int rc = 0;

	switch(step) {
	case IA_SET:
	case IA_DELETE:
	case IA_GET:
		ctx->txn = NULL;
		break;

	case IA_BATCH:
	case IA_CRUD:
		if (ctx->txn)
			rc = sp_commit(ctx->txn);
		if (rc) {
			sp_destroy(ctx->txn);
			ctx->txn = NULL;
			goto bailout;
		}
		ctx->txn = NULL;
		break;

	case IA_ITERATE:
		if (ctx->entry) {
			sp_destroy(ctx->entry);
			ctx->entry = NULL;
		}
		if (ctx->cursor) {
			sp_destroy(ctx->cursor);
			ctx->cursor = NULL;
		}
		ctx->txn = NULL;
		break;

	default:
		assert(0);
		rc = -1;
	}

	return rc;

bailout:
	error = sp_getstring(self->env, "sophia.error", 0);
	ia_log("error: %s, %s, %s (%d)", __func__,
		ia_benchmarkof(step), error, (int) sp_getint(self->env, "sophia.error"));
	free(error);
	return -1;
}

static int ia_sophia_next(iacontext* ctx, iabenchmark step, iakv *kv)
{
	iaprivate *self = ioarena.driver->priv;
	char *error;
	int rc = 0;

	switch(step) {
	case IA_SET:
		ctx->entry = sp_document(self->db);
		sp_setstring(ctx->entry, "key", kv->k, kv->ksize);
		sp_setstring(ctx->entry, "value", kv->v, kv->vsize);
		rc = sp_set(ctx->txn, ctx->entry);
		if (rc == -1)
			goto bailout;
		ctx->entry = NULL;
		break;

	case IA_DELETE:
		ctx->entry = sp_document(self->db);
		sp_setstring(ctx->entry, "key", kv->k, kv->ksize);
		rc = sp_delete(ctx->txn, ctx->entry);
		if (rc == -1)
			goto bailout;
		ctx->entry = NULL;
		break;

	case IA_GET:
		if (ctx->entry)
			sp_destroy(ctx->entry);
		ctx->entry = sp_document(self->db);
		sp_setstring(ctx->entry, "key", kv->k, kv->ksize);
		ctx->entry = sp_get(self->db, ctx->entry);
		if (ctx->entry == NULL) {
			error = sp_getstring(self->env, "sophia.error", 0);
			if (error)
				goto error_done;
			rc = ctx->txn ? 0 : ENOENT; /* TODO */
		} else {
			int vlen = 0;
			kv->v = sp_getstring(ctx->entry, "value", &vlen);
			if (kv->v == NULL) {
				error = sp_getstring(self->env, "sophia.error", 0);
				if (error)
					goto error_done;
				assert(vlen == 0);
			}
			kv->vsize = vlen;
		}
		break;

	case IA_ITERATE:
		ctx->entry = sp_get(ctx->cursor, ctx->entry);
		if (ctx->entry == NULL) {
			error = sp_getstring(self->env, "sophia.error", 0);
			if (error)
				goto error_done;
			rc = ENOENT;
		} else {
			int klen = 0;
			kv->k = sp_getstring(ctx->entry, "key", &klen);
			if (kv->k == NULL)
				goto bailout;
			kv->ksize = klen;

			int vlen = 0;
			kv->v = sp_getstring(ctx->entry, "value", &vlen);
			if (kv->v == NULL) {
				error = sp_getstring(self->env, "sophia.error", 0);
				if (error)
					goto error_done;
				assert(vlen == 0);
			}
			kv->vsize = vlen;
		}
		break;

	default:
		assert(0);
		rc = -1;
	}

	return rc;

bailout:
	error = sp_getstring(self->env, "sophia.error", 0);
error_done:
	ia_log("error: %s, %s, %s (%d)", __func__,
		ia_benchmarkof(step), error, (int) sp_getint(self->env, "sophia.error"));
	free(error);
	return -1;
}

iadriver ia_sophia =
{
	.name  = "sophia",
	.priv  = NULL,
	.open  = ia_sophia_open,
	.close = ia_sophia_close,

	.thread_new = ia_sophia_thread_new,
	.thread_dispose = ia_sophia_thread_dispose,
	.begin	= ia_sophia_begin,
	.next	= ia_sophia_next,
	.done	= ia_sophia_done
};
