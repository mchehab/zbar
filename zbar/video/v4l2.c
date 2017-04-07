/*------------------------------------------------------------------------
 *  Copyright 2007-2009 (c) Jeff Brown <spadix@users.sourceforge.net>
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

#include <config.h>
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#ifdef HAVE_LIBV4L2_H
# include <fcntl.h>
# include <libv4l2.h>
#else
# define v4l2_close close
# define v4l2_ioctl ioctl
# define v4l2_mmap mmap
# define v4l2_munmap munmap
#endif
#include <linux/videodev2.h>

#include "video.h"
#include "image.h"

#define V4L2_FORMATS_MAX 64
#define V4L2_FORMATS_SIZE_MAX 256

typedef enum v4l2_control_type_e {
    V4L2_CTRL_USER,
    V4L2_CTRL_EXT
} v4l2_control_type_t;

typedef enum v4l2_value_type_e {
    V4L2_VALUE_INT,
    V4L2_VALUE_BOOL
} v4l2_value_type_t;

typedef struct video_controls_priv_s {
    struct video_controls_s s;

    // Private fields
    __u32 id;
} video_controls_priv_t;

static int v4l2_nq (zbar_video_t *vdo,
                    zbar_image_t *img)
{
    if(vdo->iomode == VIDEO_READWRITE)
        return(video_nq_image(vdo, img));

    if(video_unlock(vdo))
        return(-1);

    struct v4l2_buffer vbuf;
    memset(&vbuf, 0, sizeof(vbuf));
    vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(vdo->iomode == VIDEO_MMAP) {
        vbuf.memory = V4L2_MEMORY_MMAP;
        vbuf.index = img->srcidx;
    }
    else {
        vbuf.memory = V4L2_MEMORY_USERPTR;
        vbuf.m.userptr = (unsigned long)img->data;
        vbuf.length = img->datalen;
        vbuf.index = img->srcidx; /* FIXME workaround broken drivers */
    }
    if(v4l2_ioctl(vdo->fd, VIDIOC_QBUF, &vbuf) < 0)
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_SYSTEM, __func__,
                           "queuing video buffer (VIDIOC_QBUF)"));
    return(0);
}

static zbar_image_t *v4l2_dq (zbar_video_t *vdo)
{
    zbar_image_t *img;
    int fd = vdo->fd;

    if(vdo->iomode != VIDEO_READWRITE) {
        video_iomode_t iomode = vdo->iomode;
        if(video_unlock(vdo))
            return(NULL);

        struct v4l2_buffer vbuf;
        memset(&vbuf, 0, sizeof(vbuf));
        vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if(iomode == VIDEO_MMAP)
            vbuf.memory = V4L2_MEMORY_MMAP;
        else
            vbuf.memory = V4L2_MEMORY_USERPTR;

        if(v4l2_ioctl(fd, VIDIOC_DQBUF, &vbuf) < 0)
            return(NULL);

        if(iomode == VIDEO_MMAP) {
            assert(vbuf.index >= 0);
            assert(vbuf.index < vdo->num_images);
            img = vdo->images[vbuf.index];
        }
        else {
            /* reverse map pointer back to image (FIXME) */
            assert(vbuf.m.userptr >= (unsigned long)vdo->buf);
            assert(vbuf.m.userptr < (unsigned long)(vdo->buf + vdo->buflen));
            int i = (vbuf.m.userptr - (unsigned long)vdo->buf) / vdo->datalen;
            assert(i >= 0);
            assert(i < vdo->num_images);
            img = vdo->images[i];
            assert(vbuf.m.userptr == (unsigned long)img->data);
        }
    }
    else {
        img = video_dq_image(vdo);
        if(!img)
            return(NULL);

        /* FIXME should read entire image */
        unsigned long datalen = read(fd, (void*)img->data, img->datalen);
        if(datalen < 0)
            return(NULL);
        else if(datalen != img->datalen)
            zprintf(0, "WARNING: read() size mismatch: 0x%lx != 0x%lx\n",
                    datalen, img->datalen);
    }
    return(img);
}

static int v4l2_start (zbar_video_t *vdo)
{
    if(vdo->iomode == VIDEO_READWRITE)
        return(0);

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(v4l2_ioctl(vdo->fd, VIDIOC_STREAMON, &type) < 0)
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_SYSTEM, __func__,
                           "starting video stream (VIDIOC_STREAMON)"));
    return(0);
}

