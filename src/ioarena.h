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

typedef enum {
	IA_SET,
	IA_BATCH,
	IA_CRUD,
	IA_DELETE,
	IA_ITERATE,
	IA_GET,
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
