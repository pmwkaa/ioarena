#ifndef IOARENA_H_
#define IOARENA_H_

/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

#define _GNU_SOURCE 1

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/resource.h>
#include <limits.h>
#include <ftw.h>


/* LY: Mac OS X workaround */
#ifndef PTHREAD_BARRIER_SERIAL_THREAD

#define PTHREAD_BARRIER_SERIAL_THREAD 1

typedef struct {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int canary;
	int threshold;
} pthread_barrier_t;

typedef struct {} pthread_barrierattr_t;

int pthread_barrier_init(pthread_barrier_t* barrier, const pthread_barrierattr_t* attr, unsigned count);
int pthread_barrier_destroy(pthread_barrier_t* barrier);
int pthread_barrier_wait(pthread_barrier_t* barrier);

#define IOARENA_NEEDS_PTHREAD_BARRIER_IMPL

#endif /* PTHREAD_BARRIER_SERIAL_THREAD */

typedef enum {
	IA_SET,
	IA_BATCH,
	IA_CRUD,
	IA_DELETE,
	IA_ITERATE,
	IA_GET,
	IA_MIX_70_30, //70% read / 30% write
	IA_MIX_30_70,
	IA_MIX_50_50,
	IA_MAX
} iabenchmark;

typedef enum {
	IA_SYNC,
	IA_LAZY,
	IA_NOSYNC
} iasyncmode;

typedef enum {
	IA_WAL_INDEF,
	IA_WAL_ON,
	IA_WAL_OFF
} iawalmode;

#include <ia_time.h>
#include <ia_rusage.h>
#include <ia_kv.h>
#include <ia_driver.h>
#include <ia_config.h>
#include <ia_histogram.h>
#include <ia_benchmark.h>
#include <ia_build.h>
#include <ia_log.h>
#include <ia.h>

extern iadriver ia_leveldb;
extern iadriver ia_rocksdb;
extern iadriver ia_lmdb;
extern iadriver ia_mdbx;
extern iadriver ia_forestdb;
extern iadriver ia_wt;
extern iadriver ia_sophia;
extern iadriver ia_nessdb;
extern iadriver ia_dummy;
extern ia ioarena;

#endif
