#pragma once
#include <mynteyed/camera.h>
#include <Eigen/Core>
#include "libra/io/IRecorder.hpp"
#include "libra/io/TimestampRetrieveMethod.hpp"
#include "turbojpeg.h"

namespace libra {
namespace io {

/**
 * @brief Recorder for MYNT EYE device, MYNT-EYE-D1000-IR-120/Color.
 *
 *  Ref:
 *      1. http://www.myntai.com/cn/mynteye/depth
 *      2. https://mynt-eye-d-sdk.readthedocs.io/zh_CN/latest/index.html
 *      3. https://github.com/slightech/MYNT-EYE-D-SDK
 *
 * The MYNT EYE device has two global shutter camera(left and right) and one IMU, but the obtained IMU acc and gyro
 * don't have the same timestamp, so I just complete the camera recorder.
 *
 * This class has two ImageRecord process function: (1)left (2)right. If only enable one camera, it will use the left
 * one.
 */
class MyntEyeRecorder : public IRecorder {
  private:
    /**
     * @brief Raw image with timestamp.
     *
     * @note Could also get the exposure time, frame id info
     */
    struct RawImage {
        std::shared_ptr<mynteyed::Image> img;  //!< image data pointer
        double timestamp = 0;                  //!< timestamp, s
    };

  public:
    /**
     * @brief Constructor
     * @param index             Device Index
     * @param frameRate         Frame rate(Hz)
     * @param saverThreadNum    Image saver thread number
     */
    explicit MyntEyeRecorder(unsigned int index = 0, unsigned int frameRate = 30,
                             const std::size_t& saverThreadNum = 2);

    /**
     * @brief Destructor
     */
    ~MyntEyeRecorder() override;

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
    inline unsigned int deviceIndex() const { return deviceIndex_; }

    /**
     * @brief Get the frame rate
     *
     * @return  Frame rate, Hz
     */
    inline unsigned int frameRate() const { return frameRate_; }

    /**
     * @brief Get the stream mode
     *
     * @return  Stream mode
     */
    inline mynteyed::StreamMode streamMode() const { return streamMode_; }

    /**
     * @brief Get the stream format
     *
     * @return  Stream format
     */
    inline mynteyed::StreamFormat streamFormat() const { return streamFormat_; }

    /**
     * @brief Get the image saver thread number
     *
     * @return  Image saver thread number
     */
    inline const std::size_t& saverThreadNum() const { return saverThreadNum_; }

    /**
     * @brief Get the method to retrieve timestamp
     *
     * @return Method to retrieve timestamp
     */
    inline TimestampRetrieveMethod timestampRetrieveMethod() const { return timestampMethod_; }

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
    void setDeviceIndex(unsigned int deviceIndex);

    /**
     * @brief Set frame rate, should call init to apply this setting
     *
     * @param frameRate Frame rate, [0, 60] Hz
     */
    void setFrameRate(const unsigned int& frameRate);

    /**
     * @brief Set the stream mode of device, could set the image size and camera number(left or left+right)
     *
     * @param streamMode Stream mode
     */
    void setStreamMode(mynteyed::StreamMode streamMode);

    /**
     * @brief Set the stream format of device, the format used for data transferring
     *
     * @param streamFormat Stream format
     */
    void setStreamFormat(mynteyed::StreamFormat streamFormat);

    /**
     * @brief Set the saver thread number, should be set before `init()`
     *
     * @param saverThreadNum  Saver thread number
     */
    void setSaverThreadNum(const std::size_t& saverThreadNum);

    /**
     * @brief Set the method to retrieve timestamp
     *
     * @param method Timestamp retrieve method
     */
    void setTimeStampRetrieveMethod(TimestampRetrieveMethod method);

    /**
     * @brief Set process function for raw image record of right camera
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
     * @brief Create thread to save image, first compress image using turbo jpeg, then save to file
     */

    /**
     * @brief Create thread to save image, first compress image using turbo jpeg, then save to file
     */
    void createImageSaverThread();

    /**
     * @brief Get the timestamp based on timestamp retrieve method
     *
     * @param imageInfo Image information obtained from device
     * @return  Timestamp, s
     */
    double getTimestamp(const std::shared_ptr<mynteyed::ImgInfo>& imageInfo);

  private:
    unsigned int deviceIndex_;                 // device index
    unsigned int frameRate_;                   // frame rate, Hz
    mynteyed::StreamMode streamMode_;          // stream mode, could set image size and camera num(left or left + right)
    mynteyed::StreamFormat streamFormat_;      // stream format, the format used for data transferring
    std::size_t saverThreadNum_;               // image saver thread number
    TimestampRetrieveMethod timestampMethod_;  // timestamp retrieve method
    // raw image record process function for right camera
    std::function<void(const core::RawImageRecord&)> processRightRawImg_;

    // camera pointer, cannot use object due to the watch dog in MyntEye SDK(device.h)
    std::shared_ptr<mynteyed::Camera> cam_;
    bool isRightCamEnabled_;                                     // right camera is enable or not
    std::shared_ptr<util::JobQueue<RawImage>> leftImageQueue_;   // left raw image queue
    std::shared_ptr<util::JobQueue<RawImage>> rightImageQueue_;  // right raw image queue
    std::vector<std::thread> leftImageSaverThreads_;             // image saver threads
    std::vector<std::thread> rightImageSaverThreads_;            // image saver threads
};

}  // namespace io
}  // namespace libra
