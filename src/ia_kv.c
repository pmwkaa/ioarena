
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

#include <ioarena.h>
#include <math.h>

/* #define DEBUG_KEYGEN 1 */

#ifndef DEBUG_KEYGEN
#	define DEBUG_KEYGEN 0
#endif

#define ALPHABET_CARDINALITY 64 /* 2 + 10 + 26 + 26 */
static const unsigned char alphabet[ALPHABET_CARDINALITY] =
	"@"
	"0123456789"
	"abcdefghijklmnopqrstuvwxyz"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"_";

#define BITMASK(n) (~UINTMAX_C(0) >> (64 - (n)))
#define ALIGN(n) (((n) + sizeof(uint64_t)-1) & ~(sizeof(uint64_t)-1))

static void kv_sbox_init(unsigned seed);
static uint64_t kv_mod2n_injection(uint64_t x);
static size_t kvpair_bytes(size_t vsize);
static char* kv_pair(unsigned vsize, unsigned vage, uint64_t point, char* dst);

static struct {
	char debug, printable;
	unsigned ksize, width, nsectors;
	uint64_t period;
} kv_globals = {
	.debug = DEBUG_KEYGEN,
};

int ia_kvgen_setup(char printable, unsigned ksize, unsigned nspaces, unsigned nsectors, uintmax_t period, int seed)
{
	uint64_t top;
	unsigned width;

	double maxkey = (double) period * (double) nspaces;
	if (maxkey < 2) {
		return -1;
	} else if (maxkey < BITMASK(16)) {
		width = 16/8;
		top = BITMASK(16);
	} else if (maxkey < BITMASK(24)) {
		width = 24/8;
		top = BITMASK(24);
	} else if (maxkey < BITMASK(32)) {
		width = 32/8;
		top = BITMASK(32);
	} else if (maxkey < BITMASK(40)) {
		width = 40/8;
		top = BITMASK(40);
	} else if (maxkey < BITMASK(48)) {
		width = 48/8;
		top = BITMASK(48);
	 } else if (maxkey < BITMASK(56)) {
		width = 56/8;
		top = BITMASK(56);
	} else if (maxkey < UINT64_MAX) {
		width = 64/8;
		top = UINT64_MAX;
	} else {
		double width = log(maxkey) / log(2);
		ia_log("key-gen: %u sector of %ju items is too huge, unable provide by %u-bit arithmetics, at least %d required",
			nsectors, period, (unsigned) sizeof(uintmax_t) * 8, (int) ceil(width));
		return -1;
	}

	double bytes4maxkey = log(top) / log(printable ? ALPHABET_CARDINALITY : 256);
	if (bytes4maxkey > (double) ksize) {
		ia_log("key-gen: key-length %u is insufficient for %u sectors of %s %ju items, at least %d required",
			ksize, nsectors, printable ? "printable" : "binary", period, (int) ceil(bytes4maxkey));
		return -1;
	}

	ia_log("key-gen: using %u bits, up to %.0lf keys", width*8, maxkey);
	kv_globals.printable = printable;
	kv_globals.ksize = ksize;
	kv_globals.period = period;
	kv_globals.width = width;
	kv_globals.nsectors = nsectors;

	if (seed < 0)
		seed = time(NULL);
	kv_sbox_init(seed);
	return 0;
}

struct ia_kvgen {
	uint64_t base, serial;
	unsigned vsize, vage, pair_bytes;
	char buf[];
};

int ia_kvgen_init(struct ia_kvgen **genptr, unsigned kspace, unsigned ksector, unsigned vsize, unsigned vage)
{
	size_t pair_size = kvpair_bytes(vsize);
	struct ia_kvgen *gen = realloc(*genptr, sizeof(struct ia_kvgen) + pair_size);
	if (!gen)
		return -1;

	gen->base = kspace * kv_globals.period;
	gen->serial = 0;
	if (ksector) {
		gen->serial = kv_globals.period * (double) ksector / kv_globals.nsectors;
		gen->serial %= kv_globals.period;
	}
	gen->vsize = vsize;
	gen->vage = vage;
	gen->pair_bytes = pair_size;

	*genptr = gen;
	return 0;
}

