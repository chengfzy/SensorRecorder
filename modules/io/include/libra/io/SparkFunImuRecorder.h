#pragma once
#include <boost/asio.hpp>
#include "libra/io/IRecorder.hpp"
#include "libra/io/TimestampRetrieveMethod.hpp"

namespace libra {
namespace io {

/**
 * @brief Recorder for SparkFun 9DoF Razor IMU M0.
 *  Ref Link:
 *      (1) https://www.sparkfun.com/products/14001
 *      (2) https://learn.sparkfun.com/tutorials/9dof-razor-imu-m0-hookup-guide
 *
 * The IMU could be connected with PC using USB, then use command `ls /dev` to check the new added port name, in my
 * computer the name is "/dev/ttyACM0". After connection, the data of IMU are sent to PC using serial port automatically
 * with frequency of 100Hz. The data format is:
 *  (0) Index, just for counting
 *  (1) IMU timestamp (ms)
 *  (2) Acceleration in X (g)
 *  (3) Acceleration in Y (g)
 *  (4) Acceleration in Z (g)
 *  (5) Gyroscope in X (deg/s)
 *  (6) Gyroscope in Y (deg/s)
 *  (7) Gyroscope in Z (deg/s)
 *  (8) Magnetometer in X
 *  (9) Magnetometer in Y
 *  (10) Magnetometer in Z
 *  (11) Error Check ? (not sure right now)
 */
class SparkFunImuRecorder : public IRecorder {
  public:
    /**
     * @brief Constructor with IMU device
     * @param device    IMU device, could checked from command `ls /dev`, this value is "/dev/ttyACM0" in my computer
     */
    explicit SparkFunImuRecorder(const std::string& device);

    /**
     * @brief Default destructor, stop and wait thread finish
     */
    ~SparkFunImuRecorder() override;

  public:
    /**
     * @brief Get the IMU device name
     *
     * @return IMU device name
     */
    inline const std::string& device() const { return device_; }

    /**
     * @brief Set the IMU device name. Should be set before `init()`
     *
     * @param device IMU device name
     */
    void setDevice(const std::string& device);

    /**
     * @brief Get the method to retrieve timestamp
     *
     * @return Method to retrieve timestamp
     */
    inline TimestampRetrieveMethod timestampRetrieveMethod() { return timestampMethod_; }

    /**
     * @brief Initialize IMU device
     */
    void init() override;

    /**
     * @brief Set the method to retrieve timestamp
     *
     * @param method Timestamp retrieve method
     */
    void setTimeStampRetrieveMethod(TimestampRetrieveMethod method);

  protected:
    /**
     * @brief The whole run function
     */
    void run() override;

  private:
    /**
     * @brief Parse IMU data and set to ImuRecord
     * @param record    IMU Record, and the reading will be set in this function, but timestamp will not change
     * @param raw       Raw string received from serial port
     * @return True if no error occurs and parse finish, false for something wrong
     */
    bool parseImuData(core::ImuRecord& record, const std::string& raw);

    /**
     * @brief Close device
     */
    void closeDevice();

  private:
    std::string device_;                       // device name
    TimestampRetrieveMethod timestampMethod_;  // timestamp retrieve method

    boost::asio::io_service io_;         // IO context
    boost::asio::serial_port port_;      // IMU serial port
    boost::asio::streambuf portBuffer_;  // serial port buffer
};

}  // namespace io
}  // namespace libra