
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

#include <ioarena.h>
#include <forestdb.h>

typedef struct {
	void *env;
	void *db;
} iaforestdb;

static int ia_forestdb_open(void)
{
	iadriver *self = ioarena.driver;
	self->priv = malloc(sizeof(iaforestdb));
	if (self->priv == NULL)
		return -1;
	mkdir(ioarena.conf.path, 0755);
	char path[1024];
	snprintf(path, sizeof(path), "%s/%s",
	         ioarena.conf.path, self->name);
	iaforestdb *s = self->priv;
	(void)s;
#if 0
	s->env = sp_env();
	if (s->env == NULL)
		return -1;
	sp_setstring(s->env, "forestdb.path", path, 0);
	sp_setstring(s->env, "scheduler.on_recover",
	             (void*)(uintptr_t)ia_forestdb_on_recover, 0);
	sp_setstring(s->env, "db", "test", 0);
	sp_setint(s->env, "db.test.mmap", 1);
	int rc = sp_open(s->env);
	if (rc == -1) {
		char *e = sp_getstring(s->env, "forestdb.error", 0);
		ia_log("error: %s", e);
		free(e);
		return -1;
	}
	s->db = sp_getobject(s->env, "db.test");
	assert(s->db != NULL);

	void *cur = sp_getobject(s->env, NULL);
	void *o = NULL;
	while ((o = sp_get(cur, o))) {
		char *key = sp_getstring(o, "key", 0);
		char *value = sp_getstring(o, "value", 0);
		ia_log("%s = %s", key, (value) ? value : "");
	}
	ia_log("");
#endif
	return 0;
}

static int ia_forestdb_close(void)
{
	iadriver *self = ioarena.driver;
	if (self->priv == NULL)
		return 0;
	iaforestdb *s = self->priv;
	(void)s;
#if 0
	int rc = 0;
	if (s->env)
		rc = sp_destroy(s->env);
	free(s);
#endif
	return 0;
}

