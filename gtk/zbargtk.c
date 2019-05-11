/*------------------------------------------------------------------------
 *  Copyright 2008-2010 (c) Jeff Brown <spadix@users.sourceforge.net>
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

#include <gtk/gtk.h>
#ifdef HAVE_X
#include <gdk/gdkx.h>
#elif defined(_WIN32)
#include <gdk/gdkwin32.h>
#endif

#include <zbar/zbargtk.h>
#include "zbargtkprivate.h"
#include "zbarmarshal.h"

#ifndef G_PARAM_STATIC_STRINGS
# define G_PARAM_STATIC_STRINGS (G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB)
#endif

#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480

enum {
    DECODED,
    DECODED_TEXT,
    LAST_SIGNAL
};

enum {
    PROP_0,
    PROP_VIDEO_DEVICE,
    PROP_VIDEO_ENABLED,
    PROP_VIDEO_OPENED,
};

static guint zbar_gtk_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE(ZBarGtk, zbar_gtk, GTK_TYPE_WIDGET);

/* FIXME what todo w/errors? OOM? */
/* FIXME signal failure notifications to main gui idle handler */

void zbar_gtk_release_pixbuf (zbar_image_t *img)
{
    GdkPixbuf *pixbuf = zbar_image_get_userdata(img);
    g_assert(GDK_IS_PIXBUF(pixbuf));

    /* remove reference */
    zbar_image_set_userdata(img, NULL);

    /* release reference to associated pixbuf and it's data */
    g_object_unref(pixbuf);
}

gboolean zbar_gtk_image_from_pixbuf (zbar_image_t *zimg,
                                     GdkPixbuf *pixbuf)
{
    /* apparently should always be packed RGB? */
    GdkColorspace colorspace = gdk_pixbuf_get_colorspace(pixbuf);
    if(colorspace != GDK_COLORSPACE_RGB) {
        g_warning("non-RGB color space not supported: %d\n", colorspace);
        return(FALSE);
    }

    int nchannels = gdk_pixbuf_get_n_channels(pixbuf);
    int bps = gdk_pixbuf_get_bits_per_sample(pixbuf);
    long type = 0;

    /* these are all guesses... */
    if(nchannels == 3 && bps == 8)
        type = zbar_fourcc('R','G','B','3');
    else if(nchannels == 4 && bps == 8)
        type = zbar_fourcc('B','G','R','4'); /* FIXME alpha flipped?! */
    else if(nchannels == 1 && bps == 8)
        type = zbar_fourcc('Y','8','0','0');
    else if(nchannels == 3 && bps == 5)
        type = zbar_fourcc('R','G','B','R');
    else if(nchannels == 3 && bps == 4)
        type = zbar_fourcc('R','4','4','4'); /* FIXME maybe? */
    else {
        g_warning("unsupported combination: nchannels=%d bps=%d\n",
                  nchannels, bps);
        return(FALSE);
    }
    zbar_image_set_format(zimg, type);

    /* FIXME we don't deal w/bpl...
     * this will cause problems w/unpadded pixbufs :|
     */
    unsigned pitch = gdk_pixbuf_get_rowstride(pixbuf);
    unsigned width = pitch / ((nchannels * bps) / 8);
    if((width * nchannels * 8 / bps) != pitch) {
        g_warning("unsupported: width=%d nchannels=%d bps=%d rowstride=%d\n",
                  width, nchannels, bps, pitch);
        return(FALSE);
    }
    unsigned height = gdk_pixbuf_get_height(pixbuf);
    /* FIXME this isn't correct either */
    unsigned long datalen = width * height * nchannels;
    zbar_image_set_size(zimg, width, height);

    /* when the zbar image is released, the pixbuf will be
     * automatically be released
     */
    zbar_image_set_userdata(zimg, pixbuf);
    zbar_image_set_data(zimg, gdk_pixbuf_get_pixels(pixbuf), datalen,
                         zbar_gtk_release_pixbuf);
#ifdef DEBUG_ZBARGTK
    g_message("colorspace=%d nchannels=%d bps=%d type=%.4s(%08lx)\n"
              "\tpitch=%d width=%d height=%d datalen=0x%lx\n",
              colorspace, nchannels, bps, (char*)&type, type,
              pitch, width, height, datalen);
#endif
    return(TRUE);
}

