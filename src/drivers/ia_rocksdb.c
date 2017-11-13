
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

#include "ioarena.h"
#include "rocksdb/c.h"

struct iaprivate {
  rocksdb_options_t *opts;
  rocksdb_readoptions_t *ropts;
  rocksdb_writeoptions_t *wopts;
  rocksdb_t *db;
};

struct iacontext {
  rocksdb_iterator_t *it;
  rocksdb_writebatch_t *batch;
  char *result;
};

static int ia_rocksdb_open(const char *datadir) {
  iadriver *drv = ioarena.driver;
  drv->priv = malloc(sizeof(iaprivate));
  if (drv->priv == NULL)
    return -1;

  iaprivate *self = drv->priv;
  self->opts = rocksdb_options_create();
  rocksdb_options_set_compression(self->opts, rocksdb_no_compression);
  rocksdb_options_set_info_log(self->opts, NULL);
  rocksdb_options_set_create_if_missing(self->opts, 1);
  self->wopts = rocksdb_writeoptions_create();
  self->ropts = rocksdb_readoptions_create();
  rocksdb_readoptions_set_fill_cache(self->ropts, 0);

  /* LY: suggestions are welcome */
  switch (ioarena.conf.syncmode) {
  case IA_SYNC:
    rocksdb_writeoptions_set_sync(self->wopts, 1);
    rocksdb_options_set_use_fsync(self->opts, 1);
    break;
  case IA_LAZY:
  case IA_NOSYNC:
    rocksdb_writeoptions_set_sync(self->wopts, 0);
    rocksdb_options_set_use_fsync(self->opts, 0);
    break;
  default:
    ia_log("error: %s(): unsupported syncmode %s", __func__,
           ia_syncmode2str(ioarena.conf.syncmode));
    return -1;
  }

  switch (ioarena.conf.walmode) {
  case IA_WAL_INDEF:
    break;
  case IA_WAL_ON:
    rocksdb_writeoptions_disable_WAL(self->wopts, 0);
    break;
  case IA_WAL_OFF:
    rocksdb_writeoptions_disable_WAL(self->wopts, 1);
    break;
  default:
    ia_log("error: %s(): unsupported walmode %s", __func__,
           ia_walmode2str(ioarena.conf.walmode));
    return -1;
  }

  char *error = NULL;
  self->db = rocksdb_open(self->opts, datadir, &error);
  if (error != NULL)
    goto bailout;

  return 0;

bailout:
  ia_log("error: %s, %s", __func__, error);
  free(error);
  return -1;
}

static int ia_rocksdb_close(void) {
  iaprivate *self = ioarena.driver->priv;
  if (self) {
    ioarena.driver->priv = NULL;
    if (self->db)
      rocksdb_close(self->db);
    if (self->ropts)
      rocksdb_readoptions_destroy(self->ropts);
    if (self->wopts)
      rocksdb_writeoptions_destroy(self->wopts);
    if (self->opts)
      rocksdb_options_destroy(self->opts);
    free(self);
  }
  return 0;
}

void ia_rocksdb_thread_dispose(iacontext *ctx) {
  if (ctx->result)
    free(ctx->result);
  if (ctx->it)
    rocksdb_iter_destroy(ctx->it);
  if (ctx->batch)
    rocksdb_writebatch_destroy(ctx->batch);
  free(ctx);
}

static iacontext *ia_rocksdb_thread_new(void) {
  iacontext *ctx = calloc(1, sizeof(iacontext));
  return ctx;
}

static int ia_rocksdb_begin(iacontext *ctx, iabenchmark step) {
  iaprivate *self = ioarena.driver->priv;
  int rc = 0;
  const char *error = NULL;

  switch (step) {
  case IA_GET:
  case IA_SET:
  case IA_DELETE:
    break;

  case IA_ITERATE:
    ctx->it = rocksdb_create_iterator(self->db, self->ropts);
    if (!ctx->it) {
      error = "rocksdb_create_iterator() failed";
      goto bailout;
    }
    rocksdb_iter_seek_to_first(ctx->it);
    break;

  case IA_BATCH:
  case IA_CRUD:
    ctx->batch = rocksdb_writebatch_create();
    if (!ctx->batch) {
      error = "rocksdb_writebatch_create() failed";
      goto bailout;
    }
    break;

  default:
    assert(0);
    rc = -1;
  }

  return rc;

bailout:
  ia_log("error: %s, %s, %s", __func__, ia_benchmarkof(step), error);
  return -1;
}

static int ia_rocksdb_done(iacontext *ctx, iabenchmark step) {
  iaprivate *self = ioarena.driver->priv;
  int rc = 0;
  char *error = NULL;

  switch (step) {
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
      rocksdb_iter_destroy(ctx->it);
      ctx->it = NULL;
    }
    break;

  case IA_CRUD:
  case IA_BATCH:
    if (ctx->batch) {
      rocksdb_write(self->db, self->wopts, ctx->batch, &error);
      if (error != NULL)
        goto bailout;
      rocksdb_writebatch_destroy(ctx->batch);
      ctx->batch = NULL;
    }
    break;

  default:
    assert(0);
    rc = -1;
  }

  return rc;

bailout:
  ia_log("error: %s, %s, %s", __func__, ia_benchmarkof(step), error);
  free(error);
  return -1;
}

static int ia_rocksdb_next(iacontext *ctx, iabenchmark step, iakv *kv) {
  iaprivate *self = ioarena.driver->priv;
  int rc = 0;
  char *error = NULL;

  switch (step) {
  case IA_SET:
    if (ctx->batch)
      rocksdb_writebatch_put(ctx->batch, kv->k, kv->ksize, kv->v, kv->vsize);
    else
      rocksdb_put(self->db, self->wopts, kv->k, kv->ksize, kv->v, kv->vsize,
                  &error);

    if (error)
      goto bailout;
    break;

  case IA_DELETE:
    if (ctx->batch)
      rocksdb_writebatch_delete(ctx->batch, kv->k, kv->ksize);
    else
      rocksdb_delete(self->db, self->wopts, kv->k, kv->ksize, &error);
    if (error)
      goto bailout;
    break;

  case IA_GET:
    if (ctx->result)
      free(ctx->result);
    ctx->result = rocksdb_get(self->db, self->ropts, kv->k, kv->ksize,
                              &kv->vsize, &error);
    if (error)
      goto bailout;
    if (!ctx->result) {
      if (!ctx->batch) /* TODO: rework to avoid */ {
        rc = ENOENT;
        break;
      }
    }
    kv->v = ctx->result;
    break;

  case IA_ITERATE:
    if (!rocksdb_iter_valid(ctx->it))
      return ENOENT;
    kv->k = (char *)rocksdb_iter_key(ctx->it, &kv->ksize);
    rocksdb_iter_next(ctx->it);
    break;

  default:
    assert(0);
    rc = -1;
  }

  return rc;

bailout:
  ia_log("error: %s, %s, %s", __func__, ia_benchmarkof(step), error);
  free(error);
  return -1;
}

iadriver ia_rocksdb = {.name = "rocksdb",
                       .priv = NULL,
                       .open = ia_rocksdb_open,
                       .close = ia_rocksdb_close,

                       .thread_new = ia_rocksdb_thread_new,
                       .thread_dispose = ia_rocksdb_thread_dispose,
                       .begin = ia_rocksdb_begin,
                       .next = ia_rocksdb_next,
                       .done = ia_rocksdb_done};