static int v4l2_stop (zbar_video_t *vdo)
{
    if(vdo->iomode == VIDEO_READWRITE)
        return(0);

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(v4l2_ioctl(vdo->fd, VIDIOC_STREAMOFF, &type) < 0)
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_SYSTEM, __func__,
                           "stopping video stream (VIDIOC_STREAMOFF)"));
    return(0);
}

static int v4l2_cleanup (zbar_video_t *vdo)
{
    if(vdo->iomode == VIDEO_READWRITE)
        return(0);

    struct v4l2_requestbuffers rb;
    memset(&rb, 0, sizeof(rb));
    rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(vdo->iomode == VIDEO_MMAP) {
        rb.memory = V4L2_MEMORY_MMAP;
        int i;
        for(i = 0; i < vdo->num_images; i++) {
            zbar_image_t *img = vdo->images[i];
            if(img->data &&
               v4l2_munmap((void*)img->data, img->datalen))
                err_capture(vdo, SEV_WARNING, ZBAR_ERR_SYSTEM, __func__,
                            "unmapping video frame buffers");
            img->data = NULL;
            img->datalen = 0;
        }
    }
    else
        rb.memory = V4L2_MEMORY_USERPTR;

    /* requesting 0 buffers
     * should implicitly disable streaming
     */
    if(v4l2_ioctl(vdo->fd, VIDIOC_REQBUFS, &rb) < 0)
        err_capture(vdo, SEV_WARNING, ZBAR_ERR_SYSTEM, __func__,
                    "releasing video frame buffers (VIDIOC_REQBUFS)");


    /* v4l2_close v4l2_open device */
    if(vdo->fd >= 0) {
        v4l2_close(vdo->fd);
        vdo->fd = -1;
    }
    return(0);
}

static int v4l2_mmap_buffers (zbar_video_t *vdo)
{

    struct v4l2_buffer vbuf;
    memset(&vbuf, 0, sizeof(vbuf));
    vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vbuf.memory = V4L2_MEMORY_MMAP;

    int i;
    for(i = 0; i < vdo->num_images; i++) {
        vbuf.index = i;
        if(v4l2_ioctl(vdo->fd, VIDIOC_QUERYBUF, &vbuf) < 0)
            /* FIXME cleanup */
            return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_SYSTEM, __func__,
                               "querying video buffer (VIDIOC_QUERYBUF)"));

        if(vbuf.length < vdo->datalen)
            fprintf(stderr, "WARNING: insufficient v4l2 video buffer size:\n"
                    "\tvbuf[%d].length=%x datalen=%lx image=%d x %d %.4s(%08x)\n",
                    i, vbuf.length, vdo->datalen, vdo->width, vdo->height,
                    (char*)&vdo->format, vdo->format);

        zbar_image_t *img = vdo->images[i];
        img->datalen = vbuf.length;
        img->data = v4l2_mmap(NULL, vbuf.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                         vdo->fd, vbuf.m.offset);
        if(img->data == MAP_FAILED)
            /* FIXME cleanup */
            return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_SYSTEM, __func__,
                               "mapping video frame buffers"));
        zprintf(2, "    buf[%d] 0x%lx bytes @%p\n",
                i, img->datalen, img->data);
    }
    return(0);
}