static inline gboolean zbar_gtk_video_open (ZBarGtk *self,
                                            const char *video_device)
{
    ZBarGtkPrivate *zbar = ZBAR_GTK_PRIVATE(self->_private);
    gboolean video_opened = FALSE;

    zbar->video_opened = FALSE;
    if(zbar->idle_id)
        g_object_notify(G_OBJECT(self), "video-opened");

    if(zbar->window) {
        /* ensure old video doesn't have image ref
         * (FIXME handle video destroyed w/images outstanding)
         */
        zbar_window_draw(zbar->window, NULL);
        gtk_widget_queue_draw(GTK_WIDGET(self));
    }

    if(zbar->video) {
        zbar_video_destroy(zbar->video);
        zbar->video = NULL;
    }

    if(video_device && video_device[0] && zbar->idle_id) {
        /* create video
         * FIXME video should support re-open
         */
        zbar->video = zbar_video_create();
        g_assert(zbar->video);

        if(zbar_video_open(zbar->video, video_device)) {
            zbar_video_error_spew(zbar->video, 0);
            zbar_video_destroy(zbar->video);
            zbar->video = NULL;
            /* FIXME error propagation */
            return(FALSE);
        }

        /* negotiation accesses the window format list,
         * so we hold the lock for this part
         */
        if(zbar->video_width && zbar->video_height)
            zbar_video_request_size(zbar->video,
                                    zbar->video_width, zbar->video_height);

        video_opened = !zbar_negotiate_format(zbar->video, zbar->window);

        if(video_opened) {
            zbar->req_width = zbar_video_get_width(zbar->video);
            zbar->req_height = zbar_video_get_height(zbar->video);
        }
        gtk_widget_queue_resize(GTK_WIDGET(self));

        zbar->video_opened = video_opened;
        if(zbar->idle_id)
            g_object_notify(G_OBJECT(self), "video-opened");
    }

    return(video_opened);
}

static inline int zbar_gtk_process_image (ZBarGtk *self,
                                          zbar_image_t *image)
{
    ZBarGtkPrivate *zbar = ZBAR_GTK_PRIVATE(self->_private);

    if(!image)
        return(-1);

    zbar_image_t *tmp = zbar_image_convert(image, zbar_fourcc('Y','8','0','0'));
    if(!tmp)
        return(-1);

    zbar_image_scanner_recycle_image(zbar->scanner, image);
    int rc = zbar_scan_image(zbar->scanner, tmp);
    zbar_image_set_symbols(image, zbar_image_get_symbols(tmp));
    zbar_image_destroy(tmp);
    if(rc < 0)
        return(rc);

    if(rc && zbar->idle_id) {
        /* update decode results */
        const zbar_symbol_t *sym;
        for(sym = zbar_image_first_symbol(image);
            sym;
            sym = zbar_symbol_next(sym))
            if(!zbar_symbol_get_count(sym)) {
                zbar_symbol_type_t type = zbar_symbol_get_type(sym);
                const char *data = zbar_symbol_get_data(sym);
                g_signal_emit(self, zbar_gtk_signals[DECODED], 0,
                              type, data);

                /* FIXME skip this when unconnected? */
                gchar *text = g_strconcat(zbar_get_symbol_name(type),
                                          ":",
                                          data,
                                          NULL);
                g_signal_emit(self, zbar_gtk_signals[DECODED_TEXT], 0, text);
                g_free(text);
            }
    }

    if(zbar->window) {
        rc = zbar_window_draw(zbar->window, image);
        gtk_widget_queue_draw(GTK_WIDGET(self));
    }
    else
        rc = -1;

    return(rc);
}

