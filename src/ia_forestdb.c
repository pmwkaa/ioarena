
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

#include <ioarena.h>
#include <forestdb.h>

typedef struct {
	fdb_file_handle *dbfile;
	fdb_kvs_handle *fdb;
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

	fdb_config config;
	memset(&config, 0, sizeof(fdb_config));
	config = fdb_get_default_config();
	config.num_compactor_threads   = 4;
	config.compress_document_body  = false;
	config.wal_threshold           = 4096;
	config.num_wal_partitions      = 31;
	config.num_bcache_partitions   = 31;
	config.seqtree_opt             = FDB_SEQTREE_NOT_USE;
	config.durability_opt          = FDB_DRB_ASYNC;

	fdb_kvs_config kvs_config;
	kvs_config = fdb_get_default_kvs_config();

	mkdir(path, 0755);
	snprintf(path, sizeof(path), "%s/%s/test",
	         ioarena.conf.path, self->name);
	creat(path, 0666);

	fdb_status status;
	status = fdb_open(&s->dbfile, path, &config);
	if (status != FDB_RESULT_SUCCESS) {
		ia_log("error: fdb_open(): %d", status);
		return -1;
	}
	status = fdb_kvs_open_default(s->dbfile, &s->fdb, &kvs_config);
	if (status != FDB_RESULT_SUCCESS) {
		ia_log("error: fdb_kvs_open_default(): %d", status);
		return -1;
	}
	return 0;
}

static int ia_forestdb_close(void)
{
	iadriver *self = ioarena.driver;
	if (self->priv == NULL)
		return 0;
	iaforestdb *s = self->priv;
	if (s->dbfile)
		fdb_close(s->dbfile);
	free(s);
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
		fdb_status status;
		status = fdb_set_kv(s->fdb,
		                    ioarena.kv.k, ioarena.kv.ksize,
		                    ioarena.kv.v, ioarena.kv.vsize);
		if (status != FDB_RESULT_SUCCESS) {
			ia_log("error: fdb_set(): %d", status);
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
	return 0;
}

static inline int
ia_forestdb_batch(void)
{
	iadriver *self = ioarena.driver;
	iaforestdb *s = self->priv;
	(void)s;
	return 0;
}

static inline int
ia_forestdb_transaction(void)
{
	iadriver *self = ioarena.driver;
	iaforestdb *s = self->priv;
	(void)s;
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
