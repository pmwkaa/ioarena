
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

#include <ioarena.h>
#include <lmdb.h>

typedef struct {
	MDB_env *env;
	MDB_txn *txn;
	MDB_dbi dbi;
} ialmdb;

static int ia_lmdb_open(void)
{
	iadriver *self = ioarena.driver;
	self->priv = malloc(sizeof(ialmdb));
	if (self->priv == NULL)
		return -1;
	mkdir(ioarena.conf.path, 0755);
	char path[1024];
	snprintf(path, sizeof(path), "%s/%s",
	         ioarena.conf.path, self->name);
	mkdir(path, 0755);
	ialmdb *s = self->priv;
	memset(s, 0, sizeof(ialmdb));
	int rc = mdb_env_create(&s->env);
	if (rc != MDB_SUCCESS) {
		ia_log("error: mdb_env_create(): %d", rc);
		return -1;
	}
	rc = mdb_env_set_mapsize(s->env, 600 * 1024 * 1024 * 1024ULL);
	if (rc != MDB_SUCCESS) {
		ia_log("error: mdb_env_set_mapsize(): %d", rc);
		return -1;
	}
	mkdir(path, 0755);
	rc = mdb_env_open(s->env, path,
	                  MDB_CREATE|MDB_NOSYNC|MDB_NOMETASYNC|MDB_NORDAHEAD, 0644);
	if (rc != MDB_SUCCESS) {
		ia_log("error: mdb_env_open(): %d", rc);
		return -1;
	}
	rc = mdb_txn_begin(s->env, NULL, 0, &s->txn);
	if (rc != MDB_SUCCESS) {
		ia_log("error: mdb_txn_begin(): %d", rc);
		return -1;
	}
	rc = mdb_open(s->txn, NULL, 0, &s->dbi);
	if (rc != MDB_SUCCESS) {
		ia_log("error: mdb_open(): %d\n", rc);
		return -1;
	}
	return 0;
}

static int ia_lmdb_close(void)
{
	iadriver *self = ioarena.driver;
	if (self->priv == NULL)
		return 0;
	ialmdb *s = self->priv;
	if (s->txn)
		mdb_txn_commit(s->txn);
	if (s->env) {
		mdb_close(s->env, s->dbi);
		mdb_env_close(s->env);
	}
	free(s);
	return 0;
}

static inline int
ia_lmdb_set(void)
{
	iadriver *self = ioarena.driver;
	ialmdb *s = self->priv;
	uint64_t i = 0;
	while (i < ioarena.conf.count)
	{
		ia_kv(&ioarena.kv);
		double t0 = ia_histogram_time();
		MDB_val k, v;
		k.mv_data = ioarena.kv.k;
		k.mv_size = ioarena.kv.ksize;
		v.mv_data = ioarena.kv.v;
		v.mv_size = ioarena.kv.vsize;
		int rc = mdb_put(s->txn, s->dbi, &k, &v, 0);
		if (rc) {
			ia_log("error: mdb_put(): %d", rc);
			return -1;
		}
		double t1 = ia_histogram_time();
		double td = t1 - t0;
		ia_histogram_add(&ioarena.hg, td);
		ia_histogram_done(&ioarena.hg, i, ioarena.kv.ksize,
		                  ioarena.kv.vsize);
		i++;
	}
	return 0;
}

static inline int
ia_lmdb_delete(void)
{
	iadriver *self = ioarena.driver;
	ialmdb *s = self->priv;
	uint64_t i = 0;
	while (i < ioarena.conf.count)
	{
		ia_kv(&ioarena.kv);
		double t0 = ia_histogram_time();
		MDB_val k;
		k.mv_data = ioarena.kv.k;
		k.mv_size = ioarena.kv.ksize;
		int rc = mdb_del(s->txn, s->dbi, &k, 0);
		if (rc) {
			ia_log("error: mdb_del(): %d", rc);
			return -1;
		}
		double t1 = ia_histogram_time();
		double td = t1 - t0;
		ia_histogram_add(&ioarena.hg, td);
		ia_histogram_done(&ioarena.hg, i, ioarena.kv.ksize,
		                  ioarena.kv.vsize);
		i++;
	}
	return 0;
}

