#include "Webcam.h"

#include <spdlog/spdlog.h>

Webcam::Webcam()
  : _width(320)
  , _height(240)
  , _fps(5) {

  const char *dev_name = "/dev/video0";

  CommonV4l2_init(&_v4l2, dev_name, _width, _height, _fps);
}

Webcam::~Webcam() {
  CommonV4l2_deinit(&_v4l2);
}

WebcamFrame Webcam::nextFrame() {
  CommonV4l2_update_image(&_v4l2);
  char *image_data = CommonV4l2_get_image(&_v4l2);

  WebcamFrame frame;
  frame.width = _width;
  frame.height = _height;
  frame.image_data = image_data;
  frame.timestamp = Clock::now();

  return frame;
}
