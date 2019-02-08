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
#include <argp.h>
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>

#include <zbar.h>
#include "test_images.h"

#ifdef _WIN32
struct timespec {
    time_t tv_sec;
    long tv_nsec;
};
#endif

zbar_video_t *video;

#define PROGRAM_NAME	"test_video"

static const char doc[] = "\nTest if ZBar is able to handle a video input (camera)\n";

static const struct argp_option options[] = {
    {"quiet", 'q', 0,         0, "Don't be verbose",           0},
    {"dev",   'd', "devnode", 0, "open devnode for video in",  0},
    {"format",'f', "fourcc",  0, "Stop after #seconds",        0},
    {"help",  '?', 0,         0, "Give this help list",       -1},
    {"usage",  -3, 0,         0, "Give a short usage message", 0},
    { 0 }
};

static int quiet = 0;
uint32_t vidfmt = fourcc('B','G','R','3');
char *dev = "";

static error_t parse_opt(int k, char *optarg, struct argp_state *state)
{
    switch (k) {
    case 'q':
        quiet = 1;
        break;
    case 'f': {
        int len = strlen(optarg);
        if (len > 4)
            len = 4;
        memcpy((char*)&vidfmt, optarg, len);
        if (len < 4)
            memset(len + (char*)&vidfmt, 0, 4 - len);
        break;
    }
    case 'd':
        dev = optarg;
        break;
    case '?':
        argp_state_help(state, state->out_stream,
                        ARGP_HELP_SHORT_USAGE | ARGP_HELP_LONG |
                        ARGP_HELP_DOC);
        exit(0);
    case -3:
        argp_state_help(state, state->out_stream, ARGP_HELP_USAGE);
        exit(0);
    default:
        return ARGP_ERR_UNKNOWN;
    };
    return 0;
}

static const struct argp argp = {
	.options = options,
	.parser = parse_opt,
	.doc = doc,
};

int main (int argc, char *argv[])
{

    if (argp_parse(&argp, argc, argv, ARGP_NO_HELP | ARGP_NO_EXIT, 0, 0)) {
        argp_help(&argp, stderr, ARGP_HELP_SHORT_USAGE, PROGRAM_NAME);
        return -1;
    }
    if (!quiet)
        zbar_set_verbosity(31);
    else
        zbar_set_verbosity(0);

    video = zbar_video_create();
    if(!video) {
        fprintf(stderr, "unable to allocate memory?!\n");
        return(1);
    }

    zbar_video_request_size(video, 640, 480);

    if(zbar_video_open(video, dev)) {
        zbar_video_error_spew(video, 0);
        fprintf(stderr,
                "ERROR: unable to access your video device\n"
                "this program requires video capture support using"
                " v4l version 1 or 2 or VfW\n"
                "    - is your video device located at \"%s\"?\n"
                "    - is your video driver installed? (check dmesg)\n"
                "    - make sure you have the latest drivers\n"
                "    - do you have read/write permission to access it?\n"
                "    - is another application is using it?\n"
                "    - does the device support video capture?\n"
                "    - does the device work with other programs?\n",
                dev);
        return(1);
    }
    if (!quiet) {
        fprintf(stderr, "opened video device: %s (fd=%d)\n",
                dev, zbar_video_get_fd(video));
        fflush(stderr);
    }

    if(zbar_video_init(video, vidfmt)) {
        fprintf(stderr, "ERROR: failed to set format: %.4s(%08x)\n",
                (char*)&vidfmt, vidfmt);
        return(zbar_video_error_spew(video, 0));
    }

    if(zbar_video_enable(video, 1)) {
        fprintf(stderr, "ERROR: starting video stream\n");
        return(zbar_video_error_spew(video, 0));
    }
    if (!quiet) {
        fprintf(stderr, "started video stream...\n");
        fflush(stderr);
    }

    zbar_image_t *image = zbar_video_next_image(video);
    if(!image) {
        fprintf(stderr, "ERROR: unable to capture image\n");
        return(zbar_video_error_spew(video, 0));
    }
    uint32_t format = zbar_image_get_format(image);
    unsigned width = zbar_image_get_width(image);
    unsigned height = zbar_image_get_height(image);
    const uint8_t *data = zbar_image_get_data(image);
    if (!quiet) {
        fprintf(stderr, "captured image: %d x %d %.4s @%p\n",
                width, height, (char*)&format, data);
        fflush(stderr);
    }

    zbar_image_destroy(image);

    if (!quiet) {
        fprintf(stderr, "\nstreaming 100 frames...\n");
        fflush(stderr);
    }

    struct timespec start, end;
#if _POSIX_TIMERS > 0
    clock_gettime(CLOCK_REALTIME, &start);
#else
    struct timeval ustime;
    gettimeofday(&ustime, NULL);
    start.tv_nsec = ustime.tv_usec * 1000;
    start.tv_sec = ustime.tv_sec;
#endif

    int i;
    for(i = 0; i < 100; i++) {
        zbar_image_t *image = zbar_video_next_image(video);
        if(!image) {
            fprintf(stderr, "ERROR: unable to capture image\n");
            return(zbar_video_error_spew(video, 0));
        }
        zbar_image_destroy(image);
    }

#if _POSIX_TIMERS > 0
    clock_gettime(CLOCK_REALTIME, &end);
#else
    gettimeofday(&ustime, NULL);
    end.tv_nsec = ustime.tv_usec * 1000;
    end.tv_sec = ustime.tv_sec;
#endif
    double ms = (end.tv_sec - start.tv_sec +
                 (end.tv_nsec - start.tv_nsec) / 1000000000.);
    double fps = i / ms;
    if (!quiet) {
        fprintf(stderr, "\nprocessed %d images in %gs @%gfps\n", i, ms, fps);
        fflush(stderr);
    }

    if(zbar_video_enable(video, 0)) {
        fprintf(stderr, "ERROR: while stopping video stream\n");
        return(zbar_video_error_spew(video, 0));
    }

    if (!quiet) {
        fprintf(stderr, "\ncleaning up...\n");
        fflush(stderr);
    }

    zbar_video_destroy(video);

    if(test_image_check_cleanup())
        return(32);

    fprintf(stderr, "video PASSED.\n");
    return(0);
}
