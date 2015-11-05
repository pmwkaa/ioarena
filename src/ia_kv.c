
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

#include <ioarena.h>

int ia_kvinit(iakv *kv, int ksize, int vsize)
{
	kv->ksize = ksize;
	kv->vsize = vsize;
	kv->k = malloc(ksize);
	if (kv->k == NULL)
		return -1;
	kv->v = NULL;
	if (vsize > 0) {
		kv->v = malloc(vsize);
		if (kv->v == NULL) {
			free(kv->k);
			kv->k = NULL;
			return -1;
		}
		memset(kv->v, 'x', vsize);
		kv->v[vsize - 1] = 0;
	}
	srand(0);
	return 0;
}

void ia_kvreset(iakv *kv)
{
	(void)kv;
	srand(0);
}

void ia_kvfree(iakv *kv)
{
	if (kv->k)
		free(kv->k);
	if (kv->v)
		free(kv->v);
}
