/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) Dmitry Simonenko
 * Copyright (c) Leonid Yuriev
 * BSD License
 */

#include "ioarena.h"

int ia_configinit(iaconfig *c) {
  c->driver = NULL;
  c->driver_if = NULL;
  c->drv_opts = NULL;
  c->path = strdup("./_ioarena");
  if (c->path == NULL)
    return -1;
  c->count = 1000000;
  c->ksize = 16;
  c->vsize = 32;
  c->csv_prefix = NULL;
  c->rthr = 0;
  c->wthr = 0;
  c->batch_length = 500;
  c->benchmark = strdup("set, get");
  if (c->benchmark == NULL) {
    free(c->path);
    c->path = NULL;
    return -1;
  }
  int i = 0;
  while (i < IA_MAX) {
    c->benchmark_list[i] = 0;
    i++;
  }
  c->syncmode = IA_LAZY;
  c->walmode = IA_WAL_INDEF;
  c->continuous_completing = 0;
  c->nrepeat = 1;

  c->kvseed = 42;
  c->binary = 0;
  c->separate = 0;
  c->ignore_keynotfound = 0;
  return 0;
}

const char *ia_syncmode2str(iasyncmode syncmode) {
  switch (syncmode) {
  case IA_SYNC:
    return "sync";
  case IA_LAZY:
    return "lazy";
  case IA_NOSYNC:
    return "nosync";
  default:
    return "???";
  }
}

static iasyncmode ia_str2syncmode(const char *str) {
  if (strcasecmp(str, ia_syncmode2str(IA_SYNC)) == 0)
    return IA_SYNC;
  if (strcasecmp(str, ia_syncmode2str(IA_LAZY)) == 0)
    return IA_LAZY;
  if (strcasecmp(str, ia_syncmode2str(IA_NOSYNC)) == 0)
    return IA_NOSYNC;
  return -1;
}

const char *ia_walmode2str(iawalmode walmode) {
  switch (walmode) {
  case IA_WAL_INDEF:
    return "indef";
  case IA_WAL_ON:
    return "walon";
  case IA_WAL_OFF:
    return "waloff";
  default:
    return "???";
  }
}

static iawalmode ia_str2walmode(const char *str) {
  if (strcasecmp(str, ia_walmode2str(IA_WAL_INDEF)) == 0)
    return IA_WAL_INDEF;
  if (strcasecmp(str, ia_walmode2str(IA_WAL_ON)) == 0)
    return IA_WAL_ON;
  if (strcasecmp(str, ia_walmode2str(IA_WAL_OFF)) == 0)
    return IA_WAL_OFF;
  return -1;
}

static inline void ia_configusage(iaconfig *c) {
  ia_log("usage: ioarena [hDBCpnkvmlrwic]");
  ia_log("  -D <database_driver>");
  ia_log("     choices: %s", ia_supported());
  ia_log("  -B <benchmarks>");
  ia_log("     choices: set, get, delete, iterate, batch, crud");
  ia_log("  -m <sync_mode>                     (default: %s)",
         ia_syncmode2str(c->syncmode));
  ia_log("     choices: sync, lazy, nosync");
  ia_log("  -l <wal_mode>                      (default: %s)",
         ia_walmode2str(c->walmode));
  ia_log("     choices: indef, walon, waloff");
  if (c->driver_if) {
    if (c->driver_if->option)
      c->driver_if->option(NULL, "--help");
  } else
    ia_log("  -o <database_option>");
  ia_log("  -C <name-prefix> generate csv      (default: %s)", c->csv_prefix);
  ia_log("  -p <path> for temporaries          (default: %s)", c->path);
  ia_log("  -n <number_of_operations>          (default: %ju)", c->count);
  ia_log("  -k <key_size>                      (default: %d)", c->ksize);
  ia_log("  -v <value_size>                    (default: %d)", c->vsize);
  ia_log("  -c continuous completing mode      (default: %s)",
         c->continuous_completing ? "yes" : "no");
  ia_log("  -r <number_of_read_threads>        (default: %d)", c->rthr);
  ia_log("     `zero` to use single main/common thread");
  ia_log("  -w <number_of_crud/write_threads>  (default: %d)", c->wthr);
  ia_log("     `zero` to use single main/common thread");
  ia_log("  -i ignore key-not-found error      (default: %s)",
         c->ignore_keynotfound ? "yes" : "no");
  ia_log("  -h                                 help");

  ia_log("\nexample:");
  ia_log("   ioarena -m sync -D sophia -B crud -n 100000000");
}

