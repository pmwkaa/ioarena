
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

#include <ioarena.h>
#include <lmdb.h>

struct iaprivate {
	MDB_env *env;
	MDB_dbi dbi;
};

#define INVALID_DBI ((MDB_dbi)-1)

struct iacontext {
	MDB_txn* txn;
	MDB_cursor *cursor;
};

static int ia_lmdb_open(const char *datadir)
{
	unsigned modeflags;
	iadriver *drv = ioarena.driver;

	drv->priv = calloc(1, sizeof(iaprivate));
	if (drv->priv == NULL)
		return -1;

	iaprivate *self = drv->priv;
	self->dbi = INVALID_DBI;
	int rc = mdb_env_create(&self->env);
	if (rc != MDB_SUCCESS)
		goto bailout;
	rc = mdb_env_set_mapsize(self->env, 4 * 1024 * 1024 * 1024ULL /* TODO */);
	if (rc != MDB_SUCCESS)
		goto bailout;

	/* LY: suggestions are welcome */
	switch(ioarena.conf.syncmode) {
	case IA_SYNC:
		modeflags = 0;
		break;
	case IA_LAZY:
		modeflags = MDB_NOSYNC|MDB_NOMETASYNC;
		break;
	case IA_NOSYNC:
		modeflags = MDB_WRITEMAP|MDB_MAPASYNC;
		break;
	default:
		ia_log("error: %s(): unsupported syncmode %s",
			__func__, ia_syncmode2str(ioarena.conf.syncmode));
		return -1;
	}

	switch(ioarena.conf.walmode) {
	case IA_WAL_INDEF:
	case IA_WAL_OFF:
		break;
	case IA_WAL_ON:
	default:
		ia_log("error: %s(): unsupported walmode %s",
			__func__, ia_walmode2str(ioarena.conf.walmode));
		return -1;
	}

	rc = mdb_env_open(self->env, datadir, modeflags|MDB_NORDAHEAD, 0644);
	if (rc != MDB_SUCCESS)
		goto bailout;
	return 0;

bailout:
	ia_log("error: %s, %s (%d)", __func__, mdb_strerror(rc), rc);
	return -1;
}

static int ia_lmdb_close(void)
{
	iaprivate *self = ioarena.driver->priv;
	if (self) {
		ioarena.driver->priv = NULL;
		if (self->dbi != INVALID_DBI)
			mdb_dbi_close(self->env, self->dbi);
		if (self->env)
			mdb_env_close(self->env);
		free(self);
	}
	return 0;
}

static iacontext* ia_lmdb_thread_new(void)
{
	iaprivate *self = ioarena.driver->priv;
	int rc;

	if (self->dbi == INVALID_DBI) {
		MDB_txn *txn = NULL;

		rc = mdb_txn_begin(self->env, NULL, 0, &txn);
		if (rc != MDB_SUCCESS)
			goto bailout;

		rc = mdb_dbi_open(txn, NULL, 0, &self->dbi);
		mdb_txn_abort(txn);
		if (rc != MDB_SUCCESS)
			goto bailout;

		assert(self->dbi != INVALID_DBI);
	}

	iacontext* ctx = calloc(1, sizeof(iacontext));
	return ctx;

bailout:
	ia_log("error: %s, %s (%d)", __func__, mdb_strerror(rc), rc);
	return NULL;
}

void ia_lmdb_thread_dispose(iacontext *ctx)
{
	if (ctx->cursor)
		mdb_cursor_close(ctx->cursor);
	if (ctx->txn)
		mdb_txn_abort(ctx->txn);
	free(ctx);
}