static int v4l2_set_format (zbar_video_t *vdo,
                            uint32_t fmt)
{
    struct v4l2_format vfmt;
    struct v4l2_pix_format *vpix = &vfmt.fmt.pix;
    memset(&vfmt, 0, sizeof(vfmt));
    vfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vpix->width = vdo->width;
    vpix->height = vdo->height;
    vpix->pixelformat = fmt;
    vpix->field = V4L2_FIELD_NONE;
    int rc = 0;
    if((rc = v4l2_ioctl(vdo->fd, VIDIOC_S_FMT, &vfmt)) < 0) {
        /* several broken drivers return an error if we request
         * no interlacing (NB v4l2 spec violation)
         * ...try again with an interlaced request
         */
        zprintf(1, "VIDIOC_S_FMT returned %d(%d), trying interlaced...\n",
                rc, errno);

        /* FIXME this might be _ANY once we can de-interlace */
        vpix->field = V4L2_FIELD_INTERLACED;

        if(v4l2_ioctl(vdo->fd, VIDIOC_S_FMT, &vfmt) < 0)
            return(err_capture_int(vdo, SEV_ERROR, ZBAR_ERR_SYSTEM, __func__,
                                   "setting format %x (VIDIOC_S_FMT)", fmt));

        zprintf(0, "WARNING: broken driver returned error when non-interlaced"
                " format requested\n");
    }

    struct v4l2_format newfmt;
    struct v4l2_pix_format *newpix = &newfmt.fmt.pix;
    memset(&newfmt, 0, sizeof(newfmt));
    newfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(v4l2_ioctl(vdo->fd, VIDIOC_G_FMT, &newfmt) < 0)
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_SYSTEM, __func__,
                           "querying format (VIDIOC_G_FMT)"));

    if(newpix->field != V4L2_FIELD_NONE)
        err_capture(vdo, SEV_WARNING, ZBAR_ERR_INVALID, __func__,
                    "video driver only supports interlaced format,"
                    " vertical scanning may not work");

    if(newpix->pixelformat != fmt
       /* FIXME bpl/bpp checks? */)
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                           "video driver can't provide compatible format"));

    vdo->format = fmt;
    vdo->width = newpix->width;
    vdo->height = newpix->height;
    vdo->datalen = newpix->sizeimage;

    zprintf(1, "set new format: %.4s(%08x) %u x %u (0x%lx)\n",
            (char*)&vdo->format, vdo->format, vdo->width, vdo->height,
            vdo->datalen);
    return(0);
}

static int v4l2_init (zbar_video_t *vdo,
                      uint32_t fmt)
{
    struct v4l2_requestbuffers rb;
    if(v4l2_set_format(vdo, fmt))
        return(-1);

    memset(&rb, 0, sizeof(rb));
    rb.count = vdo->num_images;
    rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(vdo->iomode == VIDEO_MMAP)
        rb.memory = V4L2_MEMORY_MMAP;
    else
        rb.memory = V4L2_MEMORY_USERPTR;

    if(v4l2_ioctl(vdo->fd, VIDIOC_REQBUFS, &rb) < 0)
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_SYSTEM, __func__,
                           "requesting video frame buffers (VIDIOC_REQBUFS)"));

    if(!rb.count)
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                           "driver returned 0 buffers"));

    if(vdo->num_images > rb.count)
        vdo->num_images = rb.count;

    zprintf(1, "using %u buffers (of %d requested)\n",
            rb.count, vdo->num_images);

    if(vdo->iomode == VIDEO_MMAP)
        return(v4l2_mmap_buffers(vdo));
    return(0);
}

static int v4l2_probe_iomode (zbar_video_t *vdo)
{
    struct v4l2_requestbuffers rb;
    memset(&rb, 0, sizeof(rb));
    rb.count = vdo->num_images;
    rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(vdo->iomode == VIDEO_MMAP)
        rb.memory = V4L2_MEMORY_MMAP;
    else
        rb.memory = V4L2_MEMORY_USERPTR;

    if(v4l2_ioctl(vdo->fd, VIDIOC_REQBUFS, &rb) < 0) {
        if(vdo->iomode)
            return(err_capture_int(vdo, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                                   "unsupported iomode requested (%d)",
                                   vdo->iomode));
        else if(errno != EINVAL)
            return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_SYSTEM, __func__,
                               "querying streaming mode (VIDIOC_REQBUFS)"));
#ifdef HAVE_SYS_MMAN_H
	err_capture(vdo, SEV_WARNING, ZBAR_ERR_SYSTEM, __func__,
                               "USERPTR failed. Falling back to mmap");
        vdo->iomode = VIDEO_MMAP;
#else
	return err_capture(vdo, SEV_ERROR, ZBAR_ERR_SYSTEM, __func__,
                           "Userptr not supported, and zbar was compiled without mmap support"));
