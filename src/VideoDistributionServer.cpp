#include "VideoDistributionServer.h"

#include <drogon/drogon.h>
#include <filesystem>

const std::filesystem::path VIDEO_OUTPUT_PATH = "./live";

VideoDistributionServer::VideoDistributionServer() {
    std::filesystem::create_directory(VIDEO_OUTPUT_PATH);

    // Spin up a dirt-simple Drogon webserver to serve an index file and every file from /live
    drogon::app()
            .registerHandler("/",
                             [] (const drogon::HttpRequestPtr &req,
                                 std::function<void (const drogon::HttpResponsePtr &)> &&callback)
                             {
                std::string index = R"html(
<html>
    <head>
		<script src="https://cdn.jsdelivr.net/npm/hls.js@latest"></script>
    </head>

	<body>
        <div style="width 100%; display: flex; flex-flow: column nowrap; align-items: center">
		    <video id="video" style="width: 80%; aspect-ratio: 16/9; background-color: black;" autoplay></video>
            <button onclick="playFeed()">Play feed</button>
        </div>
	</body>

    <script>
        var video = document.getElementById('video');

        function playFeed() {
            var videoSrc = '/live/feed.m3u8';

  		    if (video.canPlayType('application/vnd.apple.mpegurl')){
                video.src = videoSrc;
                video.addEventListener('loadedmetadata', () => video.play());
            } else if (Hls.isSupported()) {
    		    var hls = new Hls();
    		    hls.loadSource(videoSrc);
    		    hls.attachMedia(video);
                hls.on(Hls.Events.MANIFEST_PARSED, () => video.play());
  		    } else {
                console.log("this browser can't play hls");
            }
        }
	</script>
</html>)html";
                                 drogon::HttpResponsePtr response = drogon::HttpResponse::newHttpResponse();
                                 response->setBody(index);
                                 response->setContentTypeString("text/html");
                                 callback(response);

                             })
            .registerHandler("/live/{requestedPath}",
                             [] (const drogon::HttpRequestPtr &req,
                                 std::function<void (const drogon::HttpResponsePtr &)> &&callback,
                                 const std::string &requestedPath)
                             {
                                 // Search for the requested file in ./live
                                 for (const auto & subfile : std::filesystem::directory_iterator(VIDEO_OUTPUT_PATH)) {
                                     if (!subfile.is_directory() && subfile.path().filename() == requestedPath) {
                                         auto absolutePath = std::filesystem::absolute(subfile.path());

                                         // Serve the file we found
                                         callback(drogon::HttpResponse::newFileResponse(absolutePath.string()));
                                         return;
                                     }
                                 }

                                 // Serve a 404
                                 callback(drogon::HttpResponse::newNotFoundResponse());
                             })
                             .addListener("0.0.0.0", 8080)
                             .setThreadNum(1)
                             .run();
}

VideoDistributionServer::~VideoDistributionServer() {
    drogon::app().quit();
}
