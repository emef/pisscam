#include "common_v4l2.h"

#define COMMON_V4L2_CLEAR(x) memset(&(x), 0, sizeof(x))

void CommonV4l2_xioctl(int fh, unsigned long int request, void *arg)
{
    int r;
    do {
        r = v4l2_ioctl(fh, request, arg);
    } while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));
    if (r == -1) {
        fprintf(stderr, "error %d, %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void CommonV4l2_init(CommonV4l2 *this_v4l2, const char *dev_name, unsigned int x_res,
                     unsigned int y_res, unsigned int fps) {
    enum v4l2_buf_type type;
    struct v4l2_format fmt;
    struct v4l2_requestbuffers req;
    unsigned int i;

    this_v4l2->fd = v4l2_open(dev_name, O_RDWR | O_NONBLOCK, 0);
    if (this_v4l2->fd < 0) {
        perror("Cannot open device");
        exit(EXIT_FAILURE);
    }
    COMMON_V4L2_CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = x_res;
    fmt.fmt.pix.height      = y_res;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
    CommonV4l2_xioctl(this_v4l2->fd, VIDIOC_S_FMT, &fmt);
    if ((fmt.fmt.pix.width != x_res) || (fmt.fmt.pix.height != y_res))
        printf("Warning: driver is sending image at %dx%d\n",
               fmt.fmt.pix.width, fmt.fmt.pix.height);

    struct v4l2_streamparm streamparm;
    memset(&streamparm, 0, sizeof(streamparm));
    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    CommonV4l2_xioctl(this_v4l2->fd, VIDIOC_G_PARM, &streamparm);

    streamparm.parm.capture.capturemode |= V4L2_CAP_TIMEPERFRAME;
    streamparm.parm.capture.timeperframe.numerator = 1;
    streamparm.parm.capture.timeperframe.denominator = fps;
    CommonV4l2_xioctl(this_v4l2->fd, VIDIOC_S_PARM, &streamparm);

    COMMON_V4L2_CLEAR(req);
    req.count = 2;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    CommonV4l2_xioctl(this_v4l2->fd, VIDIOC_REQBUFS, &req);
    this_v4l2->buffers = (CommonV4l2_Buffer*) calloc(req.count, sizeof(*this_v4l2->buffers));
    for (this_v4l2->n_buffers = 0; this_v4l2->n_buffers < req.count; ++this_v4l2->n_buffers) {
        COMMON_V4L2_CLEAR(this_v4l2->buf);
        this_v4l2->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        this_v4l2->buf.memory = V4L2_MEMORY_MMAP;
        this_v4l2->buf.index = this_v4l2->n_buffers;
        CommonV4l2_xioctl(this_v4l2->fd, VIDIOC_QUERYBUF, &this_v4l2->buf);
        this_v4l2->buffers[this_v4l2->n_buffers].length = this_v4l2->buf.length;
        this_v4l2->buffers[this_v4l2->n_buffers].start = v4l2_mmap(NULL, this_v4l2->buf.length,
            PROT_READ | PROT_WRITE, MAP_SHARED, this_v4l2->fd, this_v4l2->buf.m.offset);
        if (MAP_FAILED == this_v4l2->buffers[this_v4l2->n_buffers].start) {
            perror("mmap");
            exit(EXIT_FAILURE);
        }
    }
    for (i = 0; i < this_v4l2->n_buffers; ++i) {
        COMMON_V4L2_CLEAR(this_v4l2->buf);
        this_v4l2->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        this_v4l2->buf.memory = V4L2_MEMORY_MMAP;
        this_v4l2->buf.index = i;
        CommonV4l2_xioctl(this_v4l2->fd, VIDIOC_QBUF, &this_v4l2->buf);
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    CommonV4l2_xioctl(this_v4l2->fd, VIDIOC_STREAMON, &type);
}

void CommonV4l2_update_image(CommonV4l2 *this_v4l2) {
    fd_set fds;
    int r;
    struct timeval tv;

    do {
        FD_ZERO(&fds);
        FD_SET(this_v4l2->fd, &fds);

        /* Timeout. */
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        r = select(this_v4l2->fd + 1, &fds, NULL, NULL, &tv);
    } while ((r == -1 && (errno == EINTR)));
    if (r == -1) {
        perror("select");
        exit(EXIT_FAILURE);
    }
    COMMON_V4L2_CLEAR(this_v4l2->buf);
    this_v4l2->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    this_v4l2->buf.memory = V4L2_MEMORY_MMAP;
    CommonV4l2_xioctl(this_v4l2->fd, VIDIOC_DQBUF, &this_v4l2->buf);
    CommonV4l2_xioctl(this_v4l2->fd, VIDIOC_QBUF, &this_v4l2->buf);
}

char * CommonV4l2_get_image(CommonV4l2 *this_v4l2) {
    return ((char *)this_v4l2->buffers[this_v4l2->buf.index].start);
}

size_t CommonV4l2_get_image_size(CommonV4l2 *this_v4l2) {
    return this_v4l2->buffers[this_v4l2->buf.index].length;
}

void CommonV4l2_deinit(CommonV4l2 *this_v4l2) {
    unsigned int i;
    enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    CommonV4l2_xioctl(this_v4l2->fd, VIDIOC_STREAMOFF, &type);
    for (i = 0; i < this_v4l2->n_buffers; ++i)
        v4l2_munmap(this_v4l2->buffers[i].start, this_v4l2->buffers[i].length);
    v4l2_close(this_v4l2->fd);
    free(this_v4l2->buffers);
}
