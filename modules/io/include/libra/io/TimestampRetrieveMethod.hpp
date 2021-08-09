#pragma once

namespace libra {
namespace io {

/**
 * @brief Timestamp retrieve method for sensor recorder
 */
enum class TimestampRetrieveMethod {
    Sensor,  //!< retrieved from sensor
    Host,    //!< retrieved from host(PC)
};

}  // namespace io
}  // namespace libra