static gboolean zbar_processing_idle_callback(gpointer data)
{
    ZBarGtk *self = data;
    ZBarGtkPrivate *zbar = ZBAR_GTK_PRIVATE(self->_private);

    GValue *msg = g_async_queue_try_pop(zbar->queue);
    if (!msg) {
        if (zbar->video_enabled_state) {
            zbar_image_t *image = zbar_video_next_image(zbar->video);
            if(zbar_gtk_process_image(self, image) < 0)
                zbar->video_enabled_state = FALSE;
            if(image)
                zbar_image_destroy(image);

            if (zbar->video_enabled_state)
                return TRUE;

            if(zbar_video_enable(zbar->video, 0)) {
                zbar_video_error_spew(zbar->video, 0);
                zbar->video_enabled_state = FALSE;
            }

            zbar_image_scanner_enable_cache(zbar->scanner, 0);
            /* release video image and revert to logo */
            if(zbar->window) {
                    zbar_window_draw(zbar->window, NULL);
                    gtk_widget_queue_draw(GTK_WIDGET(self));
            }

            /* must have been an error while streaming */
            zbar_gtk_video_open(self, NULL);
        }

        return TRUE;
    }

    g_assert(G_IS_VALUE(msg));

    GType type = G_VALUE_TYPE(msg);
    if(type == G_TYPE_INT) {
        /* video state change */
        int state = g_value_get_int(msg);
        if(state < 0) {
            /* error identifying state */
            g_value_unset(msg);
            g_free(msg);
	    return TRUE;
        }
        g_assert(state >= 0 && state <= 1);
        zbar->video_enabled_state = (state != 0);
    } else if(type == G_TYPE_STRING) {
        /* open new video device */
        const char *video_device = g_value_get_string(msg);
        zbar->video_enabled_state = zbar_gtk_video_open(self, video_device);
    } else if(type == GDK_TYPE_PIXBUF) {
        /* scan provided image and broadcast results */
        zbar_image_t *image = zbar_image_create();
        GdkPixbuf *pixbuf = GDK_PIXBUF(g_value_dup_object(msg));
        if(zbar_gtk_image_from_pixbuf(image, pixbuf))
            zbar_gtk_process_image(self, image);
        else
            g_object_unref(pixbuf);
        zbar_image_destroy(image);
    } else {
        gchar *dbg = g_strdup_value_contents(msg);
        g_warning("unknown message type (%x) received: %s\n",
                  (unsigned)type, dbg);
        g_free(dbg);
    }
    g_value_unset(msg);
    g_free(msg);
    msg = NULL;

    if(zbar->video_enabled_state) {
        /* release reference to any previous pixbuf */
        if(zbar->window)
            zbar_window_draw(zbar->window, NULL);

        if(zbar_video_enable(zbar->video, 1)) {
            zbar_video_error_spew(zbar->video, 0);
            zbar->video_enabled_state = FALSE;
            return TRUE;
        }
        zbar_image_scanner_enable_cache(zbar->scanner, 1);

        return TRUE;
    }

    return TRUE;
}

static void zbar_gtk_realize (GtkWidget *widget)
{
    ZBarGtk *self = ZBAR_GTK(widget);
    if(!self->_private)
        return;
    ZBarGtkPrivate *zbar = ZBAR_GTK_PRIVATE(self->_private);

    gtk_widget_set_realized(widget, TRUE);

#if GTK_MAJOR_VERSION >= 3 && GTK_MINOR_VERSION >= 14
    // FIXME: this is deprecated after 3.14 - no idea what replaces it
#else
    gtk_widget_set_double_buffered(widget, FALSE);
#endif
    GdkWindowAttr attributes;
    GtkAllocation allocation;

    gtk_widget_get_allocation(widget, &allocation);
    attributes.x = allocation.x;
    attributes.y = allocation.y;
    attributes.width = allocation.width;
    attributes.height = allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.event_mask = (gtk_widget_get_events(widget) |
                             GDK_EXPOSURE_MASK);

    GdkWindow *window = gdk_window_new(gtk_widget_get_parent_window(widget),
                                       &attributes,
                                       GDK_WA_X | GDK_WA_Y);
    gtk_widget_set_window(widget, window);
    gdk_window_set_user_data(window, widget);
#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION < 18
    gdk_window_set_background_pattern(window, NULL);
#elif GTK_MAJOR_VERSION < 3
    gdk_window_set_back_pixmap(window, NULL, TRUE);
#endif

    /* attach zbar_window to underlying X window */
#ifdef HAVE_X
    if(zbar_window_attach(zbar->window,
                           GDK_DISPLAY_XDISPLAY(gdk_display_get_default()),
                           GDK_WINDOW_XID(window)))
#elif defined(_WIN32)
    if(zbar_window_attach(zbar->window,
                           GDK_WINDOW_HWND (window),
                           0))
#endif
    zbar_window_error_spew(zbar->window, 0);
}

