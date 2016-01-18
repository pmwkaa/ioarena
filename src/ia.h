#ifndef IA_H_
#define IA_H_

/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

typedef struct ia ia;

struct ia {
	iadriver    *driver;
	iaconfig     conf;
	pthread_barrier_t barrier_start;
	int doers_count;
	volatile int doers_done;
	pthread_barrier_t barrier_fihish;
	size_t before_open_ram;
	char datadir[PATH_MAX];
	int failed;
};

int  ia_init(ia*, int, char**);
void ia_free(ia*);
int  ia_run(ia*);
void ia_fatal(const char *msg);
void ia_global_init(void);

#endif