#endif
    }
    else {
        if(!vdo->iomode)
            rb.memory = V4L2_MEMORY_USERPTR;
	/* Update the num_images with the max supported by the driver */
        if(rb.count)
            vdo->num_images = rb.count;
	else
            err_capture(vdo, SEV_WARNING, ZBAR_ERR_SYSTEM, __func__,
                        "Something is wrong: number of buffers returned by REQBUF is zero!");

        /* requesting 0 buffers
         * This cleans up the buffers allocated previously on probe
         */
	rb.count = 0;
        if(v4l2_ioctl(vdo->fd, VIDIOC_REQBUFS, &rb) < 0)
            err_capture(vdo, SEV_WARNING, ZBAR_ERR_SYSTEM, __func__,
                        "releasing video frame buffers (VIDIOC_REQBUFS)");
    }
    return(0);
}

static inline void v4l2_max_size (zbar_video_t *vdo, uint32_t pixfmt,
                                  uint32_t *max_width,  uint32_t *max_height)
{
    int mwidth = 0, mheight = 0, i;
    struct v4l2_frmsizeenum frm;

    for(i = 0; i < V4L2_FORMATS_SIZE_MAX; i++) {
        memset(&frm, 0, sizeof(frm));
        frm.index = i;
        frm.pixel_format = pixfmt;

        if(v4l2_ioctl(vdo->fd, VIDIOC_ENUM_FRAMESIZES, &frm))
            break;

        switch (frm.type) {
        case V4L2_FRMSIZE_TYPE_DISCRETE:
            mwidth = frm.discrete.width;
            mheight = frm.discrete.height;
            break;
        case V4L2_FRMSIZE_TYPE_CONTINUOUS:
        case V4L2_FRMSIZE_TYPE_STEPWISE:
            mwidth = frm.stepwise.max_width;
            mheight = frm.stepwise.max_height;
            break;
        default:
            continue;
        }
        if (mwidth > *max_width)
            *max_width = mwidth;
        if (mheight > *max_height)
            *max_height = mheight;
    }
}

static inline int v4l2_probe_formats (zbar_video_t *vdo)
{
    int n_formats = 0, n_emu_formats = 0;
    uint32_t max_width = 0, max_height = 0;

    if(vdo->width && vdo->height)
            zprintf(1, "Caller requested an specific size: %d x %d\n",
                    vdo->width, vdo->height);

    zprintf(2, "enumerating supported formats:\n");
    struct v4l2_fmtdesc desc;
    memset(&desc, 0, sizeof(desc));
    desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    for(desc.index = 0; desc.index < V4L2_FORMATS_MAX; desc.index++) {
        if(v4l2_ioctl(vdo->fd, VIDIOC_ENUM_FMT, &desc) < 0)
            break;
        zprintf(2, "    [%d] %.4s : %s%s%s\n",
                desc.index, (char*)&desc.pixelformat, desc.description,
                (desc.flags & V4L2_FMT_FLAG_COMPRESSED) ? " COMPRESSED" : "",
                (desc.flags & V4L2_FMT_FLAG_EMULATED) ? " EMULATED" : "");
        if (desc.flags & V4L2_FMT_FLAG_EMULATED) {
            vdo->emu_formats = realloc(vdo->emu_formats,
                                   (n_emu_formats + 2) * sizeof(uint32_t));
            vdo->emu_formats[n_emu_formats++] = desc.pixelformat;
        } else {
            vdo->formats = realloc(vdo->formats,
                                   (n_formats + 2) * sizeof(uint32_t));
            vdo->formats[n_formats++] = desc.pixelformat;
        }

        if(!vdo->width || !vdo->height)
            v4l2_max_size(vdo, desc.pixelformat, &max_width, &max_height);
    }

    if(!vdo->width || !vdo->height) {
        zprintf(1, "Max supported size: %d x %d\n", max_width, max_height);
        if (max_width && max_height) {
            vdo->width = max_width;
            vdo->height = max_height;
        } else {
            /* fallback to large size, driver reduces to max available */
            vdo->width = 640 * 64;
            vdo->height = 480 * 64;

        }
    }

    if(!desc.index)
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_SYSTEM, __func__,
                           "enumerating video formats (VIDIOC_ENUM_FMT)"));
    if(vdo->formats)
       vdo->formats[n_formats] = 0;
    if(vdo->emu_formats)
       vdo->emu_formats[n_emu_formats] = 0;
    if(!vdo->formats && vdo->emu_formats) {
       /*
        * If only emu formats are available, just move them to vdo->formats.
        * This happens when libv4l detects that the only available fourcc
        * formats are webcam proprietary formats or bayer formats.
        */
       vdo->formats = vdo->emu_formats;
       vdo->emu_formats = NULL;
    }

    zprintf(2, "Found %d formats and %d emulated formats.\n",
            n_formats, n_emu_formats);

    struct v4l2_format fmt;
    struct v4l2_pix_format *pix = &fmt.fmt.pix;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(v4l2_ioctl(vdo->fd, VIDIOC_G_FMT, &fmt) < 0)
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_SYSTEM, __func__,
                           "querying current video format (VIDIO_G_FMT)"));

    zprintf(1, "current format: %.4s(%08x) %u x %u%s (line=0x%x size=0x%x)\n",
            (char*)&pix->pixelformat, pix->pixelformat,
            pix->width, pix->height,
            (pix->field != V4L2_FIELD_NONE) ? " INTERLACED" : "",
            pix->bytesperline, pix->sizeimage);

    vdo->format = pix->pixelformat;
    vdo->datalen = pix->sizeimage;
    if(pix->width == vdo->width && pix->height == vdo->height)
        return(0);

    struct v4l2_format maxfmt;
    struct v4l2_pix_format *maxpix = &maxfmt.fmt.pix;
    memcpy(&maxfmt, &fmt, sizeof(maxfmt));
    maxpix->width = vdo->width;
    maxpix->height = vdo->height;

    zprintf(1, "setting requested size: %d x %d\n", vdo->width, vdo->height);
    if(v4l2_ioctl(vdo->fd, VIDIOC_S_FMT, &maxfmt) < 0) {
        zprintf(1, "set FAILED...trying to recover original format\n");
        /* ignore errors (driver broken anyway) */
        v4l2_ioctl(vdo->fd, VIDIOC_S_FMT, &fmt);
    }

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(v4l2_ioctl(vdo->fd, VIDIOC_G_FMT, &fmt) < 0)
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_SYSTEM, __func__,
                           "querying current video format (VIDIOC_G_FMT)"));

    zprintf(1, "final format: %.4s(%08x) %u x %u%s (line=0x%x size=0x%x)\n",
            (char*)&pix->pixelformat, pix->pixelformat,
            pix->width, pix->height,
            (pix->field != V4L2_FIELD_NONE) ? " INTERLACED" : "",
            pix->bytesperline, pix->sizeimage);

    vdo->width = pix->width;
    vdo->height = pix->height;
    vdo->datalen = pix->sizeimage;
    return(0);
}