static inline GValue *zbar_gtk_new_value (GType type)
{
    return(g_value_init(g_malloc0(sizeof(GValue)), type));
}

static void zbar_gtk_unrealize (GtkWidget *widget)
{
    GdkWindow *window;

    if(gtk_widget_get_mapped(widget))
        gtk_widget_unmap(widget);

    gtk_widget_set_mapped(widget, FALSE);
    ZBarGtk *self = ZBAR_GTK(widget);
    if(!self->_private)
        return;
    ZBarGtkPrivate *zbar = ZBAR_GTK_PRIVATE(self->_private);

    if(zbar->video_enabled) {
        zbar->video_enabled = FALSE;
        GValue *msg = zbar_gtk_new_value(G_TYPE_INT);
        g_value_set_int(msg, 0);
        g_async_queue_push(zbar->queue, msg);
    }

    zbar_window_attach(zbar->window, NULL, 0);

    gtk_widget_set_realized(widget, FALSE);

    window = gtk_widget_get_window(widget);
    gdk_window_set_user_data(window, NULL);
    gdk_window_destroy(window);
    gtk_widget_set_window(widget, NULL);
}

#if GTK_MAJOR_VERSION >= 3

static void zbar_get_preferred_width(GtkWidget *widget,
                                     gint      *minimum_width,
                                     gint      *natural_width)
{
    ZBarGtk *self = ZBAR_GTK(widget);
    if(!self->_private)
        return;
    ZBarGtkPrivate *zbar = ZBAR_GTK_PRIVATE(self->_private);

    /* use native video size (max) if available,
     * arbitrary defaults otherwise.
     * video attributes maintained under main gui idle handler
     */
#if GTK_MAJOR_VERSION >= 3 && GTK_MINOR_VERSION >= 22
    GdkRectangle geo;

    GdkDisplay *display = gdk_display_get_default();
    GdkMonitor *monitor = gdk_display_get_monitor(display, 0);
    gdk_monitor_get_geometry(monitor, &geo);

    unsigned int screen_width  = geo.width;
#else
    unsigned int screen_width  = gdk_screen_width();
#endif

    if (zbar->req_width > screen_width) {
         float scale = screen_width * .8 / zbar->req_width;

         zbar->req_width *= scale;
         zbar->req_height *= scale;
    }

    *minimum_width = zbar->req_width;
    *natural_width = zbar->req_width;
}

static void zbar_get_preferred_height(GtkWidget *widget,
                                      gint      *minimum_height,
                                      gint      *natural_height)
{
    ZBarGtk *self = ZBAR_GTK(widget);
    if(!self->_private)
        return;
    ZBarGtkPrivate *zbar = ZBAR_GTK_PRIVATE(self->_private);

    /* use native video size (max) if available,
     * arbitrary defaults otherwise.
     * video attributes maintained under main gui idle handler
     */
#if GTK_MAJOR_VERSION >= 3 && GTK_MINOR_VERSION >= 22
    GdkRectangle geo;

    GdkDisplay *display = gdk_display_get_default();
    GdkMonitor *monitor = gdk_display_get_monitor(display, 0);
    gdk_monitor_get_geometry(monitor, &geo);

    unsigned int screen_height  = geo.height;
#else
    unsigned int screen_height  = gdk_screen_height();
#endif

    if (zbar->req_height > screen_height) {
         float scale = screen_height * .8 / zbar->req_height;

         zbar->req_width *= scale;
         zbar->req_height *= scale;
    }

    *minimum_height = zbar->req_height;
    *natural_height = zbar->req_height;
}