int ia_configparse(iaconfig *c, int argc, char **argv) {
  int opt;
  struct iaoption **drv_opt /* the tail of single-linked list */ = &c->drv_opts;
  while ((opt = getopt(argc, argv, "hD:T:B:p:n:k:v:C:m:l:r:w:ico:")) != -1) {
    switch (opt) {
    case 'D':
      if (c->driver)
        free(c->driver);
      c->driver = strdup(optarg);
      if (c->driver == NULL)
        return -1;
      c->driver_if = ia_get_driver_for(c->driver);
      if (c->driver_if == NULL) {
        ia_log("error: unknown database driver '%s'", c->driver);
        return -1;
      }
      break;
    case 'o':
      *drv_opt = calloc(1, sizeof(struct iaoption));
      if (!*drv_opt)
        return -1;
      (*drv_opt)->arg = strdup(optarg);
      if (!(*drv_opt)->arg)
        return -1;
      drv_opt = &(*drv_opt)->next;
      break;
    case 'T':
    case 'B':
      if (c->benchmark)
        free(c->benchmark);
      c->benchmark = strdup(optarg);
      if (c->benchmark == NULL)
        return -1;
      break;
    case 'p':
      if (c->path)
        free(c->path);
      c->path = strdup(optarg);
      if (c->path == NULL)
        return -1;
      break;
    case 'n':
      c->count = atoll(optarg);
      break;
    case 'k':
      c->ksize = atoi(optarg);
      break;
    case 'v':
      c->vsize = atoi(optarg);
      break;
    case 'C':
      c->csv_prefix = optarg;
      break;
    case 'm':
      c->syncmode = ia_str2syncmode(optarg);
      if (c->syncmode == (iasyncmode)-1) {
        ia_log("error: unknown syncmode '%s'", optarg);
        return -1;
      }
      break;
    case 'l':
      c->walmode = ia_str2walmode(optarg);
      if (c->walmode == (iawalmode)-1) {
        ia_log("error: unknown walmode '%s'", optarg);
        return -1;
      }
      break;
    case 'r':
      if (optarg)
        c->rthr = atoi(optarg);
      else
        c->rthr = sysconf(_SC_NPROCESSORS_ONLN);
      break;
    case 'w':
      if (optarg)
        c->wthr = atoi(optarg);
      else
        c->wthr = sysconf(_SC_NPROCESSORS_ONLN);
      break;
    case 'i':
      c->ignore_keynotfound = 1;
      break;
    case 'c':
      c->continuous_completing = 1;
      break;
    case 'h':
      ia_configusage(c);
      return 1;
    default:
      ia_configusage(c);
      return -1;
    }
  }
  if (c->driver == NULL) {
    ia_configusage(c);
    return -1;
  }
  if (c->drv_opts && !c->driver_if->option) {
    ia_log("error: database driver '%s' don't support option(s)", c->driver);
    return -1;
  }
  if (c->ksize <= 0) {
    ia_log("error: bad key size");
    return -1;
  }
  char buf[128];
  snprintf(buf, sizeof(buf), "%s", c->benchmark);
  char *p;
  for (p = strtok(buf, ", "); p; p = strtok(NULL, ", ")) {
    iabenchmark bench = ia_benchmark(p);
    if (bench == IA_MAX) {
      ia_log("error: unknown benchmark name '%s'", p);
      return -1;
    }
    c->benchmark_list[bench] = 1;
  }
  return 0;
}

