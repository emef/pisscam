#include "VideoEncoder.h"

#include <stdexcept>
#include <utility>

#include <libavutil/log.h>

VideoEncoder::VideoEncoder(const int width, const int height, const int fps)
: _width(width)
, _height(height)
, _fps(fps)
, _pts(0) {

  av_log_set_level(AV_LOG_QUIET);

  char tmp_path[] = "/tmp/hls_XXXXXX.ts";
  _fd = mkstemps(tmp_path, 3);
  _path = std::string(tmp_path);

  const AVCodecID codec_id = AV_CODEC_ID_H264;

  _codec = avcodec_find_encoder(codec_id);
  if (!_codec) {
    throw std::runtime_error("Codec not found");
  }

  _encoder = avcodec_alloc_context3(_codec);
  if (!_encoder) {
    throw std::runtime_error("Could not allocate video codec context");
  }

  _encoder->bit_rate = 400000;
  _encoder->width = _width;
  _encoder->height = _height;
  _encoder->time_base = (AVRational) {1, fps};
  _encoder->gop_size = 10;
  _encoder->max_b_frames = 1;
  _encoder->pix_fmt = AV_PIX_FMT_YUV420P;

  av_opt_set(_encoder->priv_data, "preset", "ultrafast", 0);

  if (avcodec_open2(_encoder, _codec, NULL) < 0) {
    throw std::runtime_error("Could not open codec");
  }

  _muxer = avformat_alloc_context();
  _muxer->oformat = av_guess_format("mpegts", NULL, NULL);
  if (avio_open(&_muxer->pb, tmp_path, AVIO_FLAG_WRITE) < 0) {
    spdlog::info("Error opening file {} for encoding", tmp_path);
    throw std::runtime_error("Could not open output file");
  }

  _video_track = avformat_new_stream(_muxer, NULL);
  _muxer->oformat->video_codec = codec_id;

  avcodec_parameters_from_context(_video_track->codecpar, _encoder);
  _video_track->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
  _video_track->time_base = (AVRational) {1, fps};
  _video_track->avg_frame_rate = (AVRational) {fps, 1};

  _raw_frame = av_frame_alloc();
  if (!_raw_frame) {
    throw std::runtime_error("Could not allocate video frame");
  }

  _raw_frame->format = _encoder->pix_fmt;
  _raw_frame->width  = _encoder->width;
  _raw_frame->height = _encoder->height;

  int ret = av_image_alloc(_raw_frame->data, _raw_frame->linesize, _encoder->width,
                           _encoder->height, _encoder->pix_fmt, 32);

  if (ret < 0) {
    throw std::runtime_error("Could not allocate frame image");
  }

  _encoded_packet = av_packet_alloc();
  if (!_encoded_packet) {
    throw std::runtime_error("Could not allocated packet\n");
  }

  // Write video header
  if (0 != avformat_write_header(_muxer, NULL)) {
    throw std::runtime_error("Error writing video header");
  }
}

VideoEncoder::~VideoEncoder() {
  avcodec_close(_encoder);
  av_freep(&_raw_frame->data[0]);
  av_frame_free(&_raw_frame);
  av_packet_free(&_encoded_packet);
  avio_closep(&_muxer->pb);
  avformat_free_context(_muxer);
  remove(_path.c_str());
}

void VideoEncoder::addFrame(const char *image_data) {
  _raw_frame->pts = _pts++;

  const size_t y_length = _width * _height;
  const size_t cb_cr_length = (_width / 2) * (_height / 2);

  memcpy(_raw_frame->data[0], &image_data[0], y_length);
  memcpy(_raw_frame->data[1], &image_data[y_length], cb_cr_length);
  memcpy(_raw_frame->data[2], &image_data[y_length + cb_cr_length], cb_cr_length);

  int ret = avcodec_send_frame(_encoder, _raw_frame);
  if (ret < 0) {
    throw std::runtime_error("Error sending a frame for encoding");
  }

  drain();
}

void VideoEncoder::flush() {
  int ret;

  ret = avcodec_send_frame(_encoder, NULL);
  if (ret < 0) {
    throw std::runtime_error("Error sending NULL frame for encoding");
  }

  drain();

  ret = av_write_trailer(_muxer);
  if (ret != 0) {
    throw std::runtime_error("Error writing trailer");
  }
}

void VideoEncoder::drain() {
  while (1) {
    int ret = avcodec_receive_packet(_encoder, _encoded_packet);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
      break;
    else if (ret < 0) {
      throw std::runtime_error("Error during encoding");
    }

    int64_t scaled_pts = av_rescale_q(_encoded_packet->pts, _encoder->time_base, _video_track->time_base);
    _encoded_packet->pts = scaled_pts;

    int64_t scaled_dts = av_rescale_q(_encoded_packet->dts, _encoder->time_base, _video_track->time_base);
    _encoded_packet->dts = scaled_dts;

    _encoded_packet->stream_index = _video_track->index;

    av_interleaved_write_frame(_muxer, _encoded_packet);
    av_packet_unref(_encoded_packet);
  }
}
