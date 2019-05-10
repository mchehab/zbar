/*------------------------------------------------------------------------
 *  Copyright 2019 (c) Mauro Carvalho Chehab <mchehab+samsung@kernel.org>
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
 *------------------------------------------------------------------------*/

#include <gtk/gtk.h>
#include <zbar/zbargtk.h>
#include <girepository.h>

int main (int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    /* Support for GObject Introspection */
    GOptionContext *ctx;
    GError *error = NULL;

    ctx = g_option_context_new(NULL);
    g_option_context_add_group(ctx, g_irepository_get_option_group());

    if (!g_option_context_parse(ctx, &argc, &argv, &error)) {
        g_print ("zbarcam-gtk: %s\n", error->message);
        return 1;
    }
    return(-1);
}
