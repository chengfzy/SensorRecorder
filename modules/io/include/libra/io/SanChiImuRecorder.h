#pragma once
#include <boost/asio.hpp>
#include "libra/io/IRecorder.hpp"
#include "libra/io/TimestampRetrieveMethod.hpp"

namespace libra {
namespace io {

/**
 * @brief Recorder for SanChi SC-AHRS-100D2 IMU.
 *  Ref Link:
 *      (1) http://www.beijingsanchi.com/show/view?id=47
 *      (2) Document: https://pan.baidu.com/s/10oTcaARdI5tQ45CA3gbYNA. password: ca82
 *
 * The IMU could be connected with PC using USB, then use command `ls /dev` to check the new added port name, in my
 * computer the name is "/dev/ttyUSB0". Send `START` command using serial port, the data of IMU are sent to PC with hex
 * data automatically. If you want to stop recording, the `STOP` command should be sent to IMU.
 *
 */
class SanChiImuRecorder : public IRecorder {
  public:
    /**
     * @brief Command message which send to IMU using serial port and control the IMU
     */
    class CommandMessage {
      public:
        /**
         * @brief Command
         */
        enum class Command {
            Start,    //!< start
            Stop,     //!< stop
            SetFreq,  //!< set frequency, 10-100Hz
        };

      public:
        /**
         * @brief Constructor with command, which used for the command without any other input
         * @param cmd   Command
         */
        explicit CommandMessage(Command cmd);

        /**
         * @brief Constructor with command and data, which used for the command with data input
         * Currently only `Set Frequency` command is support, the data is the setting IMU frequency (10-100Hz)
         * @param cmd   Command
         * @param value Command setting value
         */
        explicit CommandMessage(Command cmd, unsigned short value);

        /**
         * @brief Default destructor
         */
        ~CommandMessage() = default;

      public:
        /**
         * @brief Get message of this command
         * @return  Message which will be sent with serial port
         */
        std::vector<unsigned char> message() const;

      private:
        unsigned char len_;                // length(without header)
        std::vector<unsigned char> data_;  // command or data
    };

  public:
    /**
     * @brief Constructor with IMU device and output frequency
     * @param device  IMU device, could checked from command `ls /dev`, this value is "/dev/ttyUSB0" in my computer
     * @param freq    IMU output frequency
     * @sa setFrequency
     */
    explicit SanChiImuRecorder(const std::string& device, unsigned short freq = 100);

    /**
     * @brief Default destructor, stop and wait thread finish
     */
    ~SanChiImuRecorder() override;

  public:
    /**
     * @brief Get the IMU device name
     *
     * @return IMU device name
     */
    inline const std::string& device() const { return device_; }

    /**
     * @brief Get the output frequency
     *
     * @return  IMU output frequency(Hz)
     */
    inline unsigned short frequency() const { return freq_; }

    /**
     * @brief Get the method to retrieve timestamp
     *
     * @return Method to retrieve timestamp
     */
    inline TimestampRetrieveMethod timestampRetrieveMethod() { return timestampMethod_; }

    /**
     * @brief Set the IMU device name. Should be set before `init()`
     *
     * @param device IMU device name
     */
    void setDevice(const std::string& device);

    /**
     * @brief Initialize IMU device
     */
    void init() override;

    /**
     * @brief Set IMU frequency. NOTE should be set after `init()`.
     * @param freq IMU frequency, from 10 to 100Hz with step 10.
     */
    void setFrequency(unsigned short freq = 100);

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
     * @param raw       Raw buffer received from serial port
     * @return True if no error occurs and parse finish, false for something wrong
     */
    bool parseImuData(core::ImuRecord& record, const std::vector<unsigned char>& raw);

    /**
     * @brief Close device
     */
    void closeDevice();

  private:
    std::string device_;                       // device name
    unsigned short freq_;                      // frequency, Hz
    TimestampRetrieveMethod timestampMethod_;  // timestamp retrieve method

    boost::asio::io_service io_;     // IO context
    boost::asio::serial_port port_;  // IMU serial port
};

}  // namespace io
}  // namespace libra
