#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#include "common_v4l2.h"
#include "types.h"

class Webcam {
public:
  Webcam();
  ~Webcam();

  WebcamFrame nextFrame();

  int width() const { return _width; }
  int height() const { return _height; }
  double fps() const { return _fps; }

private:
  void readThreadFunc();

  CommonV4l2 _v4l2;
  int _width;
  int _height;
  int _fps;
};
