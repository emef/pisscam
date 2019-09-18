#include <math.h>
#include <string>

extern "C" {
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
}

class VideoEncoder {
public:
  VideoEncoder(const int width, const int height, const int fps);
  ~VideoEncoder();

  void addFrame(const char *img_data);
  void flush();

  inline std::string path() const { return _path; }

private:
  void drain();

  int _width;
  int _height;
  int _fps;
  int _pts;  // current frame timestamp

  int _fd;
  std::string _path;

  AVCodec *_codec;
  AVCodecContext *_encoder;
  AVFormatContext *_muxer;
  AVStream *_video_track;
  AVFrame *_raw_frame;
  AVPacket *_encoded_packet;
};
