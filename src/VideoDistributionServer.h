#ifndef CPP_HLS_DEMO_VIDEODISTRIBUTIONSERVER_H
#define CPP_HLS_DEMO_VIDEODISTRIBUTIONSERVER_H

#include <thread>
#include <boost/lockfree/spsc_queue.hpp>

class VideoDistributionServer {
public:
    VideoDistributionServer();

    void startWebserver(int port);
    void startFeed(int width, int height, int pixelDepth, const std::string& pixelFormat, int fps);

    void provideRawVideoFrame(void *data);

    virtual ~VideoDistributionServer();

private:
    std::thread *webserverThread = nullptr;

    std::thread *feedThread = nullptr;
    FILE *rawVideoPipe = nullptr;
    bool feedCloseRequested = false;
    boost::lockfree::spsc_queue<void *> videoFrameQueue;
    long videoFrameSize = 0;

    void runWebserver(int port);
    void runFeed(int width, int height, int pixelDepth, const std::string& pixelFormat, int fps);
};


#endif //CPP_HLS_DEMO_VIDEODISTRIBUTIONSERVER_H