static gboolean zbar_gtk_scale_draw(GtkWidget *widget, cairo_t   *cr)
{
    // NOTE: should we change something here?

    ZBarGtk *self = ZBAR_GTK(widget);
    if(!self->_private)
        return(FALSE);
    ZBarGtkPrivate *zbar = ZBAR_GTK_PRIVATE(self->_private);

    if(gtk_widget_get_visible(widget) &&
       gtk_widget_get_mapped(widget) &&
       zbar_window_redraw(zbar->window))
        return(TRUE);
    return(FALSE);
}

#else
static void zbar_gtk_size_request (GtkWidget *widget,
                                   GtkRequisition *requisition)
{
    ZBarGtk *self = ZBAR_GTK(widget);
    if(!self->_private)
        return;
    ZBarGtkPrivate *zbar = ZBAR_GTK_PRIVATE(self->_private);

    /* use native video size (max) if available,
     * arbitrary defaults otherwise.
     * video attributes maintained under main gui idle handler
     */
    requisition->width = zbar->req_width;
    requisition->height = zbar->req_height;
}

static gboolean zbar_gtk_expose (GtkWidget *widget,
                                 GdkEventExpose *event)
{
    ZBarGtk *self = ZBAR_GTK(widget);
    if(!self->_private)
        return(FALSE);
    ZBarGtkPrivate *zbar = ZBAR_GTK_PRIVATE(self->_private);

    if(gtk_widget_get_visible(widget) &&
       gtk_widget_get_mapped(widget) &&
       zbar_window_redraw(zbar->window))
        return(TRUE);
    return(FALSE);
}
#endif

static void zbar_gtk_size_allocate (GtkWidget *widget,
                                    GtkAllocation *allocation)
{
    ZBarGtk *self = ZBAR_GTK(widget);
    if(!self->_private)
        return;
    ZBarGtkPrivate *zbar = ZBAR_GTK_PRIVATE(self->_private);

    (*GTK_WIDGET_CLASS(zbar_gtk_parent_class)->size_allocate)
        (widget, allocation);
    if(zbar->window)
        zbar_window_resize(zbar->window,
                            allocation->width, allocation->height);
}

void zbar_gtk_scan_image (ZBarGtk *self,
                          GdkPixbuf *img)
{
    if(!self->_private)
        return;
    ZBarGtkPrivate *zbar = ZBAR_GTK_PRIVATE(self->_private);

    g_object_ref(G_OBJECT(img));

    /* queue for scanning by the processor idle handler */
    GValue *msg = zbar_gtk_new_value(GDK_TYPE_PIXBUF);

    /* this grabs a new reference to the image,
     * eventually released by the processor idle handler
     */
    g_value_set_object(msg, img);
    g_async_queue_push(zbar->queue, msg);
}


const char *zbar_gtk_get_video_device (ZBarGtk *self)
{
    if(!self->_private)
        return(NULL);
    ZBarGtkPrivate *zbar = ZBAR_GTK_PRIVATE(self->_private);
    if(zbar->video_device)
        return(zbar->video_device);
    else
        return("");
}

void zbar_gtk_set_video_device (ZBarGtk *self,
                                const char *video_device)
{
    if(!self->_private)
        return;
    ZBarGtkPrivate *zbar = ZBAR_GTK_PRIVATE(self->_private);

    g_free((void*)zbar->video_device);
    zbar->video_device = g_strdup(video_device);
    zbar->video_enabled = video_device && video_device[0];

    /* push another copy to processor idle handler */
    GValue *msg = zbar_gtk_new_value(G_TYPE_STRING);
    if(video_device)
        g_value_set_string(msg, video_device);
    else
        g_value_set_static_string(msg, "");
    g_async_queue_push(zbar->queue, msg);

    g_object_freeze_notify(G_OBJECT(self));
    g_object_notify(G_OBJECT(self), "video-device");
    g_object_notify(G_OBJECT(self), "video-enabled");
    g_object_thaw_notify(G_OBJECT(self));
}

gboolean zbar_gtk_get_video_enabled (ZBarGtk *self)
{
    if(!self->_private)
        return(FALSE);
    ZBarGtkPrivate *zbar = ZBAR_GTK_PRIVATE(self->_private);
    return(zbar->video_enabled);
}