static int ia_lmdb_begin(iacontext *ctx, iabenchmark step)
{
	iaprivate *self = ioarena.driver->priv;
	int rc = 0;
	assert(self->dbi != INVALID_DBI);

	switch(step) {
	case IA_SET:
	case IA_BATCH:
	case IA_CRUD:
	case IA_DELETE:
		if (ctx->cursor) {
			/* LY: cursor could NOT be reused for read/write. */
			mdb_cursor_close(ctx->cursor);
			ctx->cursor = NULL;
		}
		if (ctx->txn) {
			/* LY: transaction could NOT be reused for read/write. */
			mdb_txn_abort(ctx->txn);
			ctx->txn = NULL;
		}
		rc = mdb_txn_begin(self->env, NULL, 0, &ctx->txn);
		if (rc != MDB_SUCCESS)
			goto bailout;
		break;

	case IA_ITERATE:
	case IA_GET:
		if (ctx->txn) {
			rc = mdb_txn_renew(ctx->txn);
			if (rc != MDB_SUCCESS) {
				mdb_txn_abort(ctx->txn);
				ctx->txn = NULL;
			}
		}
		if (ctx->txn == NULL) {
			rc = mdb_txn_begin(self->env, NULL, MDB_RDONLY, &ctx->txn);
			if (rc != MDB_SUCCESS)
				goto bailout;
		}

		if (step == IA_ITERATE) {
			if (ctx->cursor) {
				rc = mdb_cursor_renew(ctx->txn, ctx->cursor);
				if (rc != MDB_SUCCESS) {
					mdb_cursor_close(ctx->cursor);
					ctx->cursor = NULL;
				}
			}
			if (ctx->cursor == NULL) {
				rc = mdb_cursor_open(ctx->txn, self->dbi, &ctx->cursor);
				if (rc != MDB_SUCCESS)
					goto bailout;
			}
		}
		break;

	default:
		assert(0);
		rc = -1;
	}

	return rc;

bailout:
	ia_log("error: %s, %s, %s (%d)", __func__,
		ia_benchmarkof(step), mdb_strerror(rc), rc);
	return -1;
}

static int ia_lmdb_done(iacontext* ctx, iabenchmark step)
{
	int rc;

	switch(step) {
	case IA_SET:
	case IA_BATCH:
	case IA_CRUD:
	case IA_DELETE:
		rc = mdb_txn_commit(ctx->txn);
		if (rc != MDB_SUCCESS) {
			mdb_txn_abort(ctx->txn);
			ctx->txn = NULL;
			goto bailout;
		}
		ctx->txn = NULL;
		break;

	case IA_ITERATE:
	case IA_GET:
		mdb_txn_reset(ctx->txn);
		rc = 0;
		break;

	default:
		assert(0);
		rc = -1;
	}

	return rc;

bailout:
	ia_log("error: %s, %s, %s (%d)", __func__,
		ia_benchmarkof(step), mdb_strerror(rc), rc);
	return -1;
}

static int ia_lmdb_next(iacontext* ctx, iabenchmark step, iakv *kv)
{
	iaprivate *self = ioarena.driver->priv;
	MDB_val k, v;
	int rc;

	switch(step) {
	case IA_SET:
		k.mv_data = kv->k;
		k.mv_size = kv->ksize;
		v.mv_data = kv->v;
		v.mv_size = kv->vsize;
		rc = mdb_put(ctx->txn, self->dbi, &k, &v, 0);
		if (rc != MDB_SUCCESS)
			goto bailout;
		break;

	case IA_DELETE:
		k.mv_data = kv->k;
		k.mv_size = kv->ksize;
		rc = mdb_del(ctx->txn, self->dbi, &k, 0);
		if (rc == MDB_NOTFOUND)
			rc = ENOENT;
		else if (rc != MDB_SUCCESS)
			goto bailout;
		break;

	case IA_ITERATE:
		rc = mdb_cursor_get(ctx->cursor, &k, &v, MDB_NEXT);
		if (rc == MDB_SUCCESS) {
			kv->k = k.mv_data;
			kv->ksize = k.mv_size;
			kv->v = v.mv_data;
			kv->vsize = v.mv_size;
		} else if (rc == MDB_NOTFOUND) {
			kv->k = NULL;
			kv->ksize = 0;
			kv->v = NULL;
			kv->vsize = 0;
			rc = ENOENT;
		} else {
			goto bailout;
		}
		break;

	case IA_GET:
		k.mv_data = kv->k;
		k.mv_size = kv->ksize;
		rc = mdb_get(ctx->txn, self->dbi, &k, &v);
		if (rc != MDB_SUCCESS) {
			if (rc != MDB_NOTFOUND)
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
	ia_log("error: %s, %s, %s (%d)", __func__,
		ia_benchmarkof(step), mdb_strerror(rc), rc);
	return -1;
}

iadriver ia_lmdb =
{
	.name  = "lmdb",
	.priv  = NULL,
	.open  = ia_lmdb_open,
	.close = ia_lmdb_close,

	.thread_new = ia_lmdb_thread_new,
	.thread_dispose = ia_lmdb_thread_dispose,
	.begin	= ia_lmdb_begin,
	.next	= ia_lmdb_next,
	.done	= ia_lmdb_done
};
