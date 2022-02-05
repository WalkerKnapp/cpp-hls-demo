cmake_minimum_required(VERSION 3.16)
project(cpp_hls_demo)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(drogon)

add_executable(cpp_hls_demo src/main.cpp src/VideoDistributionServer.cpp src/VideoDistributionServer.h)
target_link_libraries(cpp_hls_demo PRIVATE drogon avformat avcodec)
