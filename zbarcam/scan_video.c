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

#include <fcntl.h>
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/videodev2.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

typedef void (cb_t) (void *userdata, const char *device);

struct devnodes {
    char *fname;
    int minor;
    int is_valid;
};

static unsigned int n_devices = 0;
static struct devnodes *devices = NULL;

/*
 * Sort order:
 *
 *  - Valid devices comes first
 *  - Lowest minors comes first
 *
 * For devnode names, it sorts on this order:
 *  - custom udev given names
 *  - /dev/v4l/by-id/
 *  - /dev/v4l/by-path/
 *  - /dev/video
 *  - /dev/char/
 *
 *  - Device name is sorted alphabetically if follows same pattern
 */
static int sort_devices(const void *__a, const void *__b)
{
    const struct devnodes *a = __a;
    const struct devnodes *b = __b;
    int val_a, val_b;

    if (a->is_valid != b->is_valid)
        return !a->is_valid - !b->is_valid;

    if (a->minor != b->minor)
        return a->minor - b->minor;

    /* Ensure that /dev/video* devices will stay at the top */

    if (strstr(a->fname, "by-id"))
        val_a = 1;
    if (strstr(a->fname, "by-path"))
        val_a = 2;
    else if (strstr(a->fname, "/dev/video"))
        val_a = 3;
    else if (strstr(a->fname, "char"))
        val_a = 4;
    else    /* Customized names comes first */
        val_a = 0;

    if (strstr(b->fname, "by-id"))
        val_b = 1;
    if (strstr(b->fname, "by-path"))
        val_b = 2;
    else if (strstr(b->fname, "/dev/video"))
        val_b = 3;
    else if (strstr(b->fname, "char"))
        val_b = 4;
    else   /* Customized names comes first */
        val_b = 0;

    if (val_a != val_b)
        return val_a - val_b;

    /* Finally, just use alphabetic order */
    return strcmp(a->fname, b->fname);
}

static int handle_video_devs(const char *file,
                             const struct stat *st,
                             int flag)
{
    int dev_minor, first_device = -1, fd;
    unsigned int i;
    struct v4l2_capability vid_cap = { 0 };

    /* Discard  devices that can't be a videodev */
    if (!S_ISCHR(st->st_mode) || major(st->st_rdev) != 81)
        return 0;

    dev_minor = minor(st->st_rdev);

    /* check if it is an already existing device */
    if (devices) {
        for (i = 0; i < n_devices; i++) {
            if (dev_minor == devices[i].minor) {
                first_device = i;
                break;
            }
        }
    }

    devices = realloc(devices, (n_devices + 1) * sizeof(struct devnodes));
    if (!devices) {
        perror("Can't allocate memory to store devices");
        exit(1);
    }
    memset(&devices[n_devices], 0, sizeof(struct devnodes));

    if (first_device < 0) {
        fd = open(file, O_RDWR);
        if (fd < 0) {
            devices[n_devices].is_valid = 0;
        } else {
            if (ioctl(fd, VIDIOC_QUERYCAP, &vid_cap) == -1) {
                devices[n_devices].is_valid = 0;
            } else {
#ifdef V4L2_CID_ALPHA_COMPONENT
                /*
                 * device_caps was added on Kernel 3.3. The preferred
                 * way to handle such compat stuff would be to include
                 * a recent videodev2.h at ZBar's source and check the
                 * V4L2 API returned by VIDIOC_QUERYCAP.
                 * However, doing that require some care, as other
                 * compat code should be checked to see if they would work.
                 * Also, it is painful to keep updating the Kernel headers.
                 * Thankfully, V4L2_CID_ALPHA_COMPONENT was also added on
                 * Kernel 3.3, so just checking if this is defined should
                 * be enough to do the right thing.
                 */
                if (!(vid_cap.device_caps & V4L2_CAP_VIDEO_CAPTURE))
                    devices[n_devices].is_valid = 0;
                else
                    devices[n_devices].is_valid = 1;
#else
                if (!(vid_cap.device_caps & V4L2_CAP_VIDEO_CAPTURE))
                    devices[n_devices].is_valid = 0;
                else
                    devices[n_devices].is_valid = 1;
#endif
            }
        }

        close(fd);
    } else {
        devices[n_devices].is_valid = devices[first_device].is_valid;
    }

    devices[n_devices].fname = strdup(file);
    devices[n_devices].minor = dev_minor;

    n_devices++;

    return(0);
}

/* scan /dev for v4l video devices and call add_device for each.
 * also looks for a specified "default" device (if not NULL)
 * if not found, the default will be appended to the list.
 * returns the index+1 of the default_device, or 0 if the default
 * was not specified.  NB *not* reentrant
 */
int scan_video (cb_t add_dev,
                void *userdata,
                const char *default_dev)
{
    unsigned int i, idx = 0;
    int default_idx = -1, last_minor = -1;

    if(ftw("/dev", handle_video_devs, 4)) {
        perror("search for video devices failed");
        return -1;
    }
    qsort(devices, n_devices, sizeof(struct devnodes), sort_devices);

    for (i = 0; i < n_devices; i++) {
        if (!devices[i].is_valid)
            continue;

        if (devices[i].minor == last_minor)
            continue;

        add_dev(userdata, devices[i].fname);
        last_minor = devices[i].minor;
        idx++;

        if (default_dev && !strcmp(default_dev, devices[i].fname))
            default_idx = idx;
        else if (!default_dev && default_idx < 0)
            default_idx = idx;
    }

    for (i = 0; i < n_devices; i++)
        free(devices[i].fname);
    free(devices);

    n_devices = 0;
    devices = NULL;

    return(default_idx);
}
