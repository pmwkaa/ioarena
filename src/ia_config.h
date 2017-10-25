#ifndef IA_CONFIG_H_
#define IA_CONFIG_H_

/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

typedef struct iaconfig iaconfig;

struct iaconfig {
	char *driver;
	iadriver *driver_if;
	char *path;
	char *benchmark;
	int benchmark_list[IA_MAX];
	int ksize;
	uintmax_t keys;
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

int  ia_configinit(iaconfig*);
int  ia_configparse(iaconfig*, int, char**);
void ia_configprint(iaconfig*);
void ia_configfree(iaconfig*);

const char* ia_syncmode2str(iasyncmode syncmode);
const char* ia_walmode2str(iawalmode walmode);

#endif
