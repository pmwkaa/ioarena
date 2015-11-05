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
	iaconfig     conf;
	iakv         kv;
	iahistogram  hg;
	iadriver    *driver;
};

int  ia_init(ia*, int, char**);
void ia_free(ia*);
int  ia_run(ia*);

#endif
