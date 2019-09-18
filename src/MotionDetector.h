#pragma once

#include <vector>

class MotionDetector {
public:
  MotionDetector(const int width, const int height, const double sensitivity,
                 const double forget_factor);

  bool observeFrame(const char* image_data);

private:
  int _width;
  int _height;
  double _sensitivity;
  double _forget_factor;
  double _count;
  std::vector<double> _sums;
  std::vector<double> _sq_sums;
};