void zbar_gtk_set_video_enabled (ZBarGtk *self,
                                 gboolean video_enabled)
{
    if(!self->_private)
        return;
    ZBarGtkPrivate *zbar = ZBAR_GTK_PRIVATE(self->_private);

    video_enabled = (video_enabled != FALSE);
    if(zbar->video_enabled != video_enabled) {
        zbar->video_enabled = video_enabled;

        /* push state change to processor idle handler */
        GValue *msg = zbar_gtk_new_value(G_TYPE_INT);
        g_value_set_int(msg, zbar->video_enabled);
        g_async_queue_push(zbar->queue, msg);

        g_object_notify(G_OBJECT(self), "video-enabled");
    }
}

gboolean zbar_gtk_get_video_opened (ZBarGtk *self)
{
    if(!self->_private)
        return(FALSE);
    ZBarGtkPrivate *zbar = ZBAR_GTK_PRIVATE(self->_private);

    return(zbar->video_opened);
}

void zbar_gtk_request_video_size (ZBarGtk *self,
                                  int width,
                                  int height)
{
    if(!self->_private || width < 0 || height < 0)
        return;
    ZBarGtkPrivate *zbar = ZBAR_GTK_PRIVATE(self->_private);

    zbar->req_width = zbar->video_width = width;
    zbar->req_height = zbar->video_height = height;
    gtk_widget_queue_resize(GTK_WIDGET(self));
}

