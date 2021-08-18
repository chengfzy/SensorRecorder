#pragma once
#include <Eigen/Core>
#include <zed-open-capture/sensorcapture.hpp>
#include <zed-open-capture/videocapture.hpp>
#include "libra/io/IRecorder.hpp"
#include "turbojpeg.h"

namespace libra {
namespace io {

/**
 * @brief Recorder for ZED device using ZED Open Capture library, test on ZED Mini Camera
 *
 *  Ref:
 *      1. https://github.com/stereolabs/zed-open-capture
 *
 * @note The ZED Mini device has two rolling shutter camera(left and right) and one IMU
 *
 * This class has two ImageRecord process function: (1)left (2)right. If only enable one camera, it will use the left
 * one.
 */
class ZedOpenRecorder : public IRecorder {
  private:
    /**
     * @brief Raw IMU with system time point
     *
     */
    struct RawImu {
        sl_oc::sensors::data::Imu imu;                                  // IMU data pointer
        std::chrono::time_point<std::chrono::system_clock> systemTime;  // system time point
    };

  public:
    /**
     * @brief Constructor
     * @param index             Device Index, if set to -1, it will open all the devices until the first success
     * @param saverThreadNum    Image saver thread number
     */
    explicit ZedOpenRecorder(int index = -1, const std::size_t& saverThreadNum = 2);

    /**
     * @brief Destructor
     */
    ~ZedOpenRecorder() override;

  public:
    /**
     * @brief Get the device list, <ID, name>. The ID is the device index, the name could be some info about this device
     *
     * @return  Device list with ID and name
     */
    static std::vector<std::pair<unsigned int, std::string>> getDevices();

    /**
     * @brief Get the device index
     *
     * @return  Device index
     */
    inline int deviceIndex() const { return deviceIndex_; }

    /**
     * @brief Get the FPS
     *
     * @return  FPS, Hz
     */
    inline sl_oc::video::FPS fps() const { return fps_; }

    /**
     * @brief Get the resolution
     *
     * @return  Resolution
     */
    inline sl_oc::video::RESOLUTION resolution() const { return resolution_; }

    /**
     * @brief Get the image saver thread number
     *
     * @return  Image saver thread number
     */
    inline const std::size_t& saverThreadNum() const { return saverThreadNum_; }

    /**
     * @brief Is the right camera is enabled
     *
     * @return  True for the right camera is enabled, false for not.
     */
    inline bool isRightCamEnabled() const { return isRightCamEnabled_; }

    /**
     * @brief Set the device index
     *
     * @param deviceIndex Device Index
     */
    void setDeviceIndex(int deviceIndex);

    /**
     * @brief Set fps
     *
     * @param fps FPS
     */
    void setFps(sl_oc::video::FPS fps);

    /**
     * @brief Set resolution
     *
     * @param resolution Resolution
     */
    void setResolution(sl_oc::video::RESOLUTION resolution);

    /**
     * @brief Set the saver thread number, should be set before `init()`
     *
     * @param saverThreadNum  Saver thread number
     */
    void setSaverThreadNum(const std::size_t& saverThreadNum);

    /**
     * @brief Set process function for raw image record of right camera
     *
     * @param func Process function for raw image record of right camera
     */
    void setRightProcessFunction(const std::function<void(const core::RawImageRecord&)>& func);

    /**
     * @brief Initialize device
     */
    void init() override;

  protected:
    /**
     * @brief The main run function
     */
    void run() override;

    /**
     * @brief Open and set device
     */
    void openDevice();

    /**
     * @brief Create thread to save image and IMU. For image, first compress image using turbo jpeg, then save to file.
     * For IMU, just convert its unit and save to file
     */
    void createSaverThread();

    /**
     * @brief Create thread for save image
     */
    void createImageSaverThread();

    /**
     * @brief Create thread for save IMU
     */
    void createImuSaverThread();

  private:
    int deviceIndex_;                      // device index
    sl_oc::video::FPS fps_;                // frame rate, Hz
    sl_oc::video::RESOLUTION resolution_;  // resolution
    std::size_t saverThreadNum_;           // image saver thread number

    // raw image record process function for right camera
    std::function<void(const core::RawImageRecord&)> processRightRawImg_;
    bool isRightCamEnabled_;  // right camera is enable or not

    std::shared_ptr<sl_oc::sensors::SensorCapture> imuCapture_;             // sensor(IMU) capture
    std::shared_ptr<sl_oc::video::VideoCapture> cameraCapture_;             // video(camera) capture
    std::shared_ptr<util::JobQueue<sl_oc::video::Frame>> leftImageQueue_;   // left raw image queue
    std::shared_ptr<util::JobQueue<sl_oc::video::Frame>> rightImageQueue_;  // right raw image queue
    std::shared_ptr<util::JobQueue<RawImu>> imuQueue_;                      // raw IMU queue
    std::vector<std::thread> leftImageSaverThreads_;                        // image saver threads
    std::vector<std::thread> rightImageSaverThreads_;                       // image saver threads
    std::thread imuSaverThread_;                                            // IMU saver thread
};

}  // namespace io
}  // namespace libra
