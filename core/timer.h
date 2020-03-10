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

#ifndef DC_TIMER_H
#define DC_TIMER_H

#include <libdivecomputer/common.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if defined (_WIN32) && !defined (__GNUC__)
typedef unsigned __int64 dc_usecs_t;
#else
typedef unsigned long long dc_usecs_t;
#endif

typedef struct dc_timer_t dc_timer_t;

dc_status_t
dc_timer_new (dc_timer_t **timer);

dc_status_t
dc_timer_now (dc_timer_t *timer, dc_usecs_t *usecs);

dc_status_t
dc_timer_free (dc_timer_t *timer);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* DC_TIMER_H */
