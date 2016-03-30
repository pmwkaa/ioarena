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

struct iakvgen;

int ia_kvgen_init(struct iakvgen **genptr, char printable, unsigned ksize, unsigned vsize, unsigned key_sector, unsigned key_sequence, uintmax_t period);
void ia_kvgen_destory(struct iakvgen **genptr);
int ia_kvgen_getcouple(struct iakvgen *gen, iakv *a, iakv *b, char key_only);
void ia_kvgen_rewind(struct iakvgen *gen);

#endif /* IA_KV_H_ */
