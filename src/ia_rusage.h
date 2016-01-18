
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) Dmitry Simonenko
 * Copyright (c) Leonid Yuriev
 * BSD License
*/

typedef struct iarusage iarusage;

struct iarusage {
	uint64_t disk;
	size_t ram;
	uint64_t cpu_kernel_ns;
	uint64_t cpu_user_ns;
	uint64_t iops_read;
	uint64_t iops_write;
	uint64_t iops_page;
};

int ia_get_rusage(iarusage *, const char *datadir);
