#include "HlsStream.h"

#include <fcntl.h>
#include <fstream>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <spdlog/spdlog.h>

HlsStream::HlsStream(const std::string out_dir, const bool live, const size_t max_segment)
  : _out_dir(out_dir)
  , _manifest_path(out_dir + "stream.m3u8")
  , _live(live)
  , _max_segment(max_segment)
  , _start_id(0) {

  mkdir(_out_dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

HlsStream::~HlsStream() {
  flush();

  if (_live) {
    remove(_manifest_path.c_str());

    while (_segment_filenames.size() > 0) {
      const std::string rm_segment_filename = _segment_filenames.front();
      const std::string rm_segment_path = _out_dir + rm_segment_filename;
      _segment_filenames.pop_front();
      remove(rm_segment_path.c_str());
    }
  }
}

void HlsStream::addSegment(const std::string &in_segment_path) {
  const int slash_index = in_segment_path.rfind('/');
  const std::string filename = in_segment_path.substr(slash_index + 1);
  const std::string out_segment_path = _out_dir + filename;

  int in_fd = open(in_segment_path.c_str(), O_RDONLY);
  int out_fd = open(out_segment_path.c_str(), O_RDWR | O_CREAT, S_IRWXU);
  size_t filesize = lseek(in_fd, 0, SEEK_END);
  lseek(in_fd, 0, SEEK_SET);
  sendfile(out_fd, in_fd, NULL, filesize);
  close(in_fd);
  close(out_fd);

  _segment_filenames.push_back(filename);
  if (_live && _segment_filenames.size() > _max_segment) {
    const std::string rm_segment_filename = _segment_filenames.front();
    const std::string rm_segment_path = _out_dir + rm_segment_filename;
    remove(rm_segment_path.c_str());
    _segment_filenames.pop_front();
    _start_id++;
  }
}

void HlsStream::flush() {
  std::ofstream manifest(_manifest_path);
  manifest << "#EXTM3U" << std::endl;
  manifest << "#EXT-X-TARGETDURATION:2" << std::endl;
  manifest << "#EXT-X-VERSION:4" << std::endl;
  manifest << "#EXT-X-MEDIA-SEQUENCE:" << _start_id << std::endl;

  if (!_live) {
    manifest << "#EXT-X-START:TIME-OFFSET=0" << std::endl;
  }

  for (const std::string segment_filename : _segment_filenames) {
    manifest << "#EXTINF:2.0," << std::endl;
    manifest << segment_filename << std::endl;
    manifest << "#EXT-X-DISCONTINUITY" << std::endl;
  }

  manifest.close();
}
