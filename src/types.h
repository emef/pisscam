#pragma once

#include <chrono>

using Clock = std::chrono::system_clock;
using Instant = std::chrono::time_point<Clock>;
using Seconds = std::chrono::seconds;
using Milliseconds = std::chrono::milliseconds;

struct WebcamFrame {
  Instant timestamp;
  int width;
  int height;
  const char* image_data;
};
