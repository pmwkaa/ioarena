#ifndef IA_KV_H_
#define IA_KV_H_

/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct iakv iakv;

struct iakv {
	int   ksize;
	int   vsize;
	char *k, *v;
};

int  ia_kvinit(iakv*, int, int);
void ia_kvfree(iakv*);
void ia_kvreset(iakv*);

static inline void ia_kv(iakv *kv)
{
	const char x[] =
		"abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"1234567890";
	int i = 0;
	while (i < kv->ksize - 1) {
		uint32_t seed = rand();
		kv->k[i] = x[seed % sizeof(x)];
		i++;
	}
	kv->k[kv->ksize - 1] = 0;
}

#endif
