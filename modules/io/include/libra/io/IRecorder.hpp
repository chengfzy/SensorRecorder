#pragma once
#include <functional>
#include "libra/core/Record.hpp"
#include "libra/util.hpp"

namespace libra {
namespace io {

/**
 * @brief Interface for sensor data recorder.
 *
 *  Every recorder are a thread, which should implement the `protected void run()` function.
 */
class IRecorder : public libra::util::Thread {
  public:
    explicit IRecorder() = default;
    ~IRecorder() override = default;

  public:
    /**
     * @brief Initialize sensor
     */
    virtual void init() = 0;

    /**
     * @brief Set process function for image record
     * @param func Process function for image record
     */
    void setProcessFunction(const std::function<void(const core::ImageRecord&)>& func) { processImg_ = func; }

    /**
     * @brief Set process function for raw image record
     * @param func Process function for raw image record
     */
    void setProcessFunction(const std::function<void(const core::RawImageRecord&)>& func) { processRawImg_ = func; }

    /**
     * @brief Set process function for IMU record
     * @param func Process function for IMU record
     */
    void setProcessFunction(const std::function<void(const core::ImuRecord&)>& func) { processImu_ = func; }

  protected:
    std::function<void(const core::ImageRecord&)> processImg_;        //!< image record process function
    std::function<void(const core::RawImageRecord&)> processRawImg_;  //!< raw image record process function
    std::function<void(const core::ImuRecord&)> processImu_;          //!< IMU record process function
};

}  // namespace io
}  // namespace libra