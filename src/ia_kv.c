
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
	unsigned ksize, vsize, width;
	uint64_t m, a, c;
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

#define BITMASK(n) ((1ull << (n)) - 1)

int ia_kvgen_init(struct iakvgen **genptr, char printable, unsigned ksize, unsigned vsize, unsigned key_sector, unsigned key_sequence, uintmax_t period)
{
	*genptr = NULL;
	uint64_t top, a, c;
	unsigned width;

	double maxkey = (double) key_sector * (double) period + (double) period;
	if (maxkey <= BITMASK(24)) {
		width = 24/8;
		top = BITMASK(24);
		a = 1140671485ul;
		c = 12820163ul;
	} else if (maxkey <= BITMASK(32)) {
		width = 32/8;
		top = BITMASK(32);
		a = 1664525ul;
		c = 1013904223ul;
	} else if (maxkey <= BITMASK(40)) {
		width = 40/8;
		top = BITMASK(40);
		a = 330169576829ull;
		c = 7036301ul;
	} else if (maxkey <= BITMASK(48)) {
		width = 48/8;
		top = BITMASK(48);
		a = 25214903917ull;
		c = 7522741ul;
	/* } else if (maxkey <= BITMASK(56)) {
		width = 56/8;
		top = BITMASK(56);
		a = ?;
		c = ?; */
	} else if (maxkey <= UINT64_MAX) {
		width = 64/8;
		top = UINT64_MAX;
		a = 6364136223846793005ull;
		c = 1442695040888963407ull;
	} else {
		double width = log(maxkey) / log(2);
		ia_log("key-gen: %u sector of %ju items is too huge, unable provide by %u-bit arithmetics, at least %d required",
			key_sector, period, (unsigned) sizeof(uintmax_t) * 8, (int) ceil(width));
		return -1;
	}

	double bytes2maxkey = log(top) / log(printable ? ALPHABET_CARDINALITY : 256);
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

	gen->width = width;
	gen->m = top;
	gen->a = a;
	gen->c = c;

	ia_log("key-gen: use a=%ju, c=%ju, m=2^%u up to %.0lf keys", a, c, width*8, maxkey);

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

static __inline uint64_t mixup4initial(uint64_t point, struct iakvgen *gen)
{
	/* LY: mixup by linear congruential */
	return (point * gen->a + gen->c) & gen->m;
}

static __inline uint64_t remix4tail(uint64_t point)
{
	/* LY: fast and dirty remix */
	return point ^ (((point << 47) | (point >> 17)) + 250297178449537ull);
}

static char* kv_rnd(uint64_t *point, char* dst, char printable, int length, int left)
{
	assert(length > 0);

	if (printable) {
		assert(ALPHABET_CARDINALITY == 64);
		uint64_t acc = *point;

		for(;;) {
			*dst++ = alphabet[acc & 63];
			if (--length == 0)
				break;
			acc >>= 6;
			left -= 6;
			if (left <= 0) {
				*point = remix4tail(*point);
				acc = *point;
				left = 64;
			}
		}
		*dst++ = 0;
	} else {
		uint64_t *p = (void *) dst;
		for(;;) {
			*p++ = *point;
			length -= 8;
			if (length <= 0)
				break;
			*point = remix4tail(*point);
		}
		dst = (void *) p;
	}

	return dst;
}

static char* ia_kvgen_get(struct iakvgen *gen, iakv *p, char key_only, uint64_t shift, char* dst)
{
	uint64_t point = mixup4initial(gen->base + shift, gen);
	dst = kv_rnd(&point, p->k = dst, gen->printable, p->ksize = gen->ksize, gen->width * 8);
	p->vsize = 0;
	p->v = NULL;
	if (! key_only) {
		point = remix4tail(point);
		dst = kv_rnd(&point, p->v = dst, gen->printable, p->vsize = gen->vsize, 64);
	}
	return dst;
}

int ia_kvgen_getcouple(struct iakvgen *gen, iakv *a, iakv *b, char key_only)
{
	char* dst = gen->buf;

	if (! gen->vsize)
		key_only = 1;

	if (a)
		dst = ia_kvgen_get(gen, a, key_only, gen->serial, dst);

	if (b)
		dst = ia_kvgen_get(gen, b, key_only, (gen->serial + 1) % gen->period, dst);

	gen->serial = (gen->serial + 2) % gen->period;
	return 0;
}

void ia_kvgen_rewind(struct iakvgen *gen)
{
	gen->serial = 0;
}
