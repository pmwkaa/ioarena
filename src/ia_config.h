#ifndef IA_CONFIG_H_
#define IA_CONFIG_H_

/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) Dmitry Simonenko
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
	int vsize;
	uint64_t count;
	int csv;
};

int  ia_configinit(iaconfig*);
int  ia_configparse(iaconfig*, int, char**);
void ia_configprint(iaconfig*);
void ia_configfree(iaconfig*);

#endif
