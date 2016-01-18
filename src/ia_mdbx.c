
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) Dmitry Simonenko
 * Copyright (c) Leonid Yuriev
 * BSD License
*/

#include <ioarena.h>

/* LY: Ugly, but what solution is better? */
#include "../db/mdbx/mdbx.h"

struct iaprivate {
	MDB_env *env;
	MDB_dbi dbi;
};

#define INVALID_DBI ((MDB_dbi)-1)

struct iacontext {
	MDB_txn* txn;
	MDB_cursor *cursor;
};

static int ia_mdbx_open(const char *datadir)
{
	unsigned modeflags;
	iadriver *drv = ioarena.driver;

	ia_log(
		"WARNING: Currently MDBX don't have any releases and include a few additional checks, TODOs and bottlenecks.\n"
		"Please wait for the release to compare performance and set of features.\n"
		"https://github.com/ReOpen/libmdbx/issues/1\n"
	);

	drv->priv = calloc(1, sizeof(iaprivate));
	if (drv->priv == NULL)
		return -1;

	iaprivate *self = drv->priv;
	self->dbi = INVALID_DBI;
	int rc = mdbx_env_create(&self->env);
	if (rc != MDB_SUCCESS)
		goto bailout;
	rc = mdbx_env_set_mapsize(self->env, 4 * 1024 * 1024 * 1024ULL /* TODO */);
	if (rc != MDB_SUCCESS)
		goto bailout;

	/* LY: suggestions are welcome */
	switch(ioarena.conf.syncmode) {
	case IA_SYNC:
		modeflags = MDB_LIFORECLAIM;
		break;
	case IA_LAZY:
		modeflags = MDB_NOSYNC|MDB_NOMETASYNC|MDB_LIFORECLAIM;
		break;
	case IA_NOSYNC:
		modeflags = MDB_WRITEMAP|MDB_UTTERLY_NOSYNC|MDB_NOMETASYNC|MDB_LIFORECLAIM;
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

	rc = mdbx_env_open(self->env, datadir, MDB_CREATE|modeflags|MDB_NORDAHEAD, 0644);
	if (rc != MDB_SUCCESS)
		goto bailout;
	return 0;

bailout:
	ia_log("error: %s, %s (%d)", __func__, mdbx_strerror(rc), rc);
	return -1;
}

static int ia_mdbx_close(void)
{
	iaprivate *self = ioarena.driver->priv;
	if (self) {
		ioarena.driver->priv = NULL;
		if (self->dbi != INVALID_DBI)
			mdbx_dbi_close(self->env, self->dbi);
		if (self->env)
			mdbx_env_close(self->env);
		free(self);
	}
	return 0;
}

static iacontext* ia_mdbx_thread_new(void)
{
	iaprivate *self = ioarena.driver->priv;
	int rc;

	if (self->dbi == INVALID_DBI) {
		MDB_txn *txn = NULL;

		rc = mdbx_txn_begin(self->env, NULL, 0, &txn);
		if (rc != MDB_SUCCESS)
			goto bailout;

		rc = mdbx_dbi_open(txn, NULL, 0, &self->dbi);
		mdbx_txn_abort(txn);
		if (rc != MDB_SUCCESS)
			goto bailout;

		assert(self->dbi != INVALID_DBI);
	}

	iacontext* ctx = calloc(1, sizeof(iacontext));
	return ctx;

bailout:
	ia_log("error: %s, %s (%d)", __func__, mdbx_strerror(rc), rc);
	return NULL;
}

void ia_mdbx_thread_dispose(iacontext *ctx)
{
	if (ctx->cursor)
		mdbx_cursor_close(ctx->cursor);
	if (ctx->txn)
		mdbx_txn_abort(ctx->txn);
	free(ctx);
}

static int ia_mdbx_begin(iacontext *ctx, iabenchmark step)
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
			mdbx_cursor_close(ctx->cursor);
			ctx->cursor = NULL;
		}
		if (ctx->txn) {
			/* LY: transaction could NOT be reused for read/write. */
			mdbx_txn_abort(ctx->txn);
			ctx->txn = NULL;
		}
		rc = mdbx_txn_begin(self->env, NULL, 0, &ctx->txn);
		if (rc != MDB_SUCCESS)
			goto bailout;
		break;

	case IA_ITERATE:
	case IA_GET:
		if (ctx->txn) {
			rc = mdbx_txn_renew(ctx->txn);
			if (rc != MDB_SUCCESS) {
				mdbx_txn_abort(ctx->txn);
				ctx->txn = NULL;
			}
		}
		if (ctx->txn == NULL) {
			rc = mdbx_txn_begin(self->env, NULL, MDB_RDONLY, &ctx->txn);
			if (rc != MDB_SUCCESS)
				goto bailout;
		}

		if (step == IA_ITERATE) {
			if (ctx->cursor) {
				rc = mdbx_cursor_renew(ctx->txn, ctx->cursor);
				if (rc != MDB_SUCCESS) {
					mdbx_cursor_close(ctx->cursor);
					ctx->cursor = NULL;
				}
			}
			if (ctx->cursor == NULL) {
				rc = mdbx_cursor_open(ctx->txn, self->dbi, &ctx->cursor);
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
		ia_benchmarkof(step), mdbx_strerror(rc), rc);
	return -1;
}

static int ia_mdbx_done(iacontext* ctx, iabenchmark step)
{
	int rc;

	switch(step) {
	case IA_SET:
	case IA_BATCH:
	case IA_CRUD:
	case IA_DELETE:
		rc = mdbx_txn_commit(ctx->txn);
		if (rc != MDB_SUCCESS) {
			mdbx_txn_abort(ctx->txn);
			goto bailout;
		}
		ctx->txn = NULL;
		break;

	case IA_ITERATE:
	case IA_GET:
		mdbx_txn_reset(ctx->txn);
		rc = 0;
		break;

	default:
		assert(0);
		rc = -1;
	}

	return rc;

bailout:
	ia_log("error: %s, %s, %s (%d)", __func__,
		ia_benchmarkof(step), mdbx_strerror(rc), rc);
	return -1;
}

static int ia_mdbx_next(iacontext* ctx, iabenchmark step, iakv *kv)
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
		rc = mdbx_put(ctx->txn, self->dbi, &k, &v, 0);
		if (rc != MDB_SUCCESS)
			goto bailout;
		break;

	case IA_DELETE:
		k.mv_data = kv->k;
		k.mv_size = kv->ksize;
		rc = mdbx_del(ctx->txn, self->dbi, &k, 0);
		if (rc == MDB_NOTFOUND)
			rc = ENOENT;
		else if (rc != MDB_SUCCESS)
			goto bailout;
		break;

	case IA_ITERATE:
		rc = mdbx_cursor_get(ctx->cursor, &k, &v, MDB_NEXT);
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
		rc = mdbx_get(ctx->txn, self->dbi, &k, &v);
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
		ia_benchmarkof(step), mdbx_strerror(rc), rc);
	return -1;
}

iadriver ia_mdbx =
{
	.name  = "mdbx",
	.priv  = NULL,
	.open  = ia_mdbx_open,
	.close = ia_mdbx_close,

	.thread_new = ia_mdbx_thread_new,
	.thread_dispose = ia_mdbx_thread_dispose,
	.begin	= ia_mdbx_begin,
	.next	= ia_mdbx_next,
	.done	= ia_mdbx_done
};