void ia_kvgen_destroy(struct ia_kvgen **genptr)
{
	struct ia_kvgen *gen = *genptr;
	if (gen) {
		free(gen);
		*genptr = NULL;
	}
}

int ia_kvgen_get(struct ia_kvgen *gen, iakv *p, char key_only)
{
	p->k = gen->buf;
	p->ksize = kv_globals.ksize;
	p->vsize = 0;
	p->v = NULL;
	if (! key_only) {
		p->v = gen->buf + (kv_globals.printable ? p->ksize + 1 : ALIGN(p->ksize));
		p->vsize = gen->vsize;
	}

	uint64_t point = gen->base + gen->serial;
	gen->serial = (gen->serial + 1) % kv_globals.period;

	char* end = kv_pair(p->vsize, gen->vage, point, gen->buf);
	assert(end == gen->buf + kvpair_bytes(p->vsize));
	(void) end;
	return 0;
}

//-----------------------------------------------------------------------------

struct ia_kvpool {
	struct ia_kvgen *gen;
	char *pos, *end;
	char buf[];
};

int ia_kvpool_init(struct ia_kvpool **poolptr, struct ia_kvgen *gen, int pool_size)
{
	if (pool_size < 1 || pool_size > INT_MAX/2)
		return -1;

	size_t bytes = gen->pair_bytes * pool_size;
	struct ia_kvpool *pool = realloc(*poolptr, sizeof(struct ia_kvpool) + bytes);
	if (!pool)
		return -1;

	int i;
	char* dst = pool->buf;
	for(i = 0; i < pool_size; ++i) {
		dst = kv_pair(gen->vsize, gen->vage, gen->base + gen->serial, dst);
		gen->serial = (gen->serial + 1) % kv_globals.period;
	}

	pool->gen = gen;
	pool->pos = pool->buf;
	pool->end = dst;
	assert(dst == pool->buf + bytes);

	*poolptr = pool;
	return 0;
}

void ia_kvpool_destroy(struct ia_kvpool **pool)
{
	if (*pool) {
		free(*pool);
		*pool = NULL;
	}
}

int ia_kvpool_pull(struct ia_kvpool *pool, iakv *p)
{
	if (pool->end - pool->pos < pool->gen->pair_bytes)
		return -1;

	p->ksize = kv_globals.ksize;
	p->k = pool->pos;
	pool->pos += kv_globals.printable ? p->ksize + 1 : ALIGN(p->ksize);

	p->vsize = pool->gen->vsize;
	p->v = NULL;
	if (p->vsize > 0) {
		p->v = pool->pos;
		pool->pos += kv_globals.printable ? p->vsize + 1 : ALIGN(p->vsize);
	}

	return 0;
}

//-----------------------------------------------------------------------------

#define SBOX_SIZE 2048
static uint16_t sbox[SBOX_SIZE];

static void kv_sbox_init(unsigned seed)
{
	unsigned i;

	for(i = 0; i < SBOX_SIZE; ++i)
		sbox[i] = i;

	for(i = 0; i < SBOX_SIZE; ++i) {
		unsigned n = (rand_r(&seed) % 280583u) & (SBOX_SIZE - 1);
		unsigned x = sbox[i] ^ sbox[n];
		sbox[i] ^= x;
		sbox[n] ^= x;
	}

	for(i = 0; i < SBOX_SIZE; ++i)
		sbox[i] ^= i;
}

