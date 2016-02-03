
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) Dmitry Simonenko
 * Copyright (c) Leonid Yuriev
 * BSD License
*/

typedef struct iarusage iarusage;

struct iarusage {
	uintmax_t disk;
	size_t ram;
	uintmax_t cpu_kernel_ns;
	uintmax_t cpu_user_ns;
	uintmax_t iops_read;
	uintmax_t iops_write;
	uintmax_t iops_page;
};

int ia_get_rusage(iarusage *, const char *datadir);
