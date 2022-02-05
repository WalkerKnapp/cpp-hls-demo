#include <iostream>
#include <libavcodec/avcodec.h>

#include "VideoDistributionServer.h"

#include <chrono>
#include <thread>

int main() {
    VideoDistributionServer distributionServer;

    // Start the webserver
    distributionServer.startWebserver(8080);
    // Start the ffmpeg feed (this can be called multiple times if necessary for changing w/h/d)
    distributionServer.startFeed(1920, 1080, 3, "rgb24", 60);

    void *someData = malloc(1920 * 1080 * 3);
    while (true) {
        memset(someData, 0xffff, 1920 * 1080 * 3);

        // Feed a raw video frame to the server.
        // This method is non-blocking, and copies the data, so the buffer can be immediately reused.
        distributionServer.provideRawVideoFrame(someData);

        std::this_thread::sleep_for(std::chrono::milliseconds(1000/60));
    }

    std::cout << &distributionServer << std::endl;
}
