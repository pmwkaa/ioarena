
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

#include <ioarena.h>
#include <wiredtiger.h>

typedef struct {
	WT_CONNECTION *conn;
	WT_SESSION *session;
	WT_CURSOR *cursor;
} iawt;

static int ia_wt_open(void)
{
	iadriver *self = ioarena.driver;
	self->priv = malloc(sizeof(iawt));
	if (self->priv == NULL)
		return -1;
	char path[1024];
	snprintf(path, sizeof(path), "%s/%s",
	         ioarena.conf.path, self->name);
	mkdir(ioarena.conf.path, 0755);
	mkdir(path, 0755);
	iawt *s = self->priv;

	char config[256];
	snprintf(config, sizeof(config), "create, log=(enabled)");

	int rc = wiredtiger_open(path, NULL, config, &s->conn);
	if (rc != 0) {
		ia_log("error: wirtedtiger_open(): %s", wiredtiger_strerror(rc));
		return -1;
	}
        snprintf(config, sizeof(config),
                "split_pct=100,leaf_item_max=1KB,"
                "type=lsm,internal_page_max=4KB,leaf_page_max=4KB");
	rc = s->conn->open_session(s->conn, NULL, NULL, &s->session);
	if (rc != 0) {
		ia_log("error: open_session(): %s", wiredtiger_strerror(rc));
		return -1;
	}
	s->session->create(s->session, "table:test", config);
	s->session->open_cursor(s->session, "table:test", NULL, NULL, &s->cursor);
	return 0;
}

static int ia_wt_close(void)
{
	iadriver *self = ioarena.driver;
	if (self->priv == NULL)
		return 0;
	iawt *s = self->priv;
	s->cursor->close(s->cursor);
	s->session->close(s->session, NULL);
	free(s);
	return 0;
}

static inline int
ia_wt_set(void)
{
	iadriver *self = ioarena.driver;
	iawt *s = self->priv;
	(void)s;
	uint64_t i = 0;
	while (i < ioarena.conf.count)
	{
		ia_kv(&ioarena.kv);
		double t0 = ia_histogram_time();
		WT_ITEM k, v;
		k.data = ioarena.kv.k;
		k.size = ioarena.kv.ksize;
		v.data = ioarena.kv.v;
		v.size = ioarena.kv.vsize;
		s->cursor->set_key(s->cursor, &k);
		s->cursor->set_value(s->cursor, &v);
		int rc = s->cursor->insert(s->cursor);
		if (rc != 0) {
			ia_log("error: insert(): %s", wiredtiger_strerror(rc));
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
ia_wt_delete(void)
{
	iadriver *self = ioarena.driver;
	iawt *s = self->priv;
	uint64_t i = 0;
	while (i < ioarena.conf.count)
	{
		ia_kv(&ioarena.kv);
		double t0 = ia_histogram_time();
		WT_ITEM k;
		k.data = ioarena.kv.k;
		k.size = ioarena.kv.ksize;
		s->cursor->set_key(s->cursor, &k);
		int rc = s->cursor->remove(s->cursor);
		if (rc != 0) {
			ia_log("error: remove(): %s", wiredtiger_strerror(rc));
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
ia_wt_get(void)
{
	iadriver *self = ioarena.driver;
	iawt *s = self->priv;
	uint64_t i = 0;
	while (i < ioarena.conf.count)
	{
		ia_kv(&ioarena.kv);
		double t0 = ia_histogram_time();
		WT_ITEM k;
		k.data = ioarena.kv.k;
		k.size = ioarena.kv.ksize;
		s->cursor->set_key(s->cursor, &k);
		int rc = s->cursor->search(s->cursor);
		if (rc != 0) {
			ia_log("error: search(): %s", wiredtiger_strerror(rc));
			return -1;
		}
		WT_ITEM v;
		s->cursor->get_value(s->cursor, &v);
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
ia_wt_iterate(void)
{
	iadriver *self = ioarena.driver;
	iawt *s = self->priv;

	WT_ITEM k,v ;
	s->cursor->reset(s->cursor);

	int rc;
	uint64_t i = 0;
	while ((rc = s->cursor->next(s->cursor)) == 0 ) {
		double t0 = ia_histogram_time();
		rc = s->cursor->get_key(s->cursor, &k);
		if (rc != 0) {
			ia_log("error: %s", wiredtiger_strerror(rc));
			break;
		}
		rc = s->cursor->get_value(s->cursor, &v);
		if (rc != 0) {
			ia_log("error: %s", wiredtiger_strerror(rc));
			break;
		}
		double t1 = ia_histogram_time();
		double td = t1 - t0;
		ia_histogram_add(&ioarena.hg, td);
		ia_histogram_done(&ioarena.hg, i, ioarena.kv.ksize,
		                  ioarena.kv.vsize);
		i++;
	}
	ia_log("traversed: %"PRIu64, i);
	return 0;
}

static inline int
ia_wt_batch(void)
{
	ia_log("error: not supported");
	return -1;
}

static inline int
ia_wt_transaction(void)
{
	iadriver *self = ioarena.driver;
	iawt *s = self->priv;
	uint64_t i = 0;
	while (i < ioarena.conf.count)
	{
		int rc = s->session->begin_transaction(s->session, NULL);
		if (rc != 0) {
			ia_log("error: begin_transaction(): %s", wiredtiger_strerror(rc));
			return -1;
		}
		int j = 0;
		while (j < 500 && (i < ioarena.conf.count))
		{
			ia_kv(&ioarena.kv);
			double t0 = ia_histogram_time();
			WT_ITEM k, v;
			k.data = ioarena.kv.k;
			k.size = ioarena.kv.ksize;
			v.data = ioarena.kv.v;
			v.size = ioarena.kv.vsize;
			s->cursor->set_key(s->cursor, &k);
			s->cursor->set_value(s->cursor, &v);
			rc = s->cursor->insert(s->cursor);
			if (rc != 0) {
				ia_log("error: insert(): %s", wiredtiger_strerror(rc));
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
		rc = s->session->commit_transaction(s->session, NULL);
		if (rc != 0) {
			ia_log("error: begin_transaction(): %s", wiredtiger_strerror(rc));
			return -1;
		}
	}
	return 0;
}

static int ia_wt_run(iabenchmark bench)
{
	switch (bench) {
	case IA_SET:         return ia_wt_set();
	case IA_GET:         return ia_wt_get();
	case IA_DELETE:      return ia_wt_delete();
	case IA_ITERATE:     return ia_wt_iterate();
	case IA_BATCH:       return ia_wt_batch();
	case IA_TRANSACTION: return ia_wt_transaction();
	default: assert(0);
	}
	return -1;
}

iadriver ia_wt =
{
	.name  = "wiredtiger",
	.priv  = NULL,
	.open  = ia_wt_open,
	.close = ia_wt_close,
	.run   = ia_wt_run
};