static inline int
ia_forestdb_set(void)
{
	iadriver *self = ioarena.driver;
	iaforestdb *s = self->priv;
	(void)s;
	uint64_t i = 0;
	while (i < ioarena.conf.count)
	{
		ia_kv(&ioarena.kv);
		double t0 = ia_histogram_time();
#if 0
		void *o = sp_object(s->db);
		sp_setstring(o, "key",   ioarena.kv.k, ioarena.kv.ksize);
		sp_setstring(o, "value", ioarena.kv.v, ioarena.kv.vsize);
		int rc = sp_set(s->db, o);
		if (rc == -1) {
			char *e = sp_getstring(s->env, "forestdb.error", 0);
			ia_log("error: %s", e);
			free(e);
			return -1;
		}
#endif
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
ia_forestdb_delete(void)
{
	iadriver *self = ioarena.driver;
	iaforestdb *s = self->priv;
	(void)s;
	uint64_t i = 0;
	while (i < ioarena.conf.count)
	{
		ia_kv(&ioarena.kv);
		double t0 = ia_histogram_time();
#if 0
		void *o = sp_object(s->db);
		sp_setstring(o, "key",   ioarena.kv.k, ioarena.kv.ksize);
		int rc = sp_delete(s->db, o);
		if (rc == -1) {
			char *e = sp_getstring(s->env, "forestdb.error", 0);
			ia_log("error: %s", e);
			free(e);
			return -1;
		}
#endif
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
ia_forestdb_get(void)
{
	iadriver *self = ioarena.driver;
	iaforestdb *s = self->priv;
	(void)s;
	uint64_t i = 0;
	while (i < ioarena.conf.count)
	{
		ia_kv(&ioarena.kv);
		double t0 = ia_histogram_time();
#if 0
		void *o = sp_object(s->db);
		sp_setstring(o, "key", ioarena.kv.k, ioarena.kv.ksize);
		o = sp_get(s->db, o);
		if (o == NULL) {
			ia_log("error: key %s not found", ioarena.kv.k);
			char *e = sp_getstring(s->env, "forestdb.error", 0);
			if (e) {
				ia_log("error: %s", e);
				free(e);
			}
			return -1;
		}
#endif
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
ia_forestdb_iterate(void)
{
	iadriver *self = ioarena.driver;
	iaforestdb *s = self->priv;
	(void)s;
#if 0
	void *cursor = sp_cursor(s->env);
	if (cursor == NULL) {
		char *e = sp_getstring(s->env, "forestdb.error", 0);
		if (e) {
			ia_log("error: %s", e);
			free(e);
		}
		return -1;
	}
	uint64_t i = 0;
	void *o = sp_object(s->db);
	for (;;) {
		double t0 = ia_histogram_time();
		o = sp_get(cursor, o);
		double t1 = ia_histogram_time();
		double td = t1 - t0;
		ia_histogram_add(&ioarena.hg, td);
		if (o == NULL)
			break;
		ia_histogram_done(&ioarena.hg, i, ioarena.kv.ksize,
		                  ioarena.kv.vsize);
		i++;
	}
	sp_destroy(cursor);
#endif
	return 0;
}

static inline int
ia_forestdb_batch(void)
{
	iadriver *self = ioarena.driver;
	iaforestdb *s = self->priv;
	(void)s;
#if 0
	uint64_t i = 0;
	while (i < ioarena.conf.count)
	{
		void *batch = sp_batch(s->db);
		if (batch == NULL)
			goto error;
		int j = 0;
		while (j < 500 && (i < ioarena.conf.count))
		{
			ia_kv(&ioarena.kv);
			double t0 = ia_histogram_time();
			void *o = sp_object(s->db);
			sp_setstring(o, "key",   ioarena.kv.k, ioarena.kv.ksize);
			sp_setstring(o, "value", ioarena.kv.v, ioarena.kv.vsize);
			int rc = sp_set(batch, o);
			if (rc == -1)
				goto error;
			double t1 = ia_histogram_time();
			double td = t1 - t0;
			ia_histogram_add(&ioarena.hg, td);
			ia_histogram_done(&ioarena.hg, i, ioarena.kv.ksize,
			                  ioarena.kv.vsize);
			j++;
			i++;
		}
		int rc = sp_commit(batch);
		if (rc == -1)
			goto error;
		assert(rc == 0);
	}
#endif
	return 0;
}

static inline int
ia_forestdb_transaction(void)
{
	iadriver *self = ioarena.driver;
	iaforestdb *s = self->priv;
	(void)s;
#if 0
	uint64_t i = 0;
	while (i < ioarena.conf.count)
	{
		void *batch = sp_begin(s->env);
		if (batch == NULL)
			goto error;
		int j = 0;
		while (j < 500 && (i < ioarena.conf.count))
		{
			ia_kv(&ioarena.kv);
			double t0 = ia_histogram_time();
			void *o = sp_object(s->db);
			sp_setstring(o, "key",   ioarena.kv.k, ioarena.kv.ksize);
			sp_setstring(o, "value", ioarena.kv.v, ioarena.kv.vsize);
			int rc = sp_set(batch, o);
			if (rc == -1)
				goto error;
			double t1 = ia_histogram_time();
			double td = t1 - t0;
			ia_histogram_add(&ioarena.hg, td);
			ia_histogram_done(&ioarena.hg, i, ioarena.kv.ksize,
			                  ioarena.kv.vsize);
			j++;
			i++;
		}
		int rc = sp_commit(batch);
		if (rc == -1)
			goto error;
		assert(rc == 0);
	}
#endif
	return 0;
}

static int ia_forestdb_run(iabenchmark bench)
{
	switch (bench) {
	case IA_SET:         return ia_forestdb_set();
	case IA_GET:         return ia_forestdb_get();
	case IA_DELETE:      return ia_forestdb_delete();
	case IA_ITERATE:     return ia_forestdb_iterate();
	case IA_BATCH:       return ia_forestdb_batch();
	case IA_TRANSACTION: return ia_forestdb_transaction();
	default: assert(0);
	}
	return -1;
}

iadriver ia_forestdb =
{
	.name  = "forestdb",
	.priv  = NULL,
	.open  = ia_forestdb_open,
	.close = ia_forestdb_close,
	.run   = ia_forestdb_run
};
