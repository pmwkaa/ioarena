/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
 */

#include "ioarena.h"
#include "unqlite.h"

struct iaprivate {
  unqlite *db;
};

struct iacontext {
  unqlite_kv_cursor *cursor;
};

static int ia_unqlite_open(const char *datadir) {
  iadriver *drv = ioarena.driver;
  drv->priv = malloc(sizeof(iaprivate));
  if (drv->priv == NULL)
    return -1;

  iaprivate *self = drv->priv;

  char *filename = NULL;
  if (asprintf(&filename, "%s/test.db", datadir) < 0)
    return -1;

  uint32_t flags = UNQLITE_OPEN_CREATE;

  switch (ioarena.conf.walmode) {
  case IA_WAL_INDEF:
    break;
  case IA_WAL_ON:
    // Journal is on by default
    break;
  case IA_WAL_OFF:
    flags += UNQLITE_OPEN_OMIT_JOURNALING;
    break;
  default:
    ia_log("error: %s(): unsupported walmode %s", __func__,
           ia_walmode2str(ioarena.conf.walmode));
    return -1;
  }

  int st = unqlite_open(&self->db, filename, flags);
  free(filename);
  if (st != UNQLITE_OK)
    return -1;

  return 0;
}

static int ia_unqlite_close(void) {
  iaprivate *self = ioarena.driver->priv;
  if (self) {
    unqlite_close(self->db);
    free(self);
  }
  return 0;
}

static void ia_unqlite_thread_dispose(iacontext *ctx) {
  iaprivate *self = ioarena.driver->priv;
  if (ctx->cursor)
    unqlite_kv_cursor_release(self->db, ctx->cursor);
  free(ctx);
}

static iacontext *ia_unqlite_thread_new(void) {
  iacontext *ctx = calloc(1, sizeof(iacontext));
  return ctx;
}

static int ia_unqlite_begin(iacontext *ctx, iabenchmark step) {
  iaprivate *self = ioarena.driver->priv;
  int st;

  switch (step) {
  case IA_GET:
  case IA_SET:
  case IA_DELETE:
    break;

  case IA_ITERATE:
    st = unqlite_kv_cursor_init(self->db, &ctx->cursor);
    if (st != UNQLITE_OK)
      goto bailout;
    break;

  case IA_BATCH:
  case IA_CRUD:
    st = unqlite_begin(self->db);
    if (st != UNQLITE_OK)
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

static int ia_unqlite_done(iacontext *ctx, iabenchmark step) {
  iaprivate *self = ioarena.driver->priv;
  int st;

  switch (step) {
  case IA_GET:
  case IA_SET:
  case IA_DELETE:
    break;

  case IA_ITERATE:
    if (ctx->cursor) {
      unqlite_kv_cursor_release(self->db, ctx->cursor);
      ctx->cursor = NULL;
    }
    break;

  case IA_CRUD:
  case IA_BATCH:
    st = unqlite_commit(self->db);
    if (st != UNQLITE_OK)
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

static int ia_unqlite_next(iacontext *ctx, iabenchmark step, iakv *kv) {
  iaprivate *self = ioarena.driver->priv;
  char buffer[256];
  unqlite_int64 size = sizeof(buffer);
  int st;

  switch (step) {
  case IA_SET:
    st = unqlite_kv_store(self->db, kv->k, kv->ksize, kv->v, kv->vsize);
    if (st != UNQLITE_OK)
      goto bailout;
    break;

  case IA_DELETE:
    st = unqlite_kv_delete(self->db, kv->k, kv->ksize);
    if (st != UNQLITE_OK) {
      if (st == UNQLITE_NOTFOUND)
        return ENOENT;
      goto bailout;
    }
    break;

  case IA_GET:
    st = unqlite_kv_fetch(self->db, kv->k, kv->ksize, buffer, &size);
    if (st != UNQLITE_OK) {
      if (st == UNQLITE_NOTFOUND)
        return ENOENT;
      goto bailout;
    }
    break;

  case IA_ITERATE:
    st = unqlite_kv_cursor_next_entry(ctx->cursor);
    if (st != UNQLITE_OK) {
      if (st == UNQLITE_NOTFOUND || st == UNQLITE_EOF)
        return ENOENT;
      goto bailout;
    }

    int key_size = kv->ksize;
    st = unqlite_kv_cursor_key(ctx->cursor, kv->k, &key_size);
    if (st != UNQLITE_OK) {
      goto bailout;
    }

    st = unqlite_kv_cursor_data(ctx->cursor, buffer, &size);
    if (st != UNQLITE_OK) {
      goto bailout;
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

iadriver ia_unqlite = {.name = "unqlite",
                       .priv = NULL,
                       .open = ia_unqlite_open,
                       .close = ia_unqlite_close,

                       .thread_new = ia_unqlite_thread_new,
                       .thread_dispose = ia_unqlite_thread_dispose,
                       .begin = ia_unqlite_begin,
                       .next = ia_unqlite_next,
                       .done = ia_unqlite_done};
