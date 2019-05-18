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
#ifndef _MISC_H_
#define _MISC_H_

struct resolution_s {
    long cx;
    long cy;
};
typedef struct resolution_s resolution_t;

struct resolution_list_s {
    resolution_t *resolutions;
    long cnt;
};
typedef struct resolution_list_s resolution_list_t;

void resolution_list_init(resolution_list_t *list);
void resolution_list_cleanup(resolution_list_t *list);
void resolution_list_add(resolution_list_t *list, resolution_t *resolution);
/// Fill <code>resolution</code> with the closest resolution found in
/// <code>list</code>.
/** If <code>list</code> is empty,
  * the <code>resolution</code> is unchanged.
  * If <code>resolution</code> is empty,
  * the biggest resolution is chosen. */
void get_closest_resolution(resolution_t *resolution,
                            resolution_list_t *list);

/// Returns 1 if the struct is null, otherwise 0
int is_struct_null_fun(const void *pdata, const int len);
/// Returns 1 if the struct is null, otherwise 0
#define is_struct_null(pdata) is_struct_null_fun(pdata, sizeof(*pdata))

#endif