static inline int v4l2_reset_crop (zbar_video_t *vdo)
{
    /* check cropping */
    struct v4l2_cropcap ccap;
    memset(&ccap, 0, sizeof(ccap));
    ccap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(v4l2_ioctl(vdo->fd, VIDIOC_CROPCAP, &ccap) < 0)
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_SYSTEM, __func__,
                           "querying crop support (VIDIOC_CROPCAP)"));

    zprintf(1, "crop bounds: %d x %d @ (%d, %d)\n",
            ccap.bounds.width, ccap.bounds.height,
            ccap.bounds.left, ccap.bounds.top);
    zprintf(1, "current crop win: %d x %d @ (%d, %d) aspect %d / %d\n",
            ccap.defrect.width, ccap.defrect.height,
            ccap.defrect.left, ccap.defrect.top,
            ccap.pixelaspect.numerator, ccap.pixelaspect.denominator);

#if 0
    // This logic causes the device to fallback to the current resolution
    if(!vdo->width || !vdo->height) {
        vdo->width = ccap.defrect.width;
        vdo->height = ccap.defrect.height;
    }
#endif

    /* reset crop parameters */
    struct v4l2_crop crop;
    memset(&crop, 0, sizeof(crop));
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c = ccap.defrect;
    if(v4l2_ioctl(vdo->fd, VIDIOC_S_CROP, &crop) < 0 && errno != EINVAL)
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_SYSTEM, __func__,
                           "setting default crop window (VIDIOC_S_CROP)"));
    return(0);
}

static void v4l2_add_control(zbar_video_t *vdo,
                            char *group_name,
                            struct v4l2_queryctrl *query,
                            struct video_controls_priv_s **ptr)
{
    // Control is disabled, ignore it. Please notice that disabled controls
    // can be re-enabled. The right thing here would be to get those too,
    // and add a logic to
    if (query->flags & V4L2_CTRL_FLAG_DISABLED)
        return;

