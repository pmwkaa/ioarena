#ifndef IA_DRIVER_H_
#define IA_DRIVER_H_

/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

typedef struct iadriver iadriver;
typedef struct iaprivate iaprivate;
typedef struct iacontext iacontext;

struct iadriver {
	const char *name;
	iaprivate *priv;
	int (*open)(const char *datadir);
	int (*close)(void);

	iacontext* (*thread_new)(void);
	void (*thread_dispose)(iacontext*);

	int (*begin)(iacontext*, iabenchmark);
	int (*next)(iacontext*, iabenchmark, iakv *kv);
	int (*done)(iacontext*, iabenchmark);
};

#endif
