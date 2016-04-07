#ifndef IA_BENCHMARK_H_
#define IA_BENCHMARK_H_

/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

extern const long bench_mask_read;
extern const long bench_mask_write;
extern const long bench_mask_2keyspace;

const char* ia_benchmarkof(iabenchmark bench);
iabenchmark ia_benchmark(const char *name);

typedef struct iadoer iadoer;

struct iadoer {
	int nth;
	int key_space, key_sequence;
	long benchmask;
	iacontext *ctx;
	struct ia_kvgen *gen_a;
	struct ia_kvgen *gen_b;
	iahistogram hg;
};

int ia_doer_init(iadoer *doer, int nth, long benchmask, int key_space, int key_sequence);
int ia_doer_fulfil(iadoer *doer);
void ia_doer_destroy(iadoer *doer);

#endif