    // FIXME: add support also for other control types
    if (!((query->type == V4L2_CTRL_TYPE_INTEGER) ||
          (query->type == V4L2_CTRL_TYPE_BOOLEAN)))
        return;

    zprintf(1, "%s %s ctrl %-32s id: 0x%x\n",
            group_name,
            (query->type == V4L2_CTRL_TYPE_INTEGER) ? "int " :
                                                      "bool",
            query->name,
            query->id);

    // Allocate a new element on the linked list
    if (!vdo->controls) {
        *ptr = calloc(1, sizeof(**ptr));
        vdo->controls = (void *)*ptr;
    } else {
        (*ptr)->s.next = calloc(1, sizeof(**ptr));
        *ptr = (*ptr)->s.next;
    }

    // Fill control data
    (*ptr)->id = query->id;
    (*ptr)->s.name = strdup((const char *)query->name);
    if (query->type == V4L2_CTRL_TYPE_INTEGER) {
        (*ptr)->s.type = VIDEO_CNTL_INTEGER;
        (*ptr)->s.min = query->minimum;
        (*ptr)->s.max = query->maximum;
        (*ptr)->s.def = query->default_value;
        (*ptr)->s.step = query->step;
    } else {
        (*ptr)->s.type = VIDEO_CNTL_BOOLEAN;
    }
}

static int v4l2_query_controls(zbar_video_t *vdo)
{
    int id = 0;
    struct video_controls_priv_s *ptr;
    struct v4l2_queryctrl query;

    // Free controls list if not NULL
    ptr = (void *)vdo->controls;
    while (ptr) {
        free(ptr->s.name);
        ptr = ptr->s.next;
    }
    free(vdo->controls);
    vdo->controls = NULL;
    ptr = NULL;

    id=0;
    do {
        query.id = id | V4L2_CTRL_FLAG_NEXT_CTRL;
        if(v4l2_ioctl(vdo->fd, VIDIOC_QUERYCTRL, &query))
            break;

        v4l2_add_control(vdo, "extended", &query, &ptr);
        id = query.id;
    } while (1);

    id=V4L2_CID_PRIVATE_BASE;
    do {
        query.id = id;
        if(v4l2_ioctl(vdo->fd, VIDIOC_QUERYCTRL, &query))
            break;
        v4l2_add_control(vdo, "private", &query, &ptr);
        id = query.id;
    } while (1);

    return(0);
}

/** locate a control entry
 */
static struct video_controls_priv_s *v4l2_g_control_def(zbar_video_t *vdo,
                                                   const char *name)
{
    struct video_controls_priv_s *p = (void *)vdo->controls;

    while (p) {
        if (!strcmp(p->s.name, name))
            break;
        p = p->s.next;
    }

    if (!p->s.name)
        return NULL;

    return p;
}

static int v4l2_s_control(zbar_video_t *vdo,
                          const char *name,
                          void *value)
{
    struct v4l2_control cs;
    struct video_controls_priv_s *p;

    p = v4l2_g_control_def(vdo, name);
    if (!p)
        return ZBAR_ERR_UNSUPPORTED; // we have no such a control on the list

    zprintf(1, "%-32s id: 0x%x set to value %d\n",
            name, p->id, *(int*)value);

    // FIXME: add support for VIDIOC_S_EXT_CTRL
    memset(&cs, 0, sizeof(cs));
    cs.id = p->id;
    cs.value = *(int*)value;
    int rv = v4l2_ioctl(vdo->fd, VIDIOC_S_CTRL, &cs);
    if(rv!=0) {
        zprintf(1, "v4l2 set user control \"%s\" returned %d\n", p->s.name, rv);
        rv = ZBAR_ERR_INVALID;
    }
    return rv;
}

