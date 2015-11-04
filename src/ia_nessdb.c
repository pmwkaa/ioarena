
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

#include <ioarena.h>
#include <ness.h>

typedef struct {
	void *e;
	void *db;
} ianessdb;

static int ia_nessdb_open(void)
{
	iadriver *self = ioarena.driver;
	self->priv = malloc(sizeof(ianessdb));
	if (self->priv == NULL)
		return -1;
	mkdir(ioarena.conf.path, 0755);
	char path[1024];
	snprintf(path, sizeof(path), "%s/%s",
	         ioarena.conf.path, self->name);
	mkdir(path, 0755);
	ianessdb *s = self->priv;
	memset(s, 0, sizeof(ianessdb));

	char dbname[] = "test.nessdb";
	s->e = ness_env_open(path, -1);
	s->db = ness_db_open(s->e, dbname);

	if (!s->db) {
		fprintf(stderr, "open db error, see ness.event for details\n");
		return 0;
	}

	if (!ness_env_set_cache_size(s->e, 1024*1024*1024)) {
		fprintf(stderr, "set cache size error, see ness.event for details\n");
		return 0;
	}

	return 0;
}

static int ia_nessdb_close(void)
{
	iadriver *self = ioarena.driver;
	if (self->priv == NULL)
		return 0;
	ianessdb *s = self->priv;

	ness_db_close(s->db);
	ness_env_close(s->e);
	
	free(s);
	return 0;
}

static inline int
ia_nessdb_set(void)
{
	iadriver *self = ioarena.driver;
	ianessdb *s = self->priv;
	uint64_t i = 0;
	while (i < ioarena.conf.count)
	{
		ia_kv(&ioarena.kv);
		double t0 = ia_histogram_time();
		int rc = ness_db_set(s->db, ioarena.kv.k, ioarena.kv.ksize, ioarena.kv.v, ioarena.kv.vsize);
		if (rc != 1) {
			ia_log("error: nessdb_set(): %d", rc);
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
ia_nessdb_delete(void)
{
	ia_log("error: not supported");
	return -1;
}

static inline int
ia_nessdb_get(void)
{
	ia_log("error: not supported");
	return -1;
}

static inline int
ia_nessdb_iterate(void)
{
	ia_log("error: not supported");
	return -1;
}

static inline int
ia_nessdb_batch(void)
{
	ia_log("error: not supported");
	return -1;
}

static int ia_nessdb_run(iabenchmark bench)
{
	switch (bench) {
	case IA_SET:         return ia_nessdb_set();
	case IA_GET:         return ia_nessdb_get();
	case IA_DELETE:      return ia_nessdb_delete();
	case IA_ITERATE:     return ia_nessdb_iterate();
	case IA_BATCH:       return ia_nessdb_batch();
	default: assert(0);
	}
	return -1;
}

iadriver ia_nessdb =
{
	.name  = "nessdb",
	.priv  = NULL,
	.open  = ia_nessdb_open,
	.close = ia_nessdb_close,
	.run   = ia_nessdb_run
};
