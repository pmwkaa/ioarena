/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
 */

#include "ioarena.h"
#include "ups/upscaledb.h"

struct iaprivate {
    ups_env_t *env;
    ups_db_t *db;
};

struct iacontext {
  ups_txn_t *txn;
  ups_cursor_t *cursor;
};

static int ia_upscaledb_open(const char *datadir) {
  iadriver *drv = ioarena.driver;
  drv->priv = malloc(sizeof(iaprivate));
  if (drv->priv == NULL)
    return -1;

  iaprivate *self = drv->priv;

  char *filename = NULL;
  if (asprintf(&filename, "%s/test.db", datadir) < 0)
    return -1;

  uint32_t flags = UPS_ENABLE_TRANSACTIONS;
  switch (ioarena.conf.syncmode) {
  case IA_SYNC:
    flags += UPS_ENABLE_FSYNC;
    flags += UPS_FLUSH_TRANSACTIONS_IMMEDIATELY;
    break;
  case IA_LAZY:
    // Upscaledb does not seem to have an equivalent
    break;
  case IA_NOSYNC:
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
    // Journal is on by default
    break;
  case IA_WAL_OFF:
    flags += UPS_DISABLE_RECOVERY;
    break;
  default:
    ia_log("error: %s(): unsupported walmode %s", __func__,
           ia_walmode2str(ioarena.conf.walmode));
    return -1;
  }

  ups_status_t st = ups_env_create(&self->env, filename, flags, 0664, 0);
  free(filename);
  if (st != UPS_SUCCESS)
    return -1;

  st = ups_env_create_db(self->env, &self->db, 1, 0, 0);
  if (st != UPS_SUCCESS)
    return -1;

  return 0;
}

static int ia_upscaledb_close(void) {
  iaprivate *self = ioarena.driver->priv;
  if (self) {
    ups_db_close(self->db, 0);
    ups_env_close(self->env, 0);
    free(self);
  }
  return 0;
}

void ia_upscaledb_thread_dispose(iacontext *ctx) {
  if (ctx->cursor)
    ups_cursor_close(ctx->cursor);
  if (ctx->txn)
    ups_txn_abort(ctx->txn, 0);
  free(ctx);
}

static iacontext *ia_upscaledb_thread_new(void) {
  iacontext *ctx = calloc(1, sizeof(iacontext));
  return ctx;
}

static int ia_upscaledb_begin(iacontext *ctx, iabenchmark step) {
  iaprivate *self = ioarena.driver->priv;
  ups_status_t st;

  switch (step) {
  case IA_GET:
  case IA_SET:
  case IA_DELETE:
    break;

  case IA_ITERATE:
    st = ups_cursor_create(&ctx->cursor, self->db, ctx->txn, 0);
    if (!ctx->cursor)
      goto bailout;
    break;

  case IA_BATCH:
  case IA_CRUD:
    st = ups_txn_begin(&ctx->txn, self->env, NULL, NULL, 0);
    if (!ctx->txn)
      goto bailout;
    break;

  default:
    assert(0);
    return -1;
  }

  return 0;

bailout:
  ia_log("error: %s, %s (%d)", __func__, ia_benchmarkof(step), st);
  return -1;
}

static int ia_upscaledb_done(iacontext *ctx, iabenchmark step) {
  ups_status_t st;

  switch (step) {
  case IA_GET:
  case IA_SET:
  case IA_DELETE:
    break;

  case IA_ITERATE:
    if (ctx->cursor) {
      ups_cursor_close(ctx->cursor);
      ctx->cursor = NULL;
    }
    break;

  case IA_CRUD:
  case IA_BATCH:
    if (ctx->txn) {
      st = ups_txn_commit(ctx->txn, 0);
      if (st != UPS_SUCCESS)
        goto bailout;
      ctx->txn = NULL;
    }
    break;

  default:
    assert(0);
    return -1;
  }

  return 0;

bailout:
  ia_log("error: %s, %s (%d)", __func__, ia_benchmarkof(step), st);
  return -1;
}

static int ia_upscaledb_next(iacontext *ctx, iabenchmark step, iakv *kv) {
  iaprivate *self = ioarena.driver->priv;
  ups_status_t st;
  ups_key_t key = { 0 };
  ups_record_t value = { 0 };

  if (kv) {
    key.size = kv->ksize;
    key.data = kv->k;
  }

  switch (step) {
  case IA_SET:
    value.size = kv->vsize;
    value.data = kv->v;
    st = ups_db_insert(self->db, ctx->txn, &key, &value, UPS_OVERWRITE);
    if (st != UPS_SUCCESS)
      goto bailout;
    break;

  case IA_DELETE:
    st = ups_db_erase(self->db, ctx->txn, &key, 0);
    if (st != UPS_SUCCESS) {
      if (st == UPS_KEY_NOT_FOUND)
        return ENOENT;
      goto bailout;
    }
    break;

  case IA_GET:
    st = ups_db_find(self->db, ctx->txn, &key, &value, 0);
    if (st != UPS_SUCCESS) {
      if (st != UPS_KEY_NOT_FOUND)
        goto bailout;
    }
    kv->v = value.data;
    kv->vsize = value.size;
    break;

  case IA_ITERATE:
    st = ups_cursor_move(ctx->cursor, &key, &value, UPS_CURSOR_NEXT);
    if (st != UPS_SUCCESS) {
      if (st == UPS_KEY_NOT_FOUND)
        return ENOENT;
      goto bailout;
    }
    kv->k = key.data;
    kv->ksize = key.size;
    kv->v = value.data;
    kv->vsize = value.size;
    break;

  default:
    assert(0);
    return -1;
  }

  return 0;

bailout:
  ia_log("error: %s, %s (%d)", __func__, ia_benchmarkof(step), st);
  return -1;
}

iadriver ia_upscaledb = {
    .name = "upscaledb",
    .priv = NULL,
    .open = ia_upscaledb_open,
    .close = ia_upscaledb_close,

    .thread_new = ia_upscaledb_thread_new,
    .thread_dispose = ia_upscaledb_thread_dispose,
    .begin = ia_upscaledb_begin,
    .next = ia_upscaledb_next,
    .done = ia_upscaledb_done
};
