#ifndef OPENGUARD_FRAME_PROCESSOR_HPP
#define OPENGUARD_FRAME_PROCESSOR_HPP
#include <opencv2/opencv.hpp>
#include "../capture/capture.hpp"
#include "../detection/motion_detector.hpp"
#include "../detection/object_detector.hpp"
#include "../recorder/recorder.hpp"

class FrameProcessor
{
    public:

    /**
     * @brief Constructor: Initializes the frame processor.
     * @param cap The capture object.
     */
    FrameProcessor(Capture& cap);
    ~FrameProcessor();

    /**
     * @brief Process the frame.
     * @param frame The frame to process.
     */
    void ProcessFrame(cv::Mat& frame);

    /**
     * @brief Render the frame.
     * @return Whether the frame was rendered successfully or if the program should exit.
     */
    bool RenderFrame();

    /// For future flexibility, allow the motion and object detectors to be altered at runtime
    /**
     * @brief Set the motion detector.
     * @param detector The motion detector.
     */
    void SetMotionDetector(std::unique_ptr<MotionDetector> detector);
    /**
     * @brief Set the object detector.
     * @param detector The object detector.
     */
    void SetObjectDetector(std::unique_ptr<ObjectDetector> detector);

    private:
    std::unique_ptr<MotionDetector> motion_detector;   /// The motion detector
    std::unique_ptr<ObjectDetector> object_detector;   /// The object detector

    OpenGuard::Utils::StateTracker motion_state;                           /// [MaybeUnused] The previous motion state
    ObjectDetector::Object object_detected = ObjectDetector::Object::NONE; /// Previous detected object

    std::shared_ptr<OverlayRenderer> overlay_renderer; /// The overlay renderer for this frame

    cv::Mat processed_frame;                           /// The processed frame
    Capture& cap;                                      /// The capture object
    std::unique_ptr<Recorder> recorder;                /// Recorder instance
    int pause_system = 0;                              /// Pause system for a number of seconds
};


#endif //OPENGUARD_FRAME_PROCESSOR_HPP
