
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

#include <ioarena.h>
#include <math.h>

#define DEBUG_KEYGEN 0

#ifndef DEBUG_KEYGEN
#	define DEBUG_KEYGEN 0
#endif

#define ALPHABET_CARDINALITY 62 /* 10 + 26 + 26 */
static const unsigned char alphabet[ALPHABET_CARDINALITY] =
	"0123456789"
	"abcdefghijklmnopqrstuvwxyz"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ";

int ia_kvpool_init(struct iakvpool *pool, char printable, unsigned ksize, unsigned vsize, unsigned key_sector, unsigned key_sequence, uintmax_t period)
{
	pool->base = (key_sector % 257) * period;
	pool->serial = key_sequence % period;
	pool->period = period;

	pool->ksize = ksize;
	pool->vsize = vsize;
	pool->pos = 0;
	pool->power = 0;
	pool->flat = NULL;

	double maxkey = (double) key_sector * (double) period + (double) (period - 1);
	if (maxkey > UINT64_MAX) {
		double width = log(maxkey) / log(2);
		ia_log("key-gen: %u sector of %ju items is too huge, unable provide by %u-bit arithmetics, at least %d required",
			key_sector, period, (unsigned) sizeof(uintmax_t) * 8, (int) ceil(width));
		return -1;
	}

	/* LY: currently a 64-bit congruential mixup is used only (see below),
	 * therefore we always need space for a 64-bit keys */
	maxkey = UINT64_MAX;

	double bytes2maxkey = log(maxkey) / log(printable ? ALPHABET_CARDINALITY : 256 );
	if (bytes2maxkey > (double) ksize) {
		ia_log("key-pool: key-length %u is insufficient for %u sector of %s %ju items, at least %d required",
			ksize, key_sector, printable ? "printable" : "binary", period, (int) ceil(bytes2maxkey));
		return -1;
	}

	return 0;
}

void ia_kvpool_destory(struct iakvpool *pool)
{
	free(pool->flat);
	pool->pos = 0;
	pool->power = 0;
	pool->flat = NULL;
}

static
char* kv_rnd(uintmax_t *point, char* dst, char printable, int length)
{
	uintmax_t gen = *point;

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
		ia_fatal(__func__);
	}

	pool->flat = dst;
	for (i = 0; i < nbandles; ++i) {
		uintmax_t point = pool->base + pool->serial;
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
