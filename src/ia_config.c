
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

#include <ioarena.h>

int ia_configinit(iaconfig *c)
{
	c->driver = NULL;
	c->driver_if = NULL;
	c->path = strdup("./_ioarena");
	if (c->path == NULL)
		return -1;
	c->count = 1000000;
	c->ksize = 16;
	c->vsize = 32;
	c->csv   = 0;
	c->benchmark = strdup("set, get");
	if (c->benchmark == NULL) {
		free(c->path);
		c->path = NULL;
		return -1;
	}
	int i = 0;
	while (i < IA_MAX) {
		c->benchmark_list[i] = 0;
		i++;
	}
	return 0;
}

static inline void
ia_configusage(iaconfig *c)
{
	ia_log("usage: ioarena [hDBCpnkv]");
	ia_log("  -D <database_driver>");
	ia_log("     %s", ia_supported());
	ia_log("  -B <benchmarks>");
	ia_log("     set, get, delete, iterate");
	ia_log("     batch, transaction");
	ia_log("  -C generate csv                 ");
	ia_log("  -p <path>                   (%s)", c->path);
	ia_log("  -n <number_of_operations>   (%d)", c->count);
	ia_log("  -k <key_size>               (%d)", c->ksize);
	ia_log("  -v <value_size>             (%d)", c->vsize);
	ia_log("  -h                          help");
	ia_log("");
	ia_log("example:");
	ia_log("   ioarena -D sophia -T set,get -n 100000000");
}

int ia_configparse(iaconfig *c, int argc, char **argv)
{
	int opt;
	while ((opt = getopt(argc, argv, "hD:T:B:p:n:k:v:C")) != -1) {
		switch (opt) {
		case 'D':
			if (c->driver)
				free(c->driver);
			c->driver = strdup(optarg);
			if (c->driver == NULL)
				return -1;
			break;
		case 'T':
		case 'B':
			if (c->benchmark)
				free(c->benchmark);
			c->benchmark = strdup(optarg);
			if (c->benchmark == NULL)
				return -1;
			break;
		case 'p':
			if (c->path)
				free(c->path);
			c->path = strdup(optarg);
			if (c->path == NULL)
				return -1;
			break;
		case 'n':
			c->count = atoll(optarg);
			break;
		case 'k':
			c->ksize = atoi(optarg);
			break;
		case 'v':
			c->vsize = atoi(optarg);
			break;
		case 'C':
			c->csv = 1;
			break;
		case 'h':
			ia_configusage(c);
			return 1;
		default:
			ia_configusage(c);
			return -1;
		}
	}
	if (c->driver == NULL) {
		ia_configusage(c);
		return -1;
	}
	c->driver_if = ia_of(c->driver);
	if (c->driver_if == NULL) {
		ia_log("error: unknown database driver '%s'", c->driver);
		return -1;
	}
	if (c->ksize <= 0) {
		ia_log("error: bad key size");
		return -1;
	}
	char buf[128];
	snprintf(buf, sizeof(buf), "%s", c->benchmark);
	char *p;
	for (p = strtok(buf, ", ");
	     p;
	     p = strtok(NULL, ", "))
	{
		iabenchmark bench = ia_benchmark(p);
		if (bench == IA_MAX) {
			ia_log("error: unknown benchmark name '%s'", p);
			return -1;
		}
		c->benchmark_list[bench] = 1;
	}
	return 0;
}

void ia_configfree(iaconfig *c)
{
	if (c->path)
		free(c->path);
	if (c->driver)
		free(c->driver);
	if (c->benchmark)
		free(c->benchmark);
}

void ia_configprint(iaconfig *c)
{
	ia_log("configuration:");
	ia_log("  database:   %s", c->driver);
	ia_log("  output:     %s", c->path);
	ia_log("  benchmark:  %s", c->benchmark);
	ia_log("  operations: %"PRIu64, c->count);
	ia_log("  key size:   %d", c->ksize);
	ia_log("  value size: %d", c->vsize);
}
