cmake_minimum_required(VERSION 3.16)
project(gstreamer_playback_sample LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED IMPORTED_TARGET
  gstreamer-1.0>=1.18
  gstreamer-app-1.0>=1.18
  gstreamer-video-1.0>=1.18
)

add_executable(gst_player main.cpp)
target_link_libraries(gst_player PRIVATE PkgConfig::GST)
