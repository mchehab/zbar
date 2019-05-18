/*------------------------------------------------------------------------
 *  Copyright 2012 (c) Jarek Czekalski <jarekczek@poczta.onet.pl>
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

#include "misc.h"
#include "error.h"

static int module_initialized = 0;
static errinfo_t err;

static void module_init()
{
    if (module_initialized)
        return;
    err_init(&err, ZBAR_MOD_UNKNOWN);
    module_initialized = 1;
}

void resolution_list_init(resolution_list_t *list)
{
    module_init();
    list->cnt = 0;
    // an empty list consists of 1 zeroed element
    list->resolutions = calloc(1, sizeof(resolution_t));
    if (!list->resolutions)
    {
        err_capture(&err, SEV_FATAL, ZBAR_ERR_NOMEM,
            __func__, "allocating resources");
    }
}

void resolution_list_cleanup(resolution_list_t *list)
{
    free(list->resolutions);
}

void resolution_list_add(resolution_list_t *list, resolution_t *resolution)
{
    list->cnt++;
    list->resolutions = realloc(list->resolutions,
                                (list->cnt + 1) * sizeof(resolution_t));
    if (!list->resolutions)
    {
        err_capture(&err, SEV_FATAL, ZBAR_ERR_NOMEM,
            __func__, "allocating resources");
    }
    list->resolutions[list->cnt - 1] = *resolution;
    memset(&list->resolutions[list->cnt], 0, sizeof(resolution_t));
}

void get_closest_resolution(resolution_t *resolution, resolution_list_t *list)
{
    resolution_t *test_res;
    long min_diff = 0;
    long idx_best = -1; // the index of best resolution in resolutions
    int i = 0;
    for (test_res = list->resolutions; !is_struct_null(test_res); test_res++)
    {
        long diff;
        if (resolution->cx)
        {
            diff = test_res->cx - resolution->cx;
            if (diff < 0)
                diff = -diff;
        }
        else
        {
            // empty resolution, looking for the biggest
            diff = -test_res->cx;
        }
        if (idx_best < 0 || diff < min_diff)
        {
            idx_best = i;
            min_diff = diff;
        }
        i++;
    }

    if (idx_best >= 0)
    {
        *resolution = list->resolutions[idx_best];
    }
}

int is_struct_null_fun(const void *pdata, const int len)
{
    int i;
    for (i = 0; i < len; i++)
    {
        if (((char*)pdata)[i] != 0)
        {
            return 0;
        }
    }
    return 1;
}
