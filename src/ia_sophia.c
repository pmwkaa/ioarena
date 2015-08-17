
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

#include <ioarena.h>
#include <sophia.h>

typedef struct {
	void *env;
	void *db;
} iasophia;

static void
ia_sophia_on_recover(char *trace, void *arg)
{
	ia_log("sophia: %s", trace);
	(void)arg;
}

static int ia_sophia_open(void)
{
	iadriver *self = ioarena.driver;
	self->priv = malloc(sizeof(iasophia));
	if (self->priv == NULL)
		return -1;
	mkdir(ioarena.conf.path, 0755);
	char path[1024];
	snprintf(path, sizeof(path), "%s/%s",
	         ioarena.conf.path, self->name);
	iasophia *s = self->priv;
	s->env = sp_env();
	if (s->env == NULL)
		return -1;
	sp_setstring(s->env, "sophia.path", path, 0);
	sp_setstring(s->env, "scheduler.on_recover",
	             (void*)(uintptr_t)ia_sophia_on_recover, 0);
	sp_setstring(s->env, "db", "test", 0);
	sp_setint(s->env, "db.test.mmap", 1);
	int rc = sp_open(s->env);
	if (rc == -1) {
		char *e = sp_getstring(s->env, "sophia.error", 0);
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
	return 0;
}

static int ia_sophia_close(void)
{
	iadriver *self = ioarena.driver;
	if (self->priv == NULL)
		return 0;
	iasophia *s = self->priv;
	int rc = 0;
	if (s->env)
		rc = sp_destroy(s->env);
	free(s);
	return rc;
}

static inline int
ia_sophia_set(void)
{
	iadriver *self = ioarena.driver;
	iasophia *s = self->priv;
	uint64_t i = 0;
	while (i < ioarena.conf.count)
	{
		ia_kv(&ioarena.kv);
		double t0 = ia_histogram_time();
		void *o = sp_object(s->db);
		sp_setstring(o, "key",   ioarena.kv.k, ioarena.kv.ksize);
		sp_setstring(o, "value", ioarena.kv.v, ioarena.kv.vsize);
		int rc = sp_set(s->db, o);
		if (rc == -1) {
			char *e = sp_getstring(s->env, "sophia.error", 0);
			ia_log("error: %s", e);
			free(e);
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
ia_sophia_delete(void)
{
	iadriver *self = ioarena.driver;
	iasophia *s = self->priv;
	uint64_t i = 0;
	while (i < ioarena.conf.count)
	{
		ia_kv(&ioarena.kv);
		double t0 = ia_histogram_time();
		void *o = sp_object(s->db);
		sp_setstring(o, "key",   ioarena.kv.k, ioarena.kv.ksize);
		int rc = sp_delete(s->db, o);
		if (rc == -1) {
			char *e = sp_getstring(s->env, "sophia.error", 0);
			ia_log("error: %s", e);
			free(e);
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
ia_sophia_get(void)
{
	iadriver *self = ioarena.driver;
	iasophia *s = self->priv;
	uint64_t i = 0;
	while (i < ioarena.conf.count)
	{
		ia_kv(&ioarena.kv);
		double t0 = ia_histogram_time();
		void *o = sp_object(s->db);
		sp_setstring(o, "key", ioarena.kv.k, ioarena.kv.ksize);
		o = sp_get(s->db, o);
		if (o == NULL) {
			ia_log("error: key %s not found", ioarena.kv.k);
			char *e = sp_getstring(s->env, "sophia.error", 0);
			if (e) {
				ia_log("error: %s", e);
				free(e);
			}
			return -1;
		}
		double t1 = ia_histogram_time();
		double td = t1 - t0;
		ia_histogram_add(&ioarena.hg, td);
		sp_destroy(o);
		ia_histogram_done(&ioarena.hg, i, ioarena.kv.ksize,
		                  ioarena.kv.vsize);
		i++;
	}
	return 0;
}

static inline int
ia_sophia_iterate(void)
{
	iadriver *self = ioarena.driver;
	iasophia *s = self->priv;
	void *cursor = sp_cursor(s->env);
	if (cursor == NULL) {
		char *e = sp_getstring(s->env, "sophia.error", 0);
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
	return 0;
}

static inline int
ia_sophia_batch(void)
{
	iadriver *self = ioarena.driver;
	iasophia *s = self->priv;
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
	return 0;
error:;
	char *e = sp_getstring(s->env, "sophia.error", 0);
	ia_log("error: %s", e);
	free(e);
	return -1;
}

static inline int
ia_sophia_transaction(void)
{
	iadriver *self = ioarena.driver;
	iasophia *s = self->priv;
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
	return 0;
error:;
	char *e = sp_getstring(s->env, "sophia.error", 0);
	ia_log("error: %s", e);
	free(e);
	return -1;
}

static int ia_sophia_run(iabenchmark bench)
{
	switch (bench) {
	case IA_SET:         return ia_sophia_set();
	case IA_GET:         return ia_sophia_get();
	case IA_DELETE:      return ia_sophia_delete();
	case IA_ITERATE:     return ia_sophia_iterate();
	case IA_BATCH:       return ia_sophia_batch();
	case IA_TRANSACTION: return ia_sophia_transaction();
	default: assert(0);
	}
	return -1;
}

iadriver ia_sophia =
{
	.name  = "sophia",
	.priv  = NULL,
	.open  = ia_sophia_open,
	.close = ia_sophia_close,
	.run   = ia_sophia_run
};