static int v4l2_g_control(zbar_video_t *vdo,
                            const char *name,
                            void *value)
{
    struct v4l2_control cs;
    struct video_controls_priv_s *p;

    p = v4l2_g_control_def(vdo, name);
    if (!p)
        return ZBAR_ERR_UNSUPPORTED; // we have no such a control on the list

    memset(&cs, 0, sizeof(cs));

    cs.id = p->id;
    cs.value = *(int*)value;
    int rv = v4l2_ioctl(vdo->fd, VIDIOC_G_CTRL, &cs);
    *(int*)value = cs.value;
    if(rv!=0) {
        zprintf(1, "v4l2 get user control \"%s\" returned %d\n", p->s.name, rv);
        rv = ZBAR_ERR_UNSUPPORTED;
    }
    return rv;

    // FIXME: add support for VIDIOC_G_EXT_CTRL
    /* untested stuff */
#if 0
    memset(&ext_ctrls, 0, sizeof(ext_ctrls));
    ext_ctrls.ctrl_class = V4L2_CTRL_CLASS_MPEG; //V4L2_CTRL_ID2CLASS(p->id);
    ext_ctrls.count = 1;
    ext_ctrls.controls = &ext_ctrl;
    memset(&ext_ctrl, 0, sizeof(ext_ctrl));
    ext_ctrl.id = p->id;
    rv = v4l2_ioctl(vdo->fd, VIDIOC_G_EXT_CTRLS, &ext_ctrls);
    *(int*)value = ext_ctrl.value;
    if(rv!=0) {
        zprintf(1, "v4l2 get ext control \"%s\" returned %d\n", p->name, rv);
        rv = ZBAR_ERR_UNSUPPORTED;
    }
    return rv;
#endif
}

int _zbar_v4l2_probe (zbar_video_t *vdo)
{
    /* check capabilities */
    struct v4l2_capability vcap;
    memset(&vcap, 0, sizeof(vcap));
    if(v4l2_ioctl(vdo->fd, VIDIOC_QUERYCAP, &vcap) < 0)
        return(err_capture(vdo, SEV_WARNING, ZBAR_ERR_UNSUPPORTED, __func__,
                           "video4linux version 2 not supported (VIDIOC_QUERYCAP)"));

    
    zprintf(1, "%.32s on %.32s driver %.16s (version %u.%u.%u)\n", vcap.card,
            (vcap.bus_info[0]) ? (char*)vcap.bus_info : "<unknown>",
            vcap.driver, (vcap.version >> 16) & 0xff,
            (vcap.version >> 8) & 0xff, vcap.version & 0xff);
    zprintf(1, "    capabilities:%s%s%s%s\n",
            (vcap.capabilities & V4L2_CAP_VIDEO_CAPTURE) ? " CAPTURE" : "",
            (vcap.capabilities & V4L2_CAP_VIDEO_OVERLAY) ? " OVERLAY" : "",
            (vcap.capabilities & V4L2_CAP_READWRITE) ? " READWRITE" : "",
            (vcap.capabilities & V4L2_CAP_STREAMING) ? " STREAMING" : "");

    if(!(vcap.capabilities & V4L2_CAP_VIDEO_CAPTURE) ||
       !(vcap.capabilities & (V4L2_CAP_READWRITE | V4L2_CAP_STREAMING)))
        return(err_capture(vdo, SEV_WARNING, ZBAR_ERR_UNSUPPORTED, __func__,
                           "v4l2 device does not support usable CAPTURE"));

    if(v4l2_reset_crop(vdo))
        /* ignoring errors (driver cropping support questionable) */;

    if(v4l2_probe_formats(vdo))
        return(-1);

    if (v4l2_query_controls(vdo))
        return(-1);

    /* FIXME report error and fallback to readwrite? (if supported...) */
    if(vdo->iomode != VIDEO_READWRITE &&
       (vcap.capabilities & V4L2_CAP_STREAMING) &&
       v4l2_probe_iomode(vdo))
        return(-1);
    if(!vdo->iomode)
        vdo->iomode = VIDEO_READWRITE;

    zprintf(1, "using I/O mode: %s\n",
            (vdo->iomode == VIDEO_READWRITE) ? "READWRITE" :
            (vdo->iomode == VIDEO_MMAP) ? "MMAP" :
            (vdo->iomode == VIDEO_USERPTR) ? "USERPTR" : "<UNKNOWN>");

    vdo->intf = VIDEO_V4L2;
    vdo->init = v4l2_init;
    vdo->cleanup = v4l2_cleanup;
    vdo->start = v4l2_start;
    vdo->stop = v4l2_stop;
    vdo->nq = v4l2_nq;
    vdo->dq = v4l2_dq;
    vdo->set_control = v4l2_s_control;
    vdo->get_control = v4l2_g_control;
    return(0);
}
