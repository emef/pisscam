var video_source = "/test_stream/stream.m3u8";
var video = document.getElementById('video');
var error = document.getElementById('error');
error.innerHTML = "init";

if (Hls.isSupported()) {
  console.log("HLS supported");

  var hls = new Hls();

  hls.attachMedia(video);

  hls.on(Hls.Events.MEDIA_ATTACHED, function () {
    console.log("video and hls.js are now bound together !");
    hls.loadSource(video_source);
    hls.on(Hls.Events.MANIFEST_PARSED, function (event, data) {
      console.log("manifest parsed");
      video.play();
    });
  });

  hls.on(Hls.Events.ERROR, function (event, data) {
    if (data.fatal) {
      switch(data.type) {
      case Hls.ErrorTypes.NETWORK_ERROR:
        // try to recover network error
        console.log("fatal network error encountered, try to recover");
        hls.startLoad();
        break;
      case Hls.ErrorTypes.MEDIA_ERROR:
        console.log("fatal media error encountered, try to recover");
        hls.recoverMediaError();
        break;
      default:
        // cannot recover
        hls.destroy();
        break;
      }
    }
  });

} else if (video.canPlayType('application/vnd.apple.mpegurl')) {
  video.src = video_source;
  video.addEventListener('loadedmetadata', function() {
    video.play();
  });

} else {
  error.innerHTML = "HLS not supported";
}
