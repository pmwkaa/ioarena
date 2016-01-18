
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

#include <ioarena.h>
#include <leveldb/c.h>

struct iaprivate {
	leveldb_options_t *opts;
	leveldb_readoptions_t *ropts;
	leveldb_writeoptions_t *wopts;
	leveldb_t *db;
};

struct iacontext {
	leveldb_iterator_t *it;
	leveldb_writebatch_t *batch;
	char* result;
};

static int ia_leveldb_open(const char *datadir)
{
	iadriver *drv = ioarena.driver;
	drv->priv = calloc(1, sizeof(iaprivate));
	if (drv->priv == NULL)
		return -1;

	iaprivate *self = drv->priv;
	self->opts = leveldb_options_create();
	leveldb_options_set_compression(self->opts, leveldb_no_compression);
	leveldb_options_set_info_log(self->opts, NULL);
	leveldb_options_set_create_if_missing(self->opts, 1);
	self->wopts = leveldb_writeoptions_create();
	self->ropts = leveldb_readoptions_create();
	leveldb_readoptions_set_fill_cache(self->ropts, 1);

	/* LY: suggestions are welcome */
	switch(ioarena.conf.syncmode) {
	case IA_SYNC:
		leveldb_writeoptions_set_sync(self->wopts, 1);
		break;
	case IA_LAZY:
	case IA_NOSYNC:
		leveldb_writeoptions_set_sync(self->wopts, 0);
		break;
	default:
		ia_log("error: %s(): unsupported syncmode %s",
			__func__, ia_syncmode2str(ioarena.conf.syncmode));
		return -1;
	}

	switch(ioarena.conf.walmode) {
	case IA_WAL_INDEF:
	case IA_WAL_ON:
		break;
	case IA_WAL_OFF:
	default:
		ia_log("error: %s(): unsupported walmode %s",
			__func__, ia_walmode2str(ioarena.conf.walmode));
		return -1;
	}

	char *error = NULL;
	self->db = leveldb_open(self->opts, datadir, &error);
	if (error != NULL)
		goto bailout;

	return 0;

bailout:
	ia_log("error: %s, %s", __func__, error);
	free(error);
	return -1;
}

static int ia_leveldb_close(void)
{
	iaprivate *self = ioarena.driver->priv;
	if (self) {
		ioarena.driver->priv = NULL;
		if (self->db)
			leveldb_close(self->db);
		if (self->ropts)
			leveldb_readoptions_destroy(self->ropts);
		if (self->wopts)
			leveldb_writeoptions_destroy(self->wopts);
		if (self->opts)
			leveldb_options_destroy(self->opts);
		free(self);
	}
	return 0;
}

static iacontext* ia_leveldb_thread_new(void)
{
	iacontext* ctx = calloc(1, sizeof(iacontext));
	return ctx;
}

void ia_leveldb_thread_dispose(iacontext *ctx)
{
	if (ctx->result)
		free(ctx->result);
	if (ctx->it)
		leveldb_iter_destroy(ctx->it);
	if (ctx->batch)
		leveldb_writebatch_destroy(ctx->batch);
	free(ctx);
}

static int ia_leveldb_begin(iacontext *ctx, iabenchmark step)
{
	iaprivate *self = ioarena.driver->priv;
	int rc = 0;
	const char *error = NULL;

	switch(step) {
	case IA_GET:
	case IA_SET:
	case IA_DELETE:
		break;

	case IA_ITERATE:
		ctx->it = leveldb_create_iterator(self->db, self->ropts);
		if (! ctx->it) {
			error = "leveldb_create_iterator() failed";
			goto bailout;
		}
		leveldb_iter_seek_to_first(ctx->it);
		break;

	case IA_CRUD:
	case IA_BATCH:
		ctx->batch = leveldb_writebatch_create();
		if (! ctx->batch) {
			error = "leveldb_writebatch_create() failed";
			goto bailout;
		}
		break;

	default:
		assert(0);
		rc = -1;
	}

	return rc;

bailout:
	ia_log("error: %s, %s, %s", __func__,
		ia_benchmarkof(step), error);
	return -1;
}

static int ia_leveldb_done(iacontext* ctx, iabenchmark step)
{
	iaprivate *self = ioarena.driver->priv;
	int rc = 0;
	char *error = NULL;

	switch(step) {
	case IA_GET:
		if (ctx->result) {
			free(ctx->result);
			ctx->result = NULL;
		}
	case IA_SET:
	case IA_DELETE:
		break;

	case IA_ITERATE:
		if (ctx->it) {
			leveldb_iter_destroy(ctx->it);
			ctx->it = NULL;
		}
		break;

	case IA_CRUD:
	case IA_BATCH:
		if (ctx->batch) {
			leveldb_write(self->db, self->wopts, ctx->batch, &error);
			if (error != NULL)
				goto bailout;
			leveldb_writebatch_destroy(ctx->batch);
			ctx->batch = NULL;
		}
		break;

	default:
		assert(0);
		rc = -1;
	}

	return rc;

bailout:
	ia_log("error: %s, %s, %s", __func__,
		ia_benchmarkof(step), error);
	free(error);
	return -1;
}

static int ia_leveldb_next(iacontext* ctx, iabenchmark step, iakv *kv)
{
	iaprivate *self = ioarena.driver->priv;
	int rc = 0;
	char *error = NULL;

	switch(step) {
	case IA_SET:
		if (ctx->batch)
			leveldb_writebatch_put(ctx->batch,
				kv->k, kv->ksize, kv->v, kv->vsize);
		else
			leveldb_put(self->db, self->wopts,
				kv->k, kv->ksize, kv->v, kv->vsize, &error);

		if (error)
			goto bailout;
		break;

	case IA_DELETE:
		if (ctx->batch)
			leveldb_writebatch_delete(ctx->batch, kv->k, kv->ksize);
		else
			leveldb_delete(self->db, self->wopts, kv->k, kv->ksize, &error);
		if (error)
			goto bailout;
		break;

	case IA_GET:
		if (ctx->result)
			free(ctx->result);
		ctx->result = leveldb_get(self->db, self->ropts, kv->k, kv->ksize, &kv->vsize, &error);
		if (error)
			goto bailout;
		if (! ctx->result) {
			if (! ctx->batch) /* TODO: rework to avoid */ {
				rc = ENOENT;
				break;
			}
		}
		kv->v = ctx->result;
		break;

	case IA_ITERATE:
		if (!leveldb_iter_valid(ctx->it))
			return ENOENT;
		kv->k = (char*) leveldb_iter_key(ctx->it, &kv->ksize);
		leveldb_iter_next(ctx->it);
		break;

	default:
		assert(0);
		rc = -1;
	}

	return rc;

bailout:
	ia_log("error: %s, %s, %s", __func__,
		ia_benchmarkof(step), error);
	free(error);
	return -1;
}

iadriver ia_leveldb =
{
	.name  = "leveldb",
	.priv  = NULL,
	.open  = ia_leveldb_open,
	.close = ia_leveldb_close,

	.thread_new = ia_leveldb_thread_new,
	.thread_dispose = ia_leveldb_thread_dispose,
	.begin	= ia_leveldb_begin,
	.next	= ia_leveldb_next,
	.done	= ia_leveldb_done
};
