#pragma once

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>

#include <libv4l2.h>
#include <linux/videodev2.h>

typedef struct {
    void *start;
    size_t length;
} CommonV4l2_Buffer;

typedef struct {
    int fd;
    CommonV4l2_Buffer *buffers;
    struct v4l2_buffer buf;
    unsigned int n_buffers;
} CommonV4l2;

void CommonV4l2_xioctl(int fh, unsigned long int request, void *arg);

void CommonV4l2_init(CommonV4l2 *this_v4l2, const char *dev_name, unsigned int x_res,
                     unsigned int y_res, unsigned int fps);

void CommonV4l2_update_image(CommonV4l2 *this_v4l2);

char * CommonV4l2_get_image(CommonV4l2 *this_v4l2);

size_t CommonV4l2_get_image_size(CommonV4l2 *this_v4l2);

void CommonV4l2_deinit(CommonV4l2 *this_v4l2);
