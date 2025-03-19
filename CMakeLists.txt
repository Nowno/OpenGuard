cmake_minimum_required(VERSION 3.29)
project(OpenGuard)

# Set the directory where OpenCV is built
set(OpenCV_DIR "C:/Users/Nono/Desktop/Stuff/Coding/Libs/OpenCv/opencv_last/opencv/build")

# Find OpenCV package
find_package(OpenCV REQUIRED COMPONENTS)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(libs INTERFACE)
target_include_directories(libs INTERFACE ${CMAKE_SOURCE_DIR}/utils/libs)

# Add the executable
add_executable(OpenGuard main.cpp
        core/capture/capture.cpp
        core/capture/capture.hpp
        utils/utils.hpp
        core/frame_processor/frame_processor.cpp
        core/frame_processor/frame_processor.hpp
        core/detection/motion_detector.hpp
        core/detection/mog2/mog2.cpp
        core/detection/mog2/mog2.hpp
        core/detection/object_detector.hpp
        core/detection/yolo/yolo.cpp
        core/detection/yolo/yolo.hpp
        utils/config_manager/config_manager.hpp
        core/overlay_renderer/overlay_renderer.cpp
        core/overlay_renderer/overlay_renderer.hpp
        core/recorder/recorder.cpp
        core/recorder/recorder.hpp
        utils/logger/logger.cpp
        utils/logger/logger.hpp
        utils/config_manager/config_manager.cpp
        core/hook_manager/hook_manager.cpp
        core/hook_manager/hook_manager.hpp
        core/openguard.hpp
        utils/worker_thread/worker_thread.tpp
        utils/worker_thread/worker_thread.hpp
        core/command_processor/command_processor.cpp
        core/command_processor/command_processor.hpp
        core/server/ws_server.cpp
        core/server/ws_server.hpp
)


# Link the OpenCV libraries
target_link_libraries(OpenGuard ${OpenCV_LIBS} libs)

# Include OpenCV headers
include_directories(${OpenCV_INCLUDE_DIRS})
