
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

#include <ioarena.h>

#define DEBUG_KEYGEN 0

#ifndef DEBUG_KEYGEN
#	define DEBUG_KEYGEN 0
#endif

void ia_kvpool_init(struct iakvpool *pool, int ksize, int vsize, int key_space, int key_sequence, uint64_t period)
{
	pool->space = (key_space % 257) * period;
	pool->serial = key_sequence % period;
	pool->period = period;

	pool->ksize = ksize;
	pool->vsize = vsize;
	pool->pos = 0;
	pool->power = 0;
	pool->flat = NULL;
}

void ia_kvpool_destory(struct iakvpool *pool)
{
	free(pool->flat);
	pool->pos = 0;
	pool->power = 0;
	pool->flat = NULL;
}

static
char* kv_rnd(uint64_t *point, char* dst, char printable, int length)
{
	uint64_t gen = *point;

#if DEBUG_KEYGEN
	(void) printable;

	dst += length;
	char *p = dst;

	*--p = 0;
	while(--length >= 1) {
		unsigned v = gen % 10;
		gen /= 10;
		*--p = v + '0';
	}

	*point = *point * 6364136223846793005ull + 1442695040888963407ull;

#else

	const unsigned char alphabet[] =
		"0123456789"
		"abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	while(--length >= printable) {
		gen = gen * 6364136223846793005ull + 1442695040888963407ull;
		unsigned v = printable ? alphabet[gen % 61u] : gen % 65537u;
		*dst++ = v;
	}

	if (printable)
		*dst++ = 0;
	*point = gen;

#endif

	return dst;
}

void ia_kvpool_fill(struct iakvpool *pool, size_t nbandles, size_t nelem)
{
	size_t i, j, bytes = nbandles * nelem * (pool->vsize + pool->ksize);
	char* dst = realloc(pool->flat, bytes);
	if (! dst) {
		ia_log("error: out of memory");
		ia_fatal(__FUNCTION__);
	}

	pool->flat = dst;
	for (i = 0; i < nbandles; ++i) {
		uint64_t point = pool->space + pool->serial;
		pool->serial = (pool->serial + 1) % pool->period;

		for (j = 0; j < nelem; ++j) {
			dst = kv_rnd(&point, dst, !ioarena.conf.binary, pool->ksize);
			if (pool->vsize)
				dst = kv_rnd(&point, dst, !ioarena.conf.binary, pool->vsize);
		}
	}

	assert(dst == pool->flat + bytes);
	pool->pos = 0;
	pool->power = bytes;
}

int ia_kvpool_pull(struct iakvpool *pool, iakv* one)
{
	if (pool->power - pool->pos < pool->ksize + pool->vsize) {
		if (pool->power < pool->ksize + pool->vsize)
			return -1;
		pool->pos = 0;
	}

	one->k = pool->flat + pool->pos;
	one->ksize = pool->ksize;
	pool->pos += pool->ksize;

	one->v = pool->flat + pool->pos;
	one->vsize = pool->vsize;
	pool->pos += pool->vsize;

	if (!ioarena.conf.binary) {
		if (one->vsize)
			assert(one->v[one->vsize - 1] == 0);
		assert(one->k[one->ksize - 1] == 0);
	}

	return 0;
}

void ia_kvpool_rewind(struct iakvpool *pool)
{
	pool->pos = 0;
}
