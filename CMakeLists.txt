cmake_minimum_required(VERSION 3.16)
project(cpp_hls_demo)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(drogon)

find_package(Boost COMPONENTS system filesystem REQUIRED)

add_executable(cpp_hls_demo src/main.cpp src/VideoDistributionServer.cpp src/VideoDistributionServer.h)
target_link_libraries(cpp_hls_demo PRIVATE drogon avformat avcodec ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY})
