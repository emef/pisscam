cmake_minimum_required(VERSION 3.3)
project(pisscam LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

find_package (Threads)

set(SOURCES
    src/common_v4l2.cpp
    src/pisscam.cpp
    src/HlsStream.cpp
    src/MotionDetector.cpp
    src/VideoEncoder.cpp
    src/Webcam.cpp
)

add_executable(pisscam ${SOURCES})
target_compile_options(pisscam PUBLIC -Wall -Werror)
target_link_libraries(pisscam
    CONAN_PKG::spdlog
    CONAN_PKG::ffmpeg
    Threads::Threads
    v4l2)
