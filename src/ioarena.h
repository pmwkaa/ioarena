#pragma once
#ifndef IOARENA_H_
#define IOARENA_H_

/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
 */

#define _GNU_SOURCE 1

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* LY: Mac OS X workaround */
#ifndef PTHREAD_BARRIER_SERIAL_THREAD

#define PTHREAD_BARRIER_SERIAL_THREAD 1

typedef struct {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  int canary;
  int threshold;
} pthread_barrier_t;

typedef struct {
} pthread_barrierattr_t;

int pthread_barrier_init(pthread_barrier_t *barrier,
                         const pthread_barrierattr_t *attr, unsigned count);
int pthread_barrier_destroy(pthread_barrier_t *barrier);
int pthread_barrier_wait(pthread_barrier_t *barrier);

#define IOARENA_NEEDS_PTHREAD_BARRIER_IMPL

#endif /* PTHREAD_BARRIER_SERIAL_THREAD */

typedef enum {
  IA_SET,
  IA_BATCH,
  IA_CRUD,
  IA_DELETE,
  IA_ITERATE,
  IA_GET,
  IA_MAX
} iabenchmark;

typedef enum { IA_SYNC, IA_LAZY, IA_NOSYNC } iasyncmode;

typedef enum { IA_WAL_INDEF, IA_WAL_ON, IA_WAL_OFF } iawalmode;

#include "ia.h"
#include "ia_benchmark.h"
#include "ia_build.h"
#include "ia_config.h"
#include "ia_driver.h"
#include "ia_histogram.h"
#include "ia_kv.h"
#include "ia_log.h"
#include "ia_rusage.h"
#include "ia_time.h"

extern ia ioarena;

#endif