static inline int
ia_lmdb_get(void)
{
	iadriver *self = ioarena.driver;
	ialmdb *s = self->priv;
	uint64_t i = 0;
	while (i < ioarena.conf.count)
	{
		ia_kv(&ioarena.kv);
		double t0 = ia_histogram_time();
		MDB_val k, v;
		k.mv_data = ioarena.kv.k;
		k.mv_size = ioarena.kv.ksize;
		int rc = mdb_get(s->txn, s->dbi, &k, &v);
		if (rc) {
			ia_log("error: mdb_get(): %d", rc);
			return -1;
		}
		double t1 = ia_histogram_time();
		double td = t1 - t0;
		ia_histogram_add(&ioarena.hg, td);
		ia_histogram_done(&ioarena.hg, i, ioarena.kv.ksize,
		                  ioarena.kv.vsize);
		i++;
	}
	return 0;
}

static inline int
ia_lmdb_iterate(void)
{
	iadriver *self = ioarena.driver;
	ialmdb *s = self->priv;
	MDB_cursor *cursor;
	mdb_cursor_open(s->txn, s->dbi, &cursor);
	MDB_val k, v;
	uint64_t i = 0;
	for (;;) {
		double t0 = ia_histogram_time();
		int rc = mdb_cursor_get(cursor, &k, &v, MDB_NEXT);
		double t1 = ia_histogram_time();
		double td = t1 - t0;
		if (rc != 0)
			break;
		ia_histogram_add(&ioarena.hg, td);
		ia_histogram_done(&ioarena.hg, i, ioarena.kv.ksize,
		                  ioarena.kv.vsize);
		i++;
	}
	mdb_cursor_close(cursor);
	return 0;
}

static inline int
ia_lmdb_batch(void)
{
	ia_log("error: not supported");
	return -1;
}

static inline int
ia_lmdb_transaction(void)
{
	iadriver *self = ioarena.driver;
	ialmdb *s = self->priv;

	mdb_close(s->env, s->dbi);
	mdb_txn_commit(s->txn);
	int rc = mdb_txn_begin(s->env, NULL, 0, &s->txn);
	if (rc != MDB_SUCCESS) {
		ia_log("error: mdb_txn_begin(): %d", rc);
		return -1;
	}
	rc = mdb_open(s->txn, NULL, 0, &s->dbi);
	if (rc != MDB_SUCCESS) {
		ia_log("error: mdb_open(): %d\n", rc);
		return -1;
	}

	uint64_t i = 0;
	while (i < ioarena.conf.count)
	{
		int j = 0;
		while (j < 500 && (i < ioarena.conf.count))
		{
			ia_kv(&ioarena.kv);
			double t0 = ia_histogram_time();
			MDB_val k, v;
			k.mv_data = ioarena.kv.k;
			k.mv_size = ioarena.kv.ksize;
			v.mv_data = ioarena.kv.v;
			v.mv_size = ioarena.kv.vsize;
			int rc = mdb_put(s->txn, s->dbi, &k, &v, 0);
			if (rc) {
				ia_log("error: mdb_put(): %d", rc);
				return -1;
			}
			double t1 = ia_histogram_time();
			double td = t1 - t0;
			ia_histogram_add(&ioarena.hg, td);
			ia_histogram_done(&ioarena.hg, i, ioarena.kv.ksize,
			                  ioarena.kv.vsize);
			j++;
			i++;
		}

		mdb_close(s->env, s->dbi);
		mdb_txn_commit(s->txn);
		int rc = mdb_txn_begin(s->env, NULL, 0, &s->txn);
		if (rc != MDB_SUCCESS) {
			ia_log("error: mdb_txn_begin(): %d", rc);
			return -1;
		}
		rc = mdb_open(s->txn, NULL, 0, &s->dbi);
		if (rc != MDB_SUCCESS) {
			ia_log("error: mdb_open(): %d\n", rc);
			return -1;
		}
	}
	return 0;
}

static int ia_lmdb_run(iabenchmark bench)
{
	switch (bench) {
	case IA_SET:         return ia_lmdb_set();
	case IA_GET:         return ia_lmdb_get();
	case IA_DELETE:      return ia_lmdb_delete();
	case IA_ITERATE:     return ia_lmdb_iterate();
	case IA_BATCH:       return ia_lmdb_batch();
	case IA_TRANSACTION: return ia_lmdb_transaction();
	default: assert(0);
	}
	return -1;
}

iadriver ia_lmdb =
{
	.name  = "lmdb",
	.priv  = NULL,
	.open  = ia_lmdb_open,
	.close = ia_lmdb_close,
	.run   = ia_lmdb_run
};