static void zbar_gtk_set_property (GObject *object,
                                   guint prop_id,
                                   const GValue *value,
                                   GParamSpec *pspec)
{
    ZBarGtk *self = ZBAR_GTK(object);
    switch(prop_id) {
    case PROP_VIDEO_DEVICE:
        zbar_gtk_set_video_device(self, g_value_get_string(value));
        break;
    case PROP_VIDEO_ENABLED:
        zbar_gtk_set_video_enabled(self, g_value_get_boolean(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void zbar_gtk_get_property (GObject *object,
                                   guint prop_id,
                                   GValue *value,
                                   GParamSpec *pspec)
{
    ZBarGtk *self = ZBAR_GTK(object);
    if(!self->_private)
        return;
    ZBarGtkPrivate *zbar = ZBAR_GTK_PRIVATE(self->_private);

    switch(prop_id) {
    case PROP_VIDEO_DEVICE:
        if(zbar->video_device)
            g_value_set_string(value, zbar->video_device);
        else
            g_value_set_static_string(value, "");
        break;
    case PROP_VIDEO_ENABLED:
        g_value_set_boolean(value, zbar->video_enabled);
        break;
    case PROP_VIDEO_OPENED:
        g_value_set_boolean(value, zbar->video_opened);
	break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void zbar_gtk_init (ZBarGtk *self)
{
    ZBarGtkPrivate *zbar = g_object_new(ZBAR_TYPE_GTK_PRIVATE, NULL);
    self->_private = (void*)zbar;

    zbar->window = zbar_window_create();
    g_assert(zbar->window);

    zbar->req_width = zbar->video_width = DEFAULT_WIDTH;
    zbar->req_height = zbar->video_width = DEFAULT_HEIGHT;

    /* use a queue to signalize about the need to handle decoding and video */
    zbar->queue = g_async_queue_new();
    zbar->idle_id = g_idle_add(zbar_processing_idle_callback, self);
    zbar->video_enabled_state = FALSE;

    /* Prepare for idle handler */
    g_object_ref(zbar);
    g_assert(zbar->queue);
    g_async_queue_ref(zbar->queue);

    zbar->scanner = zbar_image_scanner_create();
    g_assert(zbar->scanner);
}

static void zbar_gtk_dispose (GObject *object)
{


    ZBarGtk *self = ZBAR_GTK(object);
    if(!self->_private)
        return;

    ZBarGtkPrivate *zbar = ZBAR_GTK_PRIVATE(self->_private);
    self->_private = NULL;

    g_free((void*)zbar->video_device);
    zbar->video_device = NULL;

    /* signal processor idle handler to exit */
    GValue *msg = zbar_gtk_new_value(G_TYPE_INT);
    g_value_set_int(msg, -1);
    g_async_queue_push(zbar->queue, msg);
    zbar->idle_id = 0;

    /* there are no external references which might call other APIs */
    g_async_queue_unref(zbar->queue);

    g_object_unref(G_OBJECT(zbar));
}

static void zbar_gtk_private_finalize (GObject *object)
{
    ZBarGtkPrivate *zbar = ZBAR_GTK_PRIVATE(object);

    if (zbar->idle_id) {
        if(zbar->window)
            zbar_window_draw(zbar->window, NULL);
        g_object_unref(zbar);
        g_source_remove(zbar->idle_id);
        zbar->idle_id = 0;
    }

    if(zbar->window) {
        zbar_window_destroy(zbar->window);
        zbar->window = NULL;
    }
    if(zbar->scanner) {
        zbar_image_scanner_destroy(zbar->scanner);
        zbar->scanner = NULL;
    }
    if(zbar->video) {
        zbar_video_destroy(zbar->video);
        zbar->video = NULL;
    }
    g_async_queue_unref(zbar->queue);
    zbar->queue = NULL;
}

static void zbar_gtk_class_init (ZBarGtkClass *klass)
{
    zbar_gtk_parent_class = g_type_class_peek_parent(klass);

    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = zbar_gtk_dispose;
    object_class->set_property = zbar_gtk_set_property;
    object_class->get_property = zbar_gtk_get_property;

    GtkWidgetClass *widget_class = (GtkWidgetClass*)klass;
    widget_class->realize = zbar_gtk_realize;
    widget_class->unrealize = zbar_gtk_unrealize;
#if GTK_MAJOR_VERSION >= 3
    widget_class->get_preferred_width  = zbar_get_preferred_width;
    widget_class->get_preferred_height = zbar_get_preferred_height;
    widget_class->draw                 = zbar_gtk_scale_draw;
#else
    widget_class->size_request = zbar_gtk_size_request;
    widget_class->expose_event = zbar_gtk_expose;
#endif
    widget_class->size_allocate = zbar_gtk_size_allocate;

    widget_class->unmap = NULL;

    zbar_gtk_signals[DECODED] =
        g_signal_new("decoded",
                     G_TYPE_FROM_CLASS(object_class),
                     G_SIGNAL_RUN_CLEANUP,
                     G_STRUCT_OFFSET(ZBarGtkClass, decoded),
                     NULL, NULL,
                     zbar_marshal_VOID__INT_STRING,
                     G_TYPE_NONE, 2,
                     G_TYPE_INT, G_TYPE_STRING);

    zbar_gtk_signals[DECODED_TEXT] =
        g_signal_new("decoded-text",
                     G_TYPE_FROM_CLASS(object_class),
                     G_SIGNAL_RUN_CLEANUP,
                     G_STRUCT_OFFSET(ZBarGtkClass, decoded_text),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__STRING,
                     G_TYPE_NONE, 1,
                     G_TYPE_STRING);

    GParamSpec *p = g_param_spec_string("video-device",
        "Video device",
        "the platform specific name of the video device",
        NULL,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(object_class, PROP_VIDEO_DEVICE, p);

    p = g_param_spec_boolean("video-enabled",
        "Video enabled",
        "controls streaming from the video device",
        FALSE,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(object_class, PROP_VIDEO_ENABLED, p);

    p = g_param_spec_boolean("video-opened",
        "Video opened",
        "current opened state of the video device",
        FALSE,
        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(object_class, PROP_VIDEO_OPENED, p);
}

static void zbar_gtk_private_class_init (ZBarGtkPrivateClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = zbar_gtk_private_finalize;
}

static GType zbar_gtk_private_get_type (void)
{
    static GType type = 0;
    if(!type) {
        static const GTypeInfo info = {
            sizeof(ZBarGtkPrivateClass),
            NULL, NULL,
            (GClassInitFunc)zbar_gtk_private_class_init,
            NULL, NULL,
            sizeof(ZBarGtkPrivate),
        };
        type = g_type_register_static(G_TYPE_OBJECT, "ZBarGtkPrivate",
                                      &info, 0);
    }
    return(type);
}

GtkWidget *zbar_gtk_new (void)
{
    return(GTK_WIDGET(g_object_new(ZBAR_TYPE_GTK, NULL)));
}
