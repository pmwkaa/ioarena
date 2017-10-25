
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

#include <ioarena.h>
#include <ejdb.h>

//#define EJDB_DEBUG 1

struct iaprivate {
	EJDB *jb;   /* ejdb main handle */
	EJQ *jq;    /* ejdb query */
	EJCOLL *jc; /* ejdb collection */
};

__attribute__((unused))
static void dump_bson(const char *bsdata) {
	
	int rc = 0;
	int len = 0;
	char *buf = NULL;
	rc = bson2json(bsdata, &buf, &len);
	if(rc == BSON_OK) {
		ia_log("%s\n", buf);
	} else {
		ia_log("error convert bson2json\n");
	}
	if(buf != NULL) {
		free(buf);
	}
}

static int ia_ejdb_open(const char *datadir)
{	
	//int rc = 0;
	int open_mode = 0;
	const char *db_name = "/test.ejdb";
	const char *db_collection_name = "benchmark_col";
	EJCOLLOPTS db_collection_options;
  	int db_path_sz = strlen(datadir) + strlen(db_name) + 1;
  	char db_path[db_path_sz];
	memset(db_path, 0, db_path_sz); 
	strcat(db_path, datadir);
	strcat(db_path, db_name);

	iadriver *drv = ioarena.driver;

	drv->priv = calloc(1, sizeof(iaprivate));
	if (drv->priv == NULL)
		goto bailout;

	iaprivate *self = drv->priv;

	
	open_mode = JBOWRITER | JBOREADER | JBOCREAT;
	switch(ioarena.conf.syncmode) {
	case IA_SYNC:
		open_mode |= JBOTSYNC;
		break;
	case IA_LAZY:
		ia_log("error: %s(): not support syncmode %s", __func__, ia_syncmode2str(ioarena.conf.syncmode));
		goto bailout;
	case IA_NOSYNC:
		break;
	default:
		ia_log("error: %s(): unsupported syncmode %s",
			__func__, ia_syncmode2str(ioarena.conf.syncmode));
		goto bailout;
	}
	ia_log("%s(): walmode\n"
		" walmod option in EJDB implementation.\n"
		"You could use this option for choose flags JBONOLCK and JBOLCKNB\n"
		/*"indef  -> no JBO* locking flags\n"*/
		"walon  -> add JBONOLCK flag -> EJDB Open without locking\n"
		"waloff -> add JBOLCKNB flag -> EJDB Lock without blocking\n"
		"indef  -> add both JBO* locking flags\n"
		, __func__);

	switch(ioarena.conf.walmode) {
	case IA_WAL_INDEF:
		open_mode |= JBONOLCK;
		open_mode |= JBOLCKNB;
		break;
	case IA_WAL_ON:
		open_mode |= JBONOLCK;
		break;
	case IA_WAL_OFF:
		open_mode |= JBOLCKNB;
		break;
	default:
		ia_log("error: %s(): unsupported walmode %s",
			__func__, ia_walmode2str(ioarena.conf.walmode));
		goto bailout;
	}
	self->jb = ejdbnew();
	ia_log("let's open EJDB with db_path %s\n", db_path);
	if (!ejdbopen(self->jb, db_path, open_mode)) {
		ia_log("can't open EJDB with db_path %s [ open_mode = %d ]\n", db_path, open_mode);
		goto bailout;
	}

	db_collection_options.large         = false;
	db_collection_options.compressed    = false;
	db_collection_options.records       = ioarena.conf.keys;
	db_collection_options.cachedrecords = 0;
	ia_log("EJDB collection options:\n"
		"Large collection: %s. It can be larger than 2GB. Default false\n"
		"Deflate collection: %s. Collection records will be compressed with DEFLATE compression. Default: false\n"
		"Expect record number: %d. Expected records number in the collection. Default: 128K\n"
		"Cahce in memory records: %d. Maximum number of records cached in memory. Default: 0\n",
		db_collection_options.large ? "YES" : "NO",
		db_collection_options.compressed ? "YES" : "NO",
		(unsigned int)db_collection_options.records,
		(unsigned int)db_collection_options.cachedrecords
		);
	
	ia_log("let's create / open collection for openned EJDB  %s\n", db_collection_name);
	self->jc = ejdbcreatecoll(self->jb, db_collection_name, &db_collection_options);
	if(self->jc == NULL) {
		ia_log("can't create collection for openned EJDB  %s\n", db_collection_name);
		goto bailout;
	}

	return 0;
bailout:
	ia_log("error: %s", __func__);
	return -1;
}


