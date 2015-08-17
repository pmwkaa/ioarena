
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

#include <ioarena.h>
#include <leveldb/c.h>

typedef struct {
	leveldb_options_t *opts;
	leveldb_readoptions_t *ropts;
	leveldb_writeoptions_t *wopts;
	leveldb_t *db;
} ialeveldb;

static int ia_leveldb_open(void)
{
	iadriver *self = ioarena.driver;
	self->priv = malloc(sizeof(ialeveldb));
	if (self->priv == NULL)
		return -1;
	mkdir(ioarena.conf.path, 0755);
	char path[1024];
	snprintf(path, sizeof(path), "%s/%s",
	         ioarena.conf.path, self->name);
	ialeveldb *s = self->priv;
	s->opts = leveldb_options_create();
	leveldb_options_set_compression(s->opts, leveldb_no_compression);
	leveldb_options_set_info_log(s->opts, NULL);
	leveldb_options_set_create_if_missing(s->opts, 1);
	s->wopts = leveldb_writeoptions_create();
	s->ropts = leveldb_readoptions_create();
	leveldb_writeoptions_set_sync(s->wopts, 0);
	leveldb_readoptions_set_fill_cache(s->ropts, 1);
	char *error = NULL;
	s->db = leveldb_open(s->opts, path, &error);
	if (error != NULL) {
		ia_log("error: %s", error);
		free(error);
		return -1;
	}
	return 0;
}

static int ia_leveldb_close(void)
{
	iadriver *self = ioarena.driver;
	if (self->priv == NULL)
		return 0;
	ialeveldb *s = self->priv;
	if (s->db)
		leveldb_close(s->db);
	if (s->ropts)
		leveldb_readoptions_destroy(s->ropts);
	if (s->wopts)
		leveldb_writeoptions_destroy(s->wopts);
	if (s->opts)
		leveldb_options_destroy(s->opts);
	free(s);
	return 0;
}

static inline int
ia_leveldb_set(void)
{
	iadriver *self = ioarena.driver;
	ialeveldb *s = self->priv;
	uint64_t i = 0;
	while (i < ioarena.conf.count)
	{
		ia_kv(&ioarena.kv);
		double t0 = ia_histogram_time();
		char *error = NULL;
		leveldb_put(s->db, s->wopts,
		            ioarena.kv.k, ioarena.kv.ksize, 
		            ioarena.kv.v, ioarena.kv.vsize, &error);
		if (error != NULL) {
			ia_log("error: %s", error);
			free(error);
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
ia_leveldb_delete(void)
{
	iadriver *self = ioarena.driver;
	ialeveldb *s = self->priv;
	uint64_t i = 0;
	while (i < ioarena.conf.count)
	{
		ia_kv(&ioarena.kv);
		double t0 = ia_histogram_time();
		char *error = NULL;
		leveldb_delete(s->db, s->wopts, ioarena.kv.k, ioarena.kv.ksize, &error);
		if (error != NULL) {
			ia_log("error: %s", error);
			free(error);
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
ia_leveldb_get(void)
{
	iadriver *self = ioarena.driver;
	ialeveldb *s = self->priv;
	uint64_t i = 0;
	while (i < ioarena.conf.count)
	{
		ia_kv(&ioarena.kv);
		double t0 = ia_histogram_time();
		size_t vsize = 0;
		char *error = NULL;
		char *p = leveldb_get(s->db, s->ropts, ioarena.kv.k,
		                      ioarena.kv.ksize, &vsize, &error);
		if (error != NULL) {
			ia_log("error: %s", error);
			free(error);
			return -1;
		}
		if (p == NULL) {
			ia_log("error: key %s not found", ioarena.kv.k);
			return -1;
		}
		double t1 = ia_histogram_time();
		double td = t1 - t0;
		ia_histogram_add(&ioarena.hg, td);
		free(p);
		ia_histogram_done(&ioarena.hg, i, ioarena.kv.ksize,
		                  ioarena.kv.vsize);
		i++;
	}
	return 0;
}

static inline int
ia_leveldb_iterate(void)
{
	iadriver *self = ioarena.driver;
	ialeveldb *s = self->priv;
	uint64_t i = 0;
	leveldb_iterator_t *it = leveldb_create_iterator(s->db, s->ropts);
	leveldb_iter_seek_to_first(it);
	while (leveldb_iter_valid(it)) {
		double t0 = ia_histogram_time();
		size_t sz = 0;
		const char *k = leveldb_iter_key(it, &sz);
		(void)k;
		leveldb_iter_next(it);
		double t1 = ia_histogram_time();
		double td = t1 - t0;
		ia_histogram_add(&ioarena.hg, td);
		ia_histogram_done(&ioarena.hg, i, ioarena.kv.ksize,
		                  ioarena.kv.vsize);
		i++;
	}
	leveldb_iter_destroy(it);
	return 0;
}

static inline int
ia_leveldb_batch(void)
{
	iadriver *self = ioarena.driver;
	ialeveldb *s = self->priv;
	char *error = NULL;
	leveldb_writebatch_t *batch = leveldb_writebatch_create();
	if (batch == NULL) {
		ia_log("error: %s", error);
		free(error);
		return -1;
	}
	uint64_t i = 0;
	while (i < ioarena.conf.count)
	{
		leveldb_writebatch_clear(batch);

		int j = 0;
		while (j < 500 && (i < ioarena.conf.count))
		{
			ia_kv(&ioarena.kv);
			double t0 = ia_histogram_time();
			leveldb_writebatch_put(batch,
			                       ioarena.kv.k, ioarena.kv.ksize,
			                       ioarena.kv.v, ioarena.kv.vsize);
			double t1 = ia_histogram_time();
			double td = t1 - t0;
			ia_histogram_add(&ioarena.hg, td);
			ia_histogram_done(&ioarena.hg, i, ioarena.kv.ksize,
			                  ioarena.kv.vsize);
			j++;
			i++;
		}
		error = NULL;
		leveldb_write(s->db, s->wopts, batch, &error);
		if (error != NULL) {
			leveldb_writebatch_destroy(batch);
			ia_log("error: %s", error);
			free(error);
			return -1;
		}
	}
	leveldb_writebatch_destroy(batch);
	return 0;
}

static inline int
ia_leveldb_transaction(void)
{
	iadriver *self = ioarena.driver;
	ialeveldb *s = self->priv;
	(void)s;
	ia_log("error: not supported");
	return -1;
}

static int ia_leveldb_run(iabenchmark bench)
{
	switch (bench) {
	case IA_SET:         return ia_leveldb_set();
	case IA_GET:         return ia_leveldb_get();
	case IA_DELETE:      return ia_leveldb_delete();
	case IA_ITERATE:     return ia_leveldb_iterate();
	case IA_BATCH:       return ia_leveldb_batch();
	case IA_TRANSACTION: return ia_leveldb_transaction();
	default: assert(0);
	}
	return -1;
}

iadriver ia_leveldb =
{
	.name  = "leveldb",
	.priv  = NULL,
	.open  = ia_leveldb_open,
	.close = ia_leveldb_close,
	.run   = ia_leveldb_run
};
