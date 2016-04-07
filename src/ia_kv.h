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

struct ia_kvgen;

int ia_kvgen_setup(char printable, unsigned ksize, unsigned nspaces, unsigned nsectors, uintmax_t period, int seed);
int ia_kvgen_init(struct ia_kvgen **genptr, unsigned kspace, unsigned ksector, unsigned vsize, unsigned vage);
void ia_kvgen_destroy(struct ia_kvgen **genptr);
int ia_kvgen_get(struct ia_kvgen *gen, iakv *p, char key_only);

struct ia_kvpool;

int ia_kvpool_init(struct ia_kvpool **poolptr, struct ia_kvgen *gen, int pool_size);
int ia_kvpool_pull(struct ia_kvpool *pool, iakv *p);
void ia_kvpool_destroy(struct ia_kvpool **pool);

#endif /* IA_KV_H_ */