static uint64_t kv_mod2n_injection(uint64_t x)
{
	/* LY: magic 'fractal' prime, it has enough one-bits and prime by mod 2^{8,16,24,32,40,48,56,64} */
	x += UINTMAX_C(10042331536242289283);

	/* LY: stirs lower bits */
	x ^= sbox[x & (SBOX_SIZE - 1)];

	/* LY: https://en.wikipedia.org/wiki/Injective_function
	 * These "magic" prime numbers were found and verified with a bit of brute force.
	 */
	switch(kv_globals.width) {
		case 1: {
			uint8_t y = x;
			y ^= y >> 1;
			y *= 113u;
			y ^= y << 2;
			return y;
		}
		case 2: {
			uint16_t y = x;
			y ^= y >> 1;
			y *= 25693u;
			y ^= y << 7;
			return y;
		}
		case 3: {
			const uint32_t m = BITMASK(24);
			uint32_t y = x & m;
			y ^= y >> 1;
			y *= 5537317ul;
			y ^= y << 12;
			return y & m;
		}
		case 4: {
			uint32_t y = x;
			y ^= y >> 1;
			y *= 1923730889ul;
			y ^= y << 15;
			return y;
		}
		case 5: {
			const uint64_t m = BITMASK(40);
			uint64_t y = x & m;
			y ^= y >> 1;
			y *= UINTMAX_C(274992889273);
			y ^= y << 13;
			return y & m;
		}
		case 6: {
			const uint64_t m = BITMASK(48);
			uint64_t y = x & m;
			y ^= y >> 1;
			y *= UINTMAX_C(70375646670269);
			y ^= y << 15;
			return y & m;
		}
		case 7: {
			const uint64_t m = BITMASK(56);
			uint64_t y = x & m;
			y ^= y >> 1;
			y *= UINTMAX_C(23022548244171181);
			y ^= y << 4;
			return y & m;
		}
		default:
#if defined(__GNUC__) || defined(__clang__)
			__builtin_unreachable();
#endif
		case 8: {
			uint64_t y = x;
			y ^= y >> 1;
			y *= UINTMAX_C(4613509448041658233);
			y ^= y << 25;
			return y;
		}
	}
}

static size_t kvpair_bytes(size_t vsize)
{
	size_t bytes;
	if (kv_globals.printable) {
		bytes = kv_globals.ksize + ((vsize > 0) ? vsize + 2 : 1);
	} else {
		bytes = ALIGN(kv_globals.ksize) + ALIGN(vsize);
	}

	return bytes;
}

static __inline uint64_t remix4tail(uint64_t point)
{
	/* LY: fast and dirty remix */
	return point ^ (((point << 47) | (point >> 17)) + UINTMAX_C(7015912586649315971));
}

static char* kv_fill(uint64_t *point, char* dst, unsigned length)
{
	assert(length > 0);
	int left = kv_globals.width * 8;

	if (kv_globals.printable) {
		assert(ALPHABET_CARDINALITY == 64);
		uint64_t acc = *point;

		for(;;) {
			*dst++ = alphabet[acc & 63];
			if (--length == 0)
				break;
			acc >>= 6;
			left -= 6;
			if (left < 6) {
				acc = *point = remix4tail(*point + acc);
				left = kv_globals.width * 8;
			}
		}
		*dst++ = 0;
	} else {
		uint64_t *p = (void *) dst;
		for(;;) {
			*p++ = htole64(*point);
			length -= 8;
			if (length <= 0)
				break;
			do {
				*point = remix4tail(*point);
				left += left;
			} while (left < 64);
		}
		dst = (void *) p;
	}

	return dst;
}

static char* kv_pair(unsigned vsize, unsigned vage, uint64_t point, char* dst)
{
	if (! kv_globals.debug) {
		point = kv_mod2n_injection(point);
		dst = kv_fill(&point, dst, kv_globals.ksize);
	} else {
		if (kv_globals.printable) {
			dst += snprintf(dst, kv_globals.ksize + 1, "%0*" PRIu64, kv_globals.ksize, point) + 1;
		} else {
			dst += snprintf(dst, ALIGN(kv_globals.ksize), "%0*" PRIx64, (unsigned) ALIGN(kv_globals.ksize), point);
		}
	}

	if (vsize) {
		point = remix4tail(point + vage);
		dst = kv_fill(&point, dst, vsize);
	}
	return dst;
}
