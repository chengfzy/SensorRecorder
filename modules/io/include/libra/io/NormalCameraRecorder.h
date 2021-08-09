#pragma once
#include <Eigen/Core>
#include <opencv2/videoio.hpp>
#include "libra/io/IRecorder.hpp"

namespace libra {
namespace io {

/**
 * @brief Recorder for normal camera. The camera could use `OpenCV` to open and capture the images.
 */
class NormalCameraRecorder : public IRecorder {
  public:
    /**
     * @brief Construct with device name and image saver thread number
     *
     * @param device          Camera device name
     * @param saverThreadNum  Image saver thread number
     */
    explicit NormalCameraRecorder(const std::string& device, const std::size_t& saverThreadNum = 2);

    /**
     * @brief Destructor
     */
    ~NormalCameraRecorder() override;

  public:
    /**
     * @brief Get the Camera device name
     *
     * @return Camera device name
     */
    inline const std::string& device() const { return device_; }

    /**
     * @brief Get the FPS of this device. Should call `init()` first to obtain correct value.
     *
     * @return  FPS of this device
     */
    inline const double& fps() const { return fps_; }

    /**
     * @brief Get the frame size of captured image. Should call `init()` first to obtain correct value.
     *
     * @return  The frame size of captured image
     */
    inline const Eigen::Vector2i& frameSize() const { return frameSize_; }

    /**
     * @brief Get the frame width of captured image. Should call `init()` first to obtain correct value.
     *
     * @return  The frame width of captured image
     */
    int frameWidth() const { return frameSize_[0]; }

    /**
     * @brief Get the frame height of captured image. Should call `init()` first to obtain correct value.
     *
     * @return  The frame height of captured image
     */
    int frameHeight() const { return frameSize_[1]; }

    /**
     * @brief Get the image saver thread number
     *
     * @return  Image saver thread number
     */
    inline const std::size_t& saverThreadNum() const { return saverThreadNum_; }

    /**
     * @brief Set the Camera device name. Should be set before `init()`
     *
     * @param device Camera device name
     */
    void setDevice(const std::string& device);

    /**
     * @brief Set the FPS to device. Should be set after `init()`
     *
     * @param fps FPS to be set
     * @return The setted FPS value, maybe different to input.
     */
    const double& setFps(const double& fps);

    /**
     * @brief Set the frame size of device. Should be set after `init()`
     *
     * @param size  Frame size to be set
     * @return  The setted frame size, maybe different to input.
     */
    const Eigen::Vector2i& setFrameSize(const Eigen::Vector2i& size);

    /**
     * @brief Set the saver thread number. Should be set before `init()`
     *
     * @param saverThreadNum Saver thread number
     */
    void setSaverThreadNum(const std::size_t& saverThreadNum);

    /**
     * @brief Initialize camera
     */
    void init() override;

  protected:
    /**
     * @brief The main run function
     */
    void run() override;

  private:
    /**
     * @brief Create thread to save image to save image to file
     */
    void createImageSaverThread();

  private:
    std::string device_;          // device name
    double fps_;                  // FPS
    Eigen::Vector2i frameSize_;   // frame size, width x height
    std::size_t saverThreadNum_;  // image saver thread number

    ::cv::VideoCapture videoCapture_;                                // video capture
    std::shared_ptr<util::JobQueue<core::ImageRecord>> imageQueue_;  // image queue
    std::vector<std::thread> imageSaverThreads_;                     // image saver threads
};

}  // namespace io
}  // namespace libra