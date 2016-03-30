
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

	int bufsize = ksize + vsize;
	if (printable)
		bufsize += 2;

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

static
char* kv_rnd(uint64_t point, char* dst, char printable, int length)
{
	point = point * 6364136223846793005ull + 1442695040888963407ull;
	int left = 64;
	uint64_t gen = point;

	while(--length >= printable) {
		unsigned v;
		if (printable) {
			assert(ALPHABET_CARDINALITY == 64);
			v = alphabet[gen & 63];
			gen >>= 6;
			left -= 6;
		} else {
			v = (char) gen;
			gen >>= 8;
			left -= 8;
		}
		*dst++ = v;

		if (left <= 0) {
			point ^= ((point << 47) | (point >> 17)) + 250297178449537ul;
			gen = point;
			left = 64;
		}
	}

	if (printable)
		*dst++ = 0;
	point = gen;

	return dst;
}

int ia_kvgen_getcouple(struct iakvgen *gen, iakv *a, iakv *b, char key_only)
{
	char* dst = gen->buf;

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
