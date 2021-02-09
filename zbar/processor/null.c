/*------------------------------------------------------------------------
 *  Copyright 2008-2009 (c) Jeff Brown <spadix@users.sourceforge.net>
 *
 *  This file is part of the ZBar Bar Code Reader.
 *
 *  The ZBar Bar Code Reader is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU Lesser Public License as
 *  published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  The ZBar Bar Code Reader is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 *  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser Public License
 *  along with the ZBar Bar Code Reader; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301  USA
 *
 *  http://sourceforge.net/projects/zbar
 *------------------------------------------------------------------------*/

#include "processor.h"

static inline int null_error (void *m,
                              const char *func)
{
    return(err_capture(m, SEV_ERROR, ZBAR_ERR_UNSUPPORTED, func,
                       "not compiled with output window support"));
}

int _zbar_processor_open (zbar_processor_t *proc,
                          char *name,
                          unsigned w,
                          unsigned h)
{
    return(null_error(proc, __func__));
}

int _zbar_processor_close (zbar_processor_t *proc)
{
    return(null_error(proc, __func__));
}

int _zbar_processor_set_visible (zbar_processor_t *proc,
                                 int vis)
{
    return(null_error(proc, __func__));
}

int _zbar_processor_set_size (zbar_processor_t *proc,
                              unsigned width,
                              unsigned height)
{
    return(null_error(proc, __func__));
}

int _zbar_processor_invalidate (zbar_processor_t *proc)
{
    return(null_error(proc, __func__));
}

int _zbar_event_wait (zbar_event_t *event,
                      zbar_mutex_t *lock,
                      zbar_timer_t *timeout)
{
    return(null_error(event, __func__));
}

int _zbar_processor_init (zbar_processor_t *proc)
{
    return(null_error(proc, __func__));
}

int _zbar_processor_input_wait (zbar_processor_t *proc,
                                zbar_event_t *event,
                                int timeout)
{
    return(null_error(proc, __func__));
}

void _zbar_event_trigger (zbar_event_t *event)
{
    null_error(event, __func__);
}

void _zbar_event_destroy (zbar_event_t *event)
{
    null_error(event, __func__);
}

int _zbar_thread_start (zbar_thread_t *thr,
                        zbar_thread_proc_t *proc,
                        void *arg,
                        zbar_mutex_t *lock)
{
    return(null_error(proc, __func__));
}

int _zbar_processor_enable (zbar_processor_t *proc)
{
    return(null_error(proc, __func__));
}

int _zbar_thread_stop (zbar_thread_t *thr,
                       zbar_mutex_t *lock)
{
    return(null_error(thr, __func__));
}

int _zbar_processor_cleanup (zbar_processor_t *proc)
{
    return(null_error(proc, __func__));
}

int _zbar_event_init (zbar_event_t *event)
{
    return(null_error(event, __func__));
}