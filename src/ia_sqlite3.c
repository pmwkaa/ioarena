/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

#include <ioarena.h>
#include <sqlite3.h>

struct iaprivate {
//	rocksdb_options_t *opts;
//	rocksdb_readoptions_t *ropts;
//	rocksdb_writeoptions_t *wopts;
	sqlite3 *db;
};

#define CMD_SIZE 1024

struct iacontext {
	//rocksdb_iterator_t *it;
	//rocksdb_writebatch_t *batch;
	//char* result;
	char cmd_buf[CMD_SIZE];
	int batch;
};

static int ia_sqlite3_open(const char *datadir)
{
	int rc;
	char *zErrMsg = 0;
	char cmd_buf[CMD_SIZE];
	const char *db_name = "/test.db";
	int db_path_sz = strlen(datadir) + strlen(db_name) + 1;
	char db_path[db_path_sz];
	iadriver *drv = ioarena.driver;
	drv->priv = malloc(sizeof(iaprivate));
	if (drv->priv == NULL)
		return -1;

	iaprivate *self = drv->priv;

	memset(db_path, 0, db_path_sz); 
	strcat(db_path, datadir);
	strcat(db_path, db_name);
	printf("db_path %s\n", db_path);

	rc = sqlite3_open(db_path, &(self->db));
	if(rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(self->db));
		sqlite3_close(self->db);
		goto bailout;
	}
	/* LY: suggestions are welcome */
	memset(cmd_buf, 0, CMD_SIZE);

	/* PRAGMA synchronous = 0 | OFF | 1 | NORMAL | 2 | FULL; // тип синхронизации транзакции */
	switch(ioarena.conf.syncmode) {
	case IA_SYNC:
		snprintf(cmd_buf, CMD_SIZE, "PRAGMA synchronous=%d;", 2);
		break;
	case IA_LAZY:
		snprintf(cmd_buf, CMD_SIZE, "PRAGMA synchronous=%d;", 1);
		break;
	case IA_NOSYNC:
		snprintf(cmd_buf, CMD_SIZE, "PRAGMA synchronous=%d;", 0);
		break;
	default:
		ia_log("error: %s(): unsupported syncmode %s",
			__func__, ia_syncmode2str(ioarena.conf.syncmode));
		return -1;
	}
	rc = sqlite3_exec(self->db, cmd_buf, 0, 0, &zErrMsg);
	if(rc != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_close(self->db);
		goto bailout;
	}

	memset(cmd_buf, 0, CMD_SIZE);
	/* PRAGMA journal_mode = DELETE | TRUNCATE | PERSIST | MEMORY | WAL | OFF;  // задать тип журнала */
	switch(ioarena.conf.walmode) {
	case IA_WAL_INDEF:
/*		
		Default journal_mode is "DELETE"
		snprintf(cmd_buf, CMD_SIZE, "PRAGMA journal_mode=%s;", "DELETE");
*/
		break;
	case IA_WAL_ON:
		snprintf(cmd_buf, CMD_SIZE, "PRAGMA journal_mode=%s;", "WAL");
		break;
	case IA_WAL_OFF:
		snprintf(cmd_buf, CMD_SIZE, "PRAGMA journal_mode=%s;", "OFF");
		break;
	default:
		ia_log("error: %s(): unsupported walmode %s",
			__func__, ia_walmode2str(ioarena.conf.walmode));
		return -1;
	}

	rc = sqlite3_exec(self->db, cmd_buf, 0, 0, &zErrMsg);
	if(rc != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_close(self->db);
		goto bailout;
	}

	rc = sqlite3_exec(self->db, "CREATE TABLE IF NOT EXISTS benchmark_t(id INTEGER PRIMARY KEY AUTOINCREMENT, key TEXT, value BLOB)", 0, 0, &zErrMsg);
	if(rc != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_close(self->db);
		goto bailout;
	}

	return 0;

bailout:
	ia_log("error: %s, %s", __func__, zErrMsg);
	sqlite3_free(zErrMsg);
	return -1;
}

static int ia_sqlite3_close(void)
{
//	int rc;
//	char *zErrMsg = 0;
	iaprivate *self = ioarena.driver->priv;
	if (self) {
	/*
		rc = sqlite3_exec(self->db, "DROP TABLE IF EXISTS benchmark_t", 0, 0, &zErrMsg);
		if(rc != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}
	*/
		ioarena.driver->priv = NULL;
		if (self->db)
			sqlite3_close(self->db);
		free(self);
	}
	return 0;
}

void ia_sqlite3_thread_dispose(iacontext *ctx)
{
	free(ctx);
}

static iacontext* ia_sqlite3_thread_new(void)
{
	iacontext* ctx = calloc(1, sizeof(iacontext));
	return ctx;
}

