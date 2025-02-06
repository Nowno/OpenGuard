cmake_minimum_required(VERSION 3.29)
project(OpenGuard)

# Set the directory where OpenCV is built
set(OpenCV_DIR "C:/Users/Nono/Desktop/Stuff/Coding/Libs/OpenCv/opencv_last/opencv/build")

# Find OpenCV package
find_package(OpenCV REQUIRED)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the executable
add_executable(OpenGuard main.cpp
        core/capture/capture.cpp
        core/capture/capture.hpp
        util/utils.hpp
        core/frame_processor/frame_processor.cpp
        core/frame_processor/frame_processor.hpp
        core/detection/motion_detector.hpp
        core/detection/mog2/mog2.cpp
        core/detection/mog2/mog2.hpp
        core/detection/object_detector.hpp
        core/detection/yolo/yolo.cpp
        core/detection/yolo/yolo.hpp
        util/user_config.hpp
        core/overlay_renderer/overlay_renderer.cpp
        core/overlay_renderer/overlay_renderer.hpp
)

# Link the OpenCV libraries
target_link_libraries(OpenGuard ${OpenCV_LIBS})

# Include OpenCV headers
include_directories(${OpenCV_INCLUDE_DIRS})
