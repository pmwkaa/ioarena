#ifndef IA_DRIVER_H_
#define IA_DRIVER_H_

/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

typedef struct iadriver iadriver;

struct iadriver {
	const char *name;
	void *priv;
	int (*open)(void);
	int (*close)(void);
	int (*run)(iabenchmark);
};

#endif
