
/*
 * ioarena: embedded storage benchmarking
 *
 * Copyright (c) ioarena authors
 * BSD License
*/

#include <ioarena.h>

/* LY: workaround for Mac OSX */
#if defined(__MACH__) && !(defined(CLOCK_MONOTONIC_RAW) || defined(CLOCK_MONOTONIC) || defined(CLOCK_REALTIME))
#	include <mach/clock.h>
#	include <mach/mach.h>

static clock_serv_t timestamp_clk;
static pthread_once_t timestamp_initonce = PTHREAD_ONCE_INIT;

static void ia_timestamp_shutdown(void)
{
	kern_return_t rc;

	rc = mach_port_deallocate(mach_task_self(), timestamp_clk);
	assert(rc == KERN_SUCCESS);
	(void) rc;
}

static void ia_timestamp_init(void)
{
	kern_return_t rc;

	rc = host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &timestamp_clk);
	assert(rc == KERN_SUCCESS);
	rc = atexit(ia_timestamp_shutdown);
	assert(rc == 0);
	(void) rc;
}

#endif /* workaround for Mac OSX */

ia_timestamp_t ia_timestamp_ns(void)
{
#if defined(CLOCK_MONOTONIC_RAW) || defined(CLOCK_MONOTONIC) || defined(CLOCK_REALTIME)
	/* LY: POSIX.1-2001 */

#	if defined(CLOCK_MONOTONIC_RAW)
#		define CLOCK_ID	CLOCK_MONOTONIC_RAW
#	elif defined(CLOCK_MONOTONIC)
#		define CLOCK_ID	CLOCK_MONOTONIC
#	else
#		define CLOCK_ID	CLOCK_REALTIME
#	endif
	struct timespec ts;
	int rc = clock_gettime(CLOCK_ID, &ts);
	assert(rc == 0);
	(void) rc;
	return ts.tv_sec * S + ts.tv_nsec;

#elif defined(__MACH__)
	/* LY: workaround for Mac OSX, had taken at https://gist.github.com/jbenet/1087739 */

	mach_timespec_t ts;
	kern_return_t rc;

	rc = pthread_once(&timestamp_initonce, ia_timestamp_init);
	assert(rc == 0);
	rc = clock_get_time(timestamp_clk, &ts);
	assert(rc == KERN_SUCCESS);
	(void) rc;
	return ts.tv_sec * S + ts.tv_nsec;

#else
	/* LY: fallback to gettimeofday() */

	struct timeval tv;
	int rc = gettimeofday(&tv, NULL);
	assert(rc == 0);
	(void) rc;
	return tv.tv_sec * S + tv.tv_usec * US;

#endif
}
