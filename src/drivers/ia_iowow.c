/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
 */
#include "ioarena.h"
#include <iowow/iwkv.h>

struct iaprivate {
  IWKV iwkv;
  IWDB iwdb;
};

struct iacontext {
  IWKV_cursor cursor;
};

static int ia_iowow_open(const char *datadir) {
  iwrc rc = 0;
  const char *db_name = "/test.iowow";
  const int datadir_len = strlen(datadir);
  const int db_name_len = strlen(db_name);
  char db_path[datadir_len + db_name_len + 1];
  char *wp = db_path;
  memcpy(wp, datadir, datadir_len);
  wp += datadir_len;
  memcpy(wp, db_name, db_name_len);
  wp += db_name_len;
  *wp = '\0';

  iadriver *drv = ioarena.driver;
  drv->priv = calloc(1, sizeof(iaprivate));
  if (drv->priv == NULL) {
    rc = iwrc_set_errno(IW_ERROR_ALLOC, errno);
  }
  iaprivate *self = drv->priv;
  IWKV_OPTS opts = {.path = db_path};
  switch (ioarena.conf.walmode) {
  case IA_WAL_ON:
    opts.wal.enabled = true;
    break;
  case IA_WAL_OFF:
    opts.wal.enabled = false;
    break;
  case IA_WAL_INDEF:
    break;
  }
  rc = iwkv_open(&opts, &self->iwkv);
  RCGO(rc, finish);

  rc = iwkv_db(self->iwkv, 1, 0, &self->iwdb);
  RCGO(rc, finish);

finish:
  if (rc) {
    iwlog_ecode_error2(rc, "Failed to initialize db");
    return -1;
  }
  return 0;
}

static int ia_iowow_close(void) {
  iaprivate *self = ioarena.driver->priv;
  if (!self) {
    return 0;
  }
  iwrc rc = 0;
  ioarena.driver->priv = NULL;
  rc = iwkv_close(&self->iwkv);
  free(self);
  if (rc) {
    iwlog_ecode_error2(rc, "Failed to close db");
    return -1;
  }
  return 0;
}

static iacontext *ia_iowow_thread_new() { return calloc(1, sizeof(iacontext)); }

void ia_iowow_thread_dispose(iacontext *ctx) { free(ctx); }

static int ia_iowow_begin(iacontext *ctx, iabenchmark step) {
  iwrc rc = 0;
  iaprivate *self = ioarena.driver->priv;
  switch (step) {
  case IA_ITERATE:
    rc =
        iwkv_cursor_open(self->iwdb, &ctx->cursor, IWKV_CURSOR_BEFORE_FIRST, 0);
    break;
  case IA_SET:
  case IA_BATCH:
  case IA_CRUD:
  case IA_DELETE:
  case IA_GET:
    break;
  default:
    assert(0);
    rc = IW_ERROR_FAIL;
  }
  if (rc) {
    iwlog_ecode_error(rc, "Begin benchmark step: %d failed", step);
    return -1;
  }
  return 0;
}

static int ia_iowow_done(iacontext *ctx, iabenchmark step) {
  iwrc rc = 0;
  switch (step) {
  case IA_ITERATE:
    if (ctx->cursor) {
      rc = iwkv_cursor_close(&ctx->cursor);
    }
    break;
  case IA_SET:
  case IA_BATCH:
  case IA_CRUD:
  case IA_DELETE:
  case IA_GET:
    break;
  default:
    assert(0);
    rc = IW_ERROR_FAIL;
  }
  if (rc) {
    iwlog_ecode_error(rc, "Done benchmark step: %d failed", step);
    return -1;
  }
  return 0;
}

static int ia_iowow_next(iacontext *ctx, iabenchmark step, iakv *kv) {
  int rci = 0;
  iwrc rc = 0;
  iaprivate *self = ioarena.driver->priv;
  IWDB db = self->iwdb;
  IWKV_val k, v;
  bool sync = (ioarena.conf.syncmode == IA_SYNC);

  switch (step) {
  case IA_SET:
    k.data = kv->k;
    k.size = kv->ksize;
    v.data = kv->v;
    v.size = kv->vsize;
    rc = iwkv_put(db, &k, &v, sync ? IWKV_SYNC : 0);
    RCGO(rc, finish);
    break;

  case IA_DELETE:
    k.data = kv->k;
    k.size = kv->ksize;
    rc = iwkv_del(db, &k, sync ? IWKV_SYNC : 0);
    if (rc == IWKV_ERROR_NOTFOUND) {
      rci = ENOENT;
      rc = 0;
    }
    RCGO(rc, finish);
    break;

  case IA_GET:
    k.data = kv->k;
    k.size = kv->ksize;
    rc = iwkv_get(db, &k, &v);
    if (rc == IWKV_ERROR_NOTFOUND) {
      rc = 0;
      rci = ENOENT;
    } else {
      RCGO(rc, finish);
      kv->vsize = v.size;
      iwkv_val_dispose(&v);
    }
    break;

  case IA_ITERATE:
    rc = iwkv_cursor_to(ctx->cursor, IWKV_CURSOR_NEXT);
    if (rc == IWKV_ERROR_NOTFOUND) {
      kv->k = NULL;
      kv->ksize = 0;
      kv->v = NULL;
      kv->vsize = 0;
      rc = 0;
      rci = ENOENT;
    } else if (rc) {
      goto finish;
    } else {
      rc = iwkv_cursor_get(ctx->cursor, &k, &v);
      RCGO(rc, finish);
      kv->ksize = k.size;
      kv->vsize = v.size;
      iwkv_kv_dispose(&k, &v);
    }
    break;

  default:
    assert(0);
    rc = IW_ERROR_FAIL;
  }
finish:
  if (rc) {
    iwlog_ecode_error(rc, "Next benchmark step: %d failed", step);
    return -1;
  }
  return rci;
}

iadriver ia_iowow = {.name = "iowow",
                     .priv = NULL,
                     .open = ia_iowow_open,
                     .close = ia_iowow_close,
                     .thread_new = ia_iowow_thread_new,
                     .thread_dispose = ia_iowow_thread_dispose,
                     .begin = ia_iowow_begin,
                     .next = ia_iowow_next,
                     .done = ia_iowow_done};