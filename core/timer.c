/*
 * libdivecomputer
 *
 * Copyright (C) 2018 Jef Driesen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#ifdef _WIN32
#define NOGDI
#include <windows.h>
#else
#include <time.h>
#include <sys/time.h>
#ifdef HAVE_MACH_MACH_TIME_H
#include <mach/mach_time.h>
#endif
#endif

#include "timer.h"

struct dc_timer_t {
#if defined (_WIN32)
	LARGE_INTEGER timestamp;
	LARGE_INTEGER frequency;
#elif defined (HAVE_CLOCK_GETTIME)
	struct timespec timestamp;
#elif defined (HAVE_MACH_ABSOLUTE_TIME)
	uint64_t timestamp;
	mach_timebase_info_data_t info;
#else
	struct timeval timestamp;
#endif
};

dc_status_t
dc_timer_new (dc_timer_t **out)
{
	dc_timer_t *timer = NULL;

	if (out == NULL)
		return DC_STATUS_INVALIDARGS;

	timer = (dc_timer_t *) malloc (sizeof (dc_timer_t));
	if (timer == NULL) {
		return DC_STATUS_NOMEMORY;
	}

#if defined (_WIN32)
	if (!QueryPerformanceFrequency(&timer->frequency) ||
		!QueryPerformanceCounter(&timer->timestamp)) {
		free(timer);
		return DC_STATUS_IO;
	}
#elif defined (HAVE_CLOCK_GETTIME)
	if (clock_gettime(CLOCK_MONOTONIC, &timer->timestamp) != 0) {
		free(timer);
		return DC_STATUS_IO;
	}
#elif defined (HAVE_MACH_ABSOLUTE_TIME)
	if (mach_timebase_info(&timer->info) != KERN_SUCCESS) {
		free(timer);
		return DC_STATUS_IO;
	}

	timer->timestamp = mach_absolute_time();
#else
	if (gettimeofday (&timer->timestamp, NULL) != 0) {
		free(timer);
		return DC_STATUS_IO;
	}
#endif

	*out = timer;

	return DC_STATUS_SUCCESS;
}

dc_status_t
dc_timer_now (dc_timer_t *timer, dc_usecs_t *usecs)
{
	dc_status_t status = DC_STATUS_SUCCESS;
	dc_usecs_t value = 0;

	if (timer == NULL) {
		status = DC_STATUS_INVALIDARGS;
		goto out;
	}

#if defined (_WIN32)
	LARGE_INTEGER now;
	if (!QueryPerformanceCounter(&now)) {
		status = DC_STATUS_IO;
		goto out;
	}

	value = (now.QuadPart - timer->timestamp.QuadPart) * 1000000 / timer->frequency.QuadPart;
#elif defined (HAVE_CLOCK_GETTIME)
	struct timespec now, delta;
	if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
		status = DC_STATUS_IO;
		goto out;
	}

	if (now.tv_nsec < timer->timestamp.tv_nsec) {
		delta.tv_nsec = 1000000000 + now.tv_nsec - timer->timestamp.tv_nsec;
		delta.tv_sec = now.tv_sec - timer->timestamp.tv_sec - 1;
	} else {
		delta.tv_nsec = now.tv_nsec - timer->timestamp.tv_nsec;
		delta.tv_sec = now.tv_sec - timer->timestamp.tv_sec;
	}

	value = (dc_usecs_t) delta.tv_sec * 1000000 + delta.tv_nsec / 1000;
#elif defined (HAVE_MACH_ABSOLUTE_TIME)
	uint64_t now = mach_absolute_time();
	value = (now - timer->timestamp) * timer->info.numer / (timer->info.denom * 1000);
#else
	struct timeval now, delta;
	if (gettimeofday (&now, NULL) != 0) {
		status = DC_STATUS_IO;
		goto out;
	}

	timersub (&now, &timer->timestamp, &delta);

	value = (dc_usecs_t) delta.tv_sec * 1000000 + delta.tv_usec;
#endif

out:
	if (usecs)
		*usecs = value;

	return status;
}

dc_status_t
dc_timer_free (dc_timer_t *timer)
{
	free (timer);

	return DC_STATUS_SUCCESS;
}
