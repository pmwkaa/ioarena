#ifndef IA_BENCHMARK_H_
#define IA_BENCHMARK_H_

/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef enum {
	IA_SET,
	IA_BATCH,
	IA_TRANSACTION,
	IA_DELETE,
	IA_ITERATE,
	IA_GET,
	IA_MAX
} iabenchmark;

static inline char*
ia_benchmarkof(iabenchmark b)
{
	switch (b) {
	case IA_SET:         return "set";
	case IA_GET:         return "get";
	case IA_DELETE:      return "delete";
	case IA_ITERATE:     return "iterate";
	case IA_BATCH:       return "batch";
	case IA_TRANSACTION: return "transaction";
	default: assert(0);
	}
	return NULL;
}

static inline iabenchmark
ia_benchmark(char *name)
{
	if (strcasecmp(name, "set") == 0)
		return IA_SET;
	else
	if (strcasecmp(name, "get") == 0)
		return IA_GET;
	else
	if (strcasecmp(name, "delete") == 0)
		return IA_DELETE;
	else
	if (strcasecmp(name, "iterate") == 0)
		return IA_ITERATE;
	else
	if (strcasecmp(name, "batch") == 0)
		return IA_BATCH;
	else
	if (strcasecmp(name, "transaction") == 0)
		return IA_TRANSACTION;
	return IA_MAX;
}

#endif
