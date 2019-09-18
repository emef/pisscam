#include "MotionDetector.h"

#include <spdlog/spdlog.h>

MotionDetector::MotionDetector(const int width, const int height, const double sensitivity,
               const double forget_factor)
  : _width(width)
  , _height(height)
  , _sensitivity(sensitivity)
  , _forget_factor(forget_factor)
  , _count(0) {

  _sums.resize(width * height, 0);
  _sq_sums.resize(width * height, 0);
}

bool MotionDetector::observeFrame(const char* image_data) {
  const size_t elems = _width * _height;
  double var = 0;

  _count = _count * _forget_factor + 1;

  for (size_t i = 0; i < elems; i++) {
    _sums[i] = _sums[i] * _forget_factor + image_data[i];
    _sq_sums[i] = _sq_sums[i] * _forget_factor + image_data[i] * image_data[i];

    var += (_sq_sums[i] - (_sums[i] * _sums[i]) / _count) / _count;
  }

  var /= elems;

  return var > _sensitivity;
}
