
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

#include <ioarena.h>

void ia_vlog(char *fmt, va_list args)
{
	vprintf(fmt, args);
	printf("\n");
	fflush(NULL);
}
