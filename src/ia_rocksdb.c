
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

#include <ioarena.h>
#include <rocksdb/c.h>

typedef struct {
	rocksdb_options_t *opts;
	rocksdb_readoptions_t *ropts;
	rocksdb_writeoptions_t *wopts;
	rocksdb_t *db;
} iarocksdb;

static int ia_rocksdb_open(void)
{
	iadriver *self = ioarena.driver;
	self->priv = malloc(sizeof(iarocksdb));
	if (self->priv == NULL)
		return -1;
	mkdir(ioarena.conf.path, 0755);
	char path[1024];
	snprintf(path, sizeof(path), "%s/%s",
	         ioarena.conf.path, self->name);
	iarocksdb *s = self->priv;
	s->opts = rocksdb_options_create();
	rocksdb_options_set_compression(s->opts, rocksdb_no_compression);
	rocksdb_options_set_info_log(s->opts, NULL);
	rocksdb_options_set_create_if_missing(s->opts, 1);
	s->wopts = rocksdb_writeoptions_create();
	s->ropts = rocksdb_readoptions_create();
	rocksdb_writeoptions_set_sync(s->wopts, 0);
	rocksdb_readoptions_set_fill_cache(s->ropts, 0);
	char *error = NULL;
	s->db = rocksdb_open(s->opts, path, &error);
	if (error != NULL) {
		ia_log("error: %s", error);
		free(error);
		return -1;
	}
	return 0;
}

static int ia_rocksdb_close(void)
{
	iadriver *self = ioarena.driver;
	if (self->priv == NULL)
		return 0;
	iarocksdb *s = self->priv;
	if (s->db)
		rocksdb_close(s->db);
	if (s->ropts)
		rocksdb_readoptions_destroy(s->ropts);
	if (s->wopts)
		rocksdb_writeoptions_destroy(s->wopts);
	if (s->opts)
		rocksdb_options_destroy(s->opts);
	free(s);
	return 0;
}

static inline int
ia_rocksdb_set(void)
{
	iadriver *self = ioarena.driver;
	iarocksdb *s = self->priv;
	uint64_t i = 0;
	while (i < ioarena.conf.count)
	{
		ia_kv(&ioarena.kv);
		double t0 = ia_histogram_time();
		char *error = NULL;
		rocksdb_put(s->db, s->wopts,
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
ia_rocksdb_delete(void)
{
	iadriver *self = ioarena.driver;
	iarocksdb *s = self->priv;
	uint64_t i = 0;
	while (i < ioarena.conf.count)
	{
		ia_kv(&ioarena.kv);
		double t0 = ia_histogram_time();
		char *error = NULL;
		rocksdb_delete(s->db, s->wopts, ioarena.kv.k, ioarena.kv.ksize, &error);
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
ia_rocksdb_get(void)
{
	iadriver *self = ioarena.driver;
	iarocksdb *s = self->priv;
	uint64_t i = 0;
	while (i < ioarena.conf.count)
	{
		ia_kv(&ioarena.kv);
		double t0 = ia_histogram_time();
		size_t vsize = 0;
		char *error = NULL;
		char *p = rocksdb_get(s->db, s->ropts, ioarena.kv.k,
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
ia_rocksdb_iterate(void)
{
	iadriver *self = ioarena.driver;
	iarocksdb *s = self->priv;
	uint64_t i = 0;
	rocksdb_iterator_t *it = rocksdb_create_iterator(s->db, s->ropts);
	rocksdb_iter_seek_to_first(it);
	while (rocksdb_iter_valid(it)) {
		double t0 = ia_histogram_time();
		size_t sz = 0;
		const char *k = rocksdb_iter_key(it, &sz);
		(void)k;
		rocksdb_iter_next(it);
		double t1 = ia_histogram_time();
		double td = t1 - t0;
		ia_histogram_add(&ioarena.hg, td);
		ia_histogram_done(&ioarena.hg, i, ioarena.kv.ksize,
		                  ioarena.kv.vsize);
		i++;
	}
	rocksdb_iter_destroy(it);
	return 0;
}

static inline int
ia_rocksdb_batch(void)
{
	iadriver *self = ioarena.driver;
	iarocksdb *s = self->priv;
	char *error = NULL;
	rocksdb_writebatch_t *batch = rocksdb_writebatch_create();
	if (batch == NULL) {
		ia_log("error: %s", error);
		free(error);
		return -1;
	}
	uint64_t i = 0;
	while (i < ioarena.conf.count)
	{
		rocksdb_writebatch_clear(batch);

		int j = 0;
		while (j < 500 && (i < ioarena.conf.count))
		{
			ia_kv(&ioarena.kv);
			double t0 = ia_histogram_time();
			rocksdb_writebatch_put(batch,
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
		rocksdb_write(s->db, s->wopts, batch, &error);
		if (error != NULL) {
			rocksdb_writebatch_destroy(batch);
			ia_log("error: %s", error);
			free(error);
			return -1;
		}
	}
	rocksdb_writebatch_destroy(batch);
	return 0;
}

static inline int
ia_rocksdb_transaction(void)
{
	iadriver *self = ioarena.driver;
	iarocksdb *s = self->priv;
	(void)s;
	ia_log("error: not supported");
	return -1;
}

static int ia_rocksdb_run(iabenchmark bench)
{
	switch (bench) {
	case IA_SET:         return ia_rocksdb_set();
	case IA_GET:         return ia_rocksdb_get();
	case IA_DELETE:      return ia_rocksdb_delete();
	case IA_ITERATE:     return ia_rocksdb_iterate();
	case IA_BATCH:       return ia_rocksdb_batch();
	case IA_TRANSACTION: return ia_rocksdb_transaction();
	default: assert(0);
	}
	return -1;
}

iadriver ia_rocksdb =
{
	.name  = "rocksdb",
	.priv  = NULL,
	.open  = ia_rocksdb_open,
	.close = ia_rocksdb_close,
	.run   = ia_rocksdb_run
};