static int ia_sqlite3_begin(iacontext *ctx, iabenchmark step)
{
	int rc;
	(void) ctx;
	char *zErrMsg = 0;
	iaprivate *self = ioarena.driver->priv;
	//memset(ctx->cmd_buf, 0, CMD_SIZE);
	//snprintf(ctx->cmd_buf, CMD_SIZE, "PRAGMA journal_mode=%s;");


	switch(step) {
	case IA_CRUD:
	case IA_BATCH:
		rc = sqlite3_exec(self->db, "BEGIN;", 0, 0, &zErrMsg);
		if(rc != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			goto bailout;
		}
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
bailout:
	ia_log("error: %s, %s, %s", __func__, ia_benchmarkof(step), zErrMsg);
	sqlite3_free(zErrMsg);
	return -1;
}

static int ia_sqlite3_done(iacontext* ctx, iabenchmark step)
{
	int rc;
	(void) ctx;
	char *zErrMsg = 0;
	iaprivate *self = ioarena.driver->priv;

	switch(step) {
	case IA_CRUD:
	case IA_BATCH:
		rc = sqlite3_exec(self->db, "COMMIT;", 0, 0, &zErrMsg);
		if(rc != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			goto bailout;
		}
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
bailout:
	ia_log("error: %s, %s, %s", __func__, ia_benchmarkof(step), zErrMsg);
	sqlite3_free(zErrMsg);
	return -1;
}

static int select_callback(void *data, int argc, char **argv, char **azColName){
	int i;
	fprintf(stderr, "%s: ", (const char*)data);
	for(i=0; i<argc; i++){
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}

static int ia_sqlite3_next(iacontext* ctx, iabenchmark step, iakv *kv)
{
	int rc;
	(void) ctx;
	char cmd_buf[CMD_SIZE];
	char *zErrMsg = 0;
	//char key[kv->ksize + 1];
	//key[kv->ksize] = 0;
	//memcpy(key, kv->k, kv->ksize);
	//memset(cmd_buf, 0, CMD_SIZE);
	iaprivate *self = ioarena.driver->priv;

	switch(step) {
	case IA_SET:
		memset(cmd_buf, 0, CMD_SIZE);
		snprintf(cmd_buf, CMD_SIZE, "REPLACE INTO benchmark_t (key, value) VALUES(\"%s\", ?);", kv->k);
		sqlite3_stmt *stmt = NULL;
		//rc = sqlite3_prepare_v2(self->db, cmd_buf, -1, &stmt, &zErrMsg);
		rc = sqlite3_prepare_v2(self->db, cmd_buf, -1, &stmt, NULL);
		if (rc != SQLITE_OK) {
			fprintf(stderr, "prepare failed: %s, error: %s\n", sqlite3_errmsg(self->db), zErrMsg);
			//rc = -1;
			goto bailout;
		} else {
			// SQLITE_STATIC because the statement is finalized
			// before the buffer is freed:
			rc = sqlite3_bind_blob(stmt, 1, kv->v, kv->vsize, SQLITE_STATIC);
			if (rc != SQLITE_OK) {
				fprintf(stderr, "bind failed, error: %s\n", sqlite3_errmsg(self->db));
				goto bailout;
			} else {
				rc = sqlite3_step(stmt);
				if (rc != SQLITE_DONE) {
					fprintf(stderr, "execution failed, error: %s\n", sqlite3_errmsg(self->db));
					goto bailout;
				}
			}
		}
	        sqlite3_finalize(stmt);
		//rc = ness_db_set(self->db, kv->k, kv->ksize, kv->v, kv->vsize);
		//if (rc != 1)
		//	goto bailout;
		rc = 0;
		break;
	case IA_DELETE:
		memset(cmd_buf, 0, CMD_SIZE);
		snprintf(cmd_buf, CMD_SIZE, "DELETE FROM benchmark_t WHERE key = \"%s\";", kv->k);
		rc = sqlite3_exec(self->db, cmd_buf, 0, 0, &zErrMsg);
		if(rc != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_close(self->db);
			goto bailout;
		}
		rc = 0;
		break;
	case IA_GET:
		//const char* data = "Select callback function called";
		memset(cmd_buf, 0, CMD_SIZE);
		snprintf(cmd_buf, CMD_SIZE, "SELECT * FROM benchmark_t WHERE key = \"%s\";", kv->k);
		//rc = sqlite3_exec(self->db, cmd_buf, select_callback, (void *)data, &zErrMsg);
		rc = sqlite3_exec(self->db, cmd_buf, select_callback, NULL, &zErrMsg);
		if(rc != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_close(self->db);
			goto bailout;
		}
		rc = 0;
	case IA_ITERATE:
		rc = 0;
		break;
	default:
		assert(0);
		rc = -1;
	}
	return rc;

bailout:
	ia_log("error: %s, %s, %s", __func__, ia_benchmarkof(step), zErrMsg);
	sqlite3_free(zErrMsg);
	return -1;
}

iadriver ia_sqlite3 =
{
	.name  = "sqlite3",
	.priv  = NULL,
	.open  = ia_sqlite3_open,
	.close = ia_sqlite3_close,

	.thread_new = ia_sqlite3_thread_new,
	.thread_dispose = ia_sqlite3_thread_dispose,
	.begin	= ia_sqlite3_begin,
	.next	= ia_sqlite3_next,
	.done	= ia_sqlite3_done
};
