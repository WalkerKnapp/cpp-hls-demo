#include "VideoDistributionServer.h"

#include <drogon/drogon.h>
#include <filesystem>
#include <thread>
#include <iostream>
#include <iomanip>
#include <stdio.h>

const std::filesystem::path VIDEO_OUTPUT_PATH = "./live";

VideoDistributionServer::VideoDistributionServer() : videoFrameQueue(30) {
}


void VideoDistributionServer::startWebserver(int port) {
    // If we have an existing webserver, kill it and wait for its thread to die
    if (this->webserverThread) {
        drogon::app().quit();
        this->webserverThread->join();
    }

    this->webserverThread = new std::thread(&VideoDistributionServer::runWebserver, this, port);
}

void VideoDistributionServer::runWebserver(int port) {
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
            .addListener("0.0.0.0", port)
            .setThreadNum(1)
            .run();

    std::cout << "Listening for HTTP connections on port " << port << std::endl;
}

void VideoDistributionServer::startFeed(int width, int height, int pixelDepth, const std::string& pixelFormat, int fps) {
    // If we have an existing feed, kill it and wait for its thread to die
    if (this->feedThread) {
        this->feedCloseRequested = true;
        this->feedThread->join();
    }

    this->feedCloseRequested = false;
    this->feedThread = new std::thread(&VideoDistributionServer::runFeed, this, width, height, pixelDepth, pixelFormat, fps);
}

void VideoDistributionServer::runFeed(int width, int height, int pixelDepth, const std::string& pixelFormat, int fps) {
    this->videoFrameSize = width * height * pixelDepth;

    // Open ffmpeg on the command line, configured to receive raw video over a pipe and output an HLS stream
    std::stringstream sstm;
    sstm << "ffmpeg -y"
         << " -f rawvideo -vcodec rawvideo -s " << width << "x" << height << " -pix_fmt " << pixelFormat << " -r " << fps << " -i -" // Input spec
         << " -c:v h264 -flags +cgop -hls_time 2 -hls_list_size 2 -hls_delete_threshold 2 -hls_flags delete_segments -hls_segment_type mpegts " << VIDEO_OUTPUT_PATH << "/feed.m3u8"; // Output spec

    if (!(this->rawVideoPipe = popen(sstm.str().c_str(), "w"))) {
        std::cerr << "Failed to open ffmpeg" << std::endl;
        return;
    }

    // Spin wait for new
    void *imgData;

    while(true) {
        if (this->feedCloseRequested) {
            fclose(rawVideoPipe);
            return;
        }

        // If we have a new video frame available, write it to the feed
        if (this->videoFrameQueue.pop(&imgData, 1)) {
            fwrite(imgData, 1, this->videoFrameSize, rawVideoPipe);
            fflush(rawVideoPipe);
            free(imgData);
        }
    }

}


void VideoDistributionServer::provideRawVideoFrame(void *data) {
    void *dataCopy = malloc(this->videoFrameSize);
    memcpy(dataCopy, data, this->videoFrameSize);

    this->videoFrameQueue.push(dataCopy);
}

VideoDistributionServer::~VideoDistributionServer() {
    drogon::app().quit();
}

