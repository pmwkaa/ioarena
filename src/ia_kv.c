
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

struct iakvgen {
	uintmax_t base, serial, period;
	unsigned ksize, vsize;
	char printable;
	char buf[];
};

#define ALPHABET_CARDINALITY 64 /* 2 + 10 + 26 + 26 */
static const unsigned char alphabet[ALPHABET_CARDINALITY] =
	"@"
	"0123456789"
	"abcdefghijklmnopqrstuvwxyz"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"_";

int ia_kvgen_init(struct iakvgen **genptr, char printable, unsigned ksize, unsigned vsize, unsigned key_sector, unsigned key_sequence, uintmax_t period)
{
	*genptr = NULL;

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
		ia_log("key-gen: key-length %u is insufficient for %u sector of %s %ju items, at least %d required",
			ksize, key_sector, printable ? "printable" : "binary", period, (int) ceil(bytes2maxkey));
		return -1;
	}

	size_t bufsize;
	if (printable) {
		bufsize = ksize + vsize + 2;
	} else {
		bufsize = ((ksize + 7) & ~7ul) + ((vsize + 7) & ~7ul);
	}

	struct iakvgen *gen = calloc(1, sizeof(struct iakvgen) + bufsize * 2);
	if (!gen)
		return -1;

	gen->base = key_sector * period;
	gen->serial = key_sequence % period;
	gen->period = period;
	gen->ksize = ksize;
	gen->vsize = vsize;
	gen->printable = printable;

	*genptr = gen;
	return 0;
}

void ia_kvgen_destory(struct iakvgen **genptr)
{
	struct iakvgen *gen = *genptr;
	if (gen) {
		free(gen);
		*genptr = NULL;
	}
}

static __inline uint64_t mixup4initial(uint64_t point)
{
	/* LY: Linear congruential 2^64 by Donald Knuth */
	return point * 6364136223846793005ull + 1442695040888963407ull;
}

static __inline uint64_t remix4tail(uint64_t point)
{
	/* LY: fast and dirty remix */
	return point ^ (((point << 47) | (point >> 17)) + 250297178449537ull);
}

static
char* kv_rnd(uint64_t point, char* dst, char printable, int length)
{
	assert(length > 0);
	point = mixup4initial(point);

	if (printable) {
		int left = 64;
		uint64_t acc = point;
		assert(ALPHABET_CARDINALITY == 64);

		for(;;) {
			*dst++ = alphabet[acc & 63];
			if (--length == 0)
				break;
			acc >>= 6;
			left -= 6;
			if (left <= 0) {
				point = remix4tail(point);
				acc = point;
				left = 64;
			}
		}
		*dst++ = 0;
	} else {
		uint64_t *p = (void *) dst;
		for(;;) {
			*p++ = point;
			length -= 8;
			if (length <= 0)
				break;
			point = remix4tail(point);
		}
		dst = (void *) p;
	}

	return dst;
}

int ia_kvgen_getcouple(struct iakvgen *gen, iakv *a, iakv *b, char key_only)
{
	char* dst = gen->buf;

	if (! gen->vsize)
		key_only = 1;

	if (a) {
		uintmax_t point = gen->base + gen->serial;
		dst = kv_rnd(point, a->k = dst, gen->printable, a->ksize = gen->ksize);
		a->vsize = 0;
		a->v = NULL;
		if (! key_only)
			dst = kv_rnd(point + (1ul << 47), a->v = dst, gen->printable, a->vsize = gen->vsize);
	}

	if (b) {
		uintmax_t point = gen->base + (gen->serial + 1) % gen->period;
		dst = kv_rnd(point, b->k = dst, gen->printable, b->ksize = gen->ksize);
		b->vsize = 0;
		b->v = NULL;
		if (! key_only)
			dst = kv_rnd(point + (1ul << 47), b->v = dst, gen->printable, b->vsize = gen->vsize);
	}

	gen->serial = (gen->serial + 2) % gen->period;
	return 0;
}

void ia_kvgen_rewind(struct iakvgen *gen)
{
	gen->serial = 0;
}
