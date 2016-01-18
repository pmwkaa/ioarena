#ifndef IA_KV_H_
#define IA_KV_H_

/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

typedef struct iakv iakv;

struct iakv {
	size_t   ksize;
	size_t   vsize;
	char *k, *v;
};

struct iakvpool {
	uint64_t space, serial, period;
	size_t pos, power;
	unsigned ksize, vsize;
	char* flat;
};

void ia_kvpool_init(struct iakvpool *pool, int ksize, int vsize, int key_space, int key_sequence, uint64_t period);
void ia_kvpool_destory(struct iakvpool *pool);
void ia_kvpool_fill(struct iakvpool *pool, size_t nbandles, size_t nelem);
int ia_kvpool_pull(struct iakvpool *pool, iakv* one);
void ia_kvpool_rewind(struct iakvpool *pool);

#endif /* IA_KV_H_ */