void ia_configfree(iaconfig *c) {
  if (c->path)
    free(c->path);
  if (c->driver)
    free(c->driver);
  if (c->benchmark)
    free(c->benchmark);
  for (struct iaoption *drv_opt = c->drv_opts; drv_opt;) {
    void *tmp = drv_opt;
    free((void *)drv_opt->arg);
    drv_opt = drv_opt->next;
    free(tmp);
  }
}

void ia_configprint(iaconfig *c) {
  ia_log("configuration:");
  ia_log("  engine       = %s", c->driver);
  ia_log("  datadir      = %s", c->path);
  ia_log("  benchmark    = %s", c->benchmark);
  ia_log("  durability   = %s", ia_syncmode2str(c->syncmode));
  ia_log("  wal          = %s", ia_walmode2str(c->walmode));
  for (struct iaoption *drv_opt = c->drv_opts; drv_opt; drv_opt = drv_opt->next)
    ia_log("          option %s", drv_opt->arg);
  ia_log("  operations   = %ju", c->count);
  ia_log("  key size     = %d", c->ksize);
  ia_log("  value size   = %d", c->vsize);
  ia_log("  binary       = %s", c->binary ? "yes" : "no");
  if (c->rthr)
    ia_log("  r-threads    = %d", c->rthr);
  if (c->wthr)
    ia_log("  w-threads    = %d", c->wthr);
  if (ia_benchmark(c->benchmark) == IA_BATCH || c->benchmark_list[IA_BATCH])
    ia_log("  batch length = %d", c->batch_length);
  ia_log("  continuous   = %s\n", c->continuous_completing ? "yes" : "no");
}

const char *ia_benchmarkof(iabenchmark b) {
  switch (b) {
  case IA_SET:
    return "set";
  case IA_GET:
    return "get";
  case IA_DELETE:
    return "del";
  case IA_ITERATE:
    return "iter";
  case IA_BATCH:
    return "batch";
  case IA_CRUD:
    return "crud";
  default:
    assert(0);
  }
  return NULL;
}

iabenchmark ia_benchmark(const char *name) {
  if (strcasecmp(name, "set") == 0)
    return IA_SET;
  else if (strcasecmp(name, "get") == 0)
    return IA_GET;
  else if (strcasecmp(name, "del") == 0 || strcasecmp(name, "delete") == 0)
    return IA_DELETE;
  else if (strcasecmp(name, "iter") == 0 || strcasecmp(name, "iterate") == 0)
    return IA_ITERATE;
  else if (strcasecmp(name, "batch") == 0)
    return IA_BATCH;
  else if (strcasecmp(name, "crud") == 0 || strcasecmp(name, "transact") == 0)
    return IA_CRUD;
  return IA_MAX;
}

static int partmatch(const char *part, size_t part_len, const char *item) {
  return part_len == strlen(item) && strncasecmp(part, item, part_len) == 0;
}

int ia_parse_option_bool(const char **parg, const char *opt, int8_t *target) {
  const size_t name_len = strlen(opt);
  if (strncasecmp(*parg, opt, name_len) != 0 || (*parg)[name_len] != '=')
    return 0 /* don't match */;

  const char *const value = *parg + name_len + 1;
  const char *const comma = strchr(value, ',');
  const size_t value_len = comma ? (size_t)(comma - value) : strlen(value);

  if (partmatch(value, value_len, "ON") || partmatch(value, value_len, "YES") ||
      partmatch(value, value_len, "TRUE") || partmatch(value, value_len, "1"))
    *target = ia_opt_bool_on;
  else if (partmatch(value, value_len, "OFF") ||
           partmatch(value, value_len, "NO") ||
           partmatch(value, value_len, "FALSE") ||
           partmatch(value, value_len, "0"))
    *target = ia_opt_bool_off;
  else
    return -1 /* invalid value */;

  if (!comma)
    return 1 /* done */;

  *parg = comma + 1 /* seek after the comma */;
  return **parg == '\0' /* continue if not at the end */;
}
