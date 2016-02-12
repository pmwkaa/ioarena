
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) Dmitry Simonenko
 * Copyright (c) Leonid Yuriev
 * BSD License
*/

#include <ioarena.h>

static __thread uintmax_t diskusage;

static int ftw_diskspace(const char *fpath, const struct stat *sb, int typeflag)
{
	(void) fpath;
	(void) typeflag;
	diskusage += sb->st_size;
	return 0;
}

int ia_get_rusage(iarusage *dst, const char *datadir)
{
	struct rusage glibc;

	if (getrusage(RUSAGE_SELF, &glibc))
		return -1;

	diskusage = 0;
	if (datadir && ftw(datadir, ftw_diskspace, 42))
		return -1;

	dst->disk = diskusage;
	dst->ram  = glibc.ru_maxrss;
	dst->cpu_kernel_ns = glibc.ru_stime.tv_sec * 1000000000ull
		+ glibc.ru_stime.tv_usec * 1000ull;
	dst->cpu_user_ns = glibc.ru_utime.tv_sec * 1000000000ull
		+ glibc.ru_utime.tv_usec * 1000ull;
	dst->iops_read = glibc.ru_inblock;
	dst->iops_write = glibc.ru_oublock;
	dst->iops_page = glibc.ru_majflt;

	return 0;
}
