#include <chrono>
#include <cstdlib>
#include <fstream>
#include <signal.h>
#include <sstream>
#include <thread>
#include <utility>

#include <spdlog/spdlog.h>

#include "HlsStream.h"
#include "MotionDetector.h"
#include "VideoEncoder.h"
#include "Webcam.h"
#include "types.h"

static bool interrupted = false;

void handle_sigint(int sig) {
  if (interrupted) {
    spdlog::critical("unhandled interrupt");
    exit(1);
  }

  spdlog::warn("Ctrl-C received, shutting down");
  interrupted = true;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    spdlog::critical("Usage: {} OUT_DIR", argv[0]);
    return -1;
  }

  signal(SIGINT, handle_sigint);

  const std::string out_dir(argv[1]);

  Webcam webcam;
  const int width = webcam.width();
  const int height = webcam.height();
  const double fps = webcam.fps();

  const std::string live_stream_dir = out_dir + "/live/";

  auto live_stream = std::make_unique<HlsStream>(live_stream_dir, true);

  MotionDetector detector(width, height, 1000, 0.9);

  std::unique_ptr<HlsStream> recorded_stream;

  const Instant program_start = Clock::now();

  while (!interrupted) {
    const Instant start = Clock::now();
    const Instant end = start + Seconds(2);

    auto encoder = std::make_shared<VideoEncoder>(width, height, fps);
    const std::string new_segment_path = encoder->path();

    bool movement_detected = false;

    while (!interrupted && Clock::now() < end) {
      const WebcamFrame frame = webcam.nextFrame();
      encoder->addFrame(frame.image_data);

      const long since_program_start =
        std::chrono::duration_cast<Seconds>(Clock::now() - program_start).count();

      const bool warmed_up = since_program_start > 10;

      movement_detected |= (warmed_up && detector.observeFrame(frame.image_data));
    }

    if (interrupted) {
      break;
    }

    encoder->flush();
    spdlog::info("new segment at {}", new_segment_path);

    live_stream->addSegment(new_segment_path);
    live_stream->flush();

    if (movement_detected && recorded_stream == nullptr) {
      std::stringstream recorded_out_dir_ss;
      recorded_out_dir_ss
        << out_dir << "/"
        << std::chrono::duration_cast<Milliseconds>(start.time_since_epoch()).count()
        << "/";

      const std::string recorded_out_dir = recorded_out_dir_ss.str();

      recorded_stream = std::make_unique<HlsStream>(recorded_out_dir, false);
    } else if (!movement_detected && recorded_stream != nullptr) {
      recorded_stream->flush();
      recorded_stream.reset(nullptr);
    }

    if (recorded_stream != nullptr) {
      recorded_stream->addSegment(new_segment_path);
    }
  }

  return 0;
}