static int ia_ejdb_close(void)
{
	int rc = 0;
	(void)rc;
	iaprivate *self = ioarena.driver->priv;
	if (self) {
	 	rc = (ejdbsetindex(self->jc, "key", JBIDXSTR | JBIDXREBLD) ? 0 : -1);
#ifdef EJDB_DEBUG
		ia_log("in %s(): rebuild index %s\n", __func__, (rc == 0 ? "OK" : "FAILED"));
#endif
		ejdbclose(self->jb);
		ioarena.driver->priv = NULL;
		free(self);
	}
	return 0;
}

static iacontext* ia_ejdb_thread_new(void)
{
	iacontext* ctx = calloc(1, sizeof(ctx));
	return ctx;
}

void ia_ejdb_thread_dispose(iacontext *ctx)
{
	free(ctx);
}

static int ia_ejdb_begin(iacontext *ctx, iabenchmark step)
{
	(void) ctx;
	int rc;

	iaprivate *self = ioarena.driver->priv;

	switch(step) {
	case IA_BATCH:
	case IA_CRUD:
		rc = (ejdbtranbegin(self->jc) ? 0 : -1);
		break;
	case IA_SET:
	case IA_DELETE:
		rc = 0;
		break;

	case IA_ITERATE:
	case IA_GET:
	
		rc = 0;
		break;

	default:
		assert(0);
		rc = -1;
	}

	return rc;
}

static int ia_ejdb_done(iacontext* ctx, iabenchmark step)
{
	int rc;

	(void) ctx;

	iaprivate *self = ioarena.driver->priv;

	switch(step) {
	case IA_BATCH:
	case IA_CRUD:
		rc = (ejdbtrancommit(self->jc) ? 0 : -1);
		break;
	case IA_SET:
	case IA_DELETE:
	case IA_ITERATE:
	case IA_GET:
		rc = 0;
		break;

	default:
		assert(0);
		rc = -1;
	}

	return rc;
}

static int ia_ejdb_next(iacontext* ctx, iabenchmark step, iakv *kv)
{
	bson bsrec;
	bson bq;
	bson_oid_t oid;
	uint32_t count;
	int rc = 0;
	bson_bool_t drop = 1;
	(void) ctx;
	char key[kv->ksize + 1];
	memcpy(key, kv->k, kv->ksize);
	key[kv->ksize] = 0;
	char value[kv->vsize + 1];
	memcpy(value, kv->v, kv->vsize);
	value[kv->vsize] = 0;
	iaprivate *self = ioarena.driver->priv;
	
//	ia_log("KEY = \"%s\", VALUE = \"%s\"\n", key, value);
	switch(step) {
	case IA_SET:
		bson_init(&bsrec);
		bson_append_string(&bsrec, "key",   key);
		bson_append_string(&bsrec, "value", value);
		bson_finish(&bsrec);
		rc = (ejdbsavebson(self->jc, &bsrec, &oid) ? 0 : -1);
		bson_destroy(&bsrec);
		break;
	case IA_DELETE:
		bson_init_as_query(&bq);
		bson_append_string(&bq, "key",   key);
		bson_append_bool(&bq, "$dropall", drop);
		bson_finish(&bq);
		count =  ejdbupdate(self->jc, &bq, 0, 0, 0, 0);
		//ia_log("drop count = %d\n", count);
		if(count != 1 ) {
			rc = -1;
		}
		bson_destroy(&bq);
		rc = 0;
		break;
	case IA_ITERATE:
		rc = 0;
		break;
	case IA_GET:
		count = 0;
		bson_init_as_query(&bq);
		bson_append_string(&bq, "key",   key);
		bson_finish(&bq);
		self->jq = ejdbcreatequery(self->jb, &bq, NULL, 0, NULL);
		if(self->jq == NULL) {
			rc = -1;
			break;
		}
		TCLIST *res = ejdbqryexecute(self->jc, self->jq, &count, 0, NULL);
		
#ifdef EJDB_DEBUG
		ia_log("Records found: %d\n", count);
		for (int i = 0; i < TCLISTNUM(res); ++i) {
			void *bsdata = TCLISTVALPTR(res, i);
			bson_print_raw(bsdata, 0);
			dump_bson(bsdata);
		}
		ia_log("\n");
#endif
		
		tclistdel(res);
		ejdbquerydel(self->jq);
		bson_destroy(&bq);
		rc = 0;
		break;
	default:
		assert(0);
		rc = -1;
	}

	return rc;
}

iadriver ia_ejdb =
{
	.name  = "ejdb",
	.priv  = NULL,
	.open  = ia_ejdb_open,
	.close = ia_ejdb_close,

	.thread_new = ia_ejdb_thread_new,
	.thread_dispose = ia_ejdb_thread_dispose,
	.begin	= ia_ejdb_begin,
	.next	= ia_ejdb_next,
	.done	= ia_ejdb_done
};
