#pragma once

#include <deque>
#include <string>

class HlsStream {
public:
  HlsStream(const std::string out_dir, const bool live, const size_t max_segment = 4);
  ~HlsStream();
  void addSegment(const std::string &segment);
  void flush();

private:
  std::string _out_dir;
  std::string _manifest_path;
  bool _live;
  size_t _max_segment;
  int _start_id;
  std::deque<std::string> _segment_filenames;
};
