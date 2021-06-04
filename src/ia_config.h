#pragma once
#ifndef IA_CONFIG_H_
#define IA_CONFIG_H_

/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
 */

typedef struct iaconfig iaconfig;

#include "ia_driver.h"

struct iaoption {
  const char *arg;
  struct iaoption *next;
};

struct iaconfig {
  char *driver;
  iadriver *driver_if;
  char *path;
  char *benchmark;
  struct iaoption *drv_opts;
  int benchmark_list[IA_MAX];
  int ksize;
  int vsize;
  uintmax_t count;
  iasyncmode syncmode;
  iawalmode walmode;
  int rthr;
  int wthr;
  int batch_length;
  int nrepeat;
  int kvseed;
  const char *csv_prefix;
  char binary;
  char separate;
  char ignore_keynotfound;
  char continuous_completing;
};

int ia_configinit(iaconfig *);
int ia_configparse(iaconfig *, int, char **);
void ia_configprint(iaconfig *);
void ia_configfree(iaconfig *);

const char *ia_syncmode2str(iasyncmode syncmode);
const char *ia_walmode2str(iawalmode walmode);

#define ia_opt_bool_default 0
#define ia_opt_bool_off -1
#define ia_opt_bool_on 1
int ia_parse_option_bool(const char **parg, const char *opt, int8_t *target);

#endif
