#pragma once
#include <opencv2/core.hpp>
#include <optional>
#include <vector>
#include "ImuReading.h"
#include "RawImageReading.hpp"

namespace libra {
namespace core {

/**
 * @brief Sensor record, include timestamp, corresponding sensor reading, and an optional system timestamp
 * @tparam T  Sensor reading
 *
 * @note The class also include an optional member: system timestamp, which is useful if sensor has two timestamp. For
 * example, the IMU has its own clock which could obtain the sensor timestamp, and when IMU connect to a computer, the
 * computer will also has its clock. Both timestamp are important, and could used for sensor time alignment.
 *
 * So if sensor has only one timestamp, then set it to "timestamp", if sensor has two timestamp, set the timestamp from
 * sensor to "timestamp", set the timestamp for computer(host) to "system timestamp".
 *
 */
template <typename T>
class Record {
  public:
    /**
     * @brief Construct with timestamp and sensor reading
     * @param timestamp Timestamp
     * @param reading   Sensor reading
     */
    explicit Record(const double& timestamp = 0, const T& reading = T()) : timestamp_(timestamp), reading_(reading) {}

    /**
     * @brief Construct with timestamp and sensor reading using move syntax
     * @param timestamp Timestamp
     * @param reading   Sensor reading
     */
    explicit Record(double&& timestamp, T&& reading) : timestamp_(std::move(timestamp)), reading_(std::move(reading)) {}

    /**
     * @brief Default destructor
     */
    ~Record() = default;

  public:
    // getter
    inline const double& timestamp() const { return timestamp_; }
    inline const std::optional<double>& systemTimestamp() const { return systemTimestamp; }
    inline const T& reading() const { return reading_; }

    /**
     * @brief Get the sensor reading without const quaifer
     * @return  Sensor reading without const quaifer
     *
     * @note This method is very useful for sensor reading which contains the raw data pointer and its corresponding
     * size, for example, raw image reading. Some operations maybe be conduct in the raw data
     */
    inline T& reading() { return reading_; }

    /**
     * @brief Set timestamp
     * @param timestamp Timestamp, [s]
     */
    void setTimestamp(const double& timestamp) { timestamp_ = timestamp; }

    /**
     * @brief Set system timestamp
     * @param timestamp System timestamp, [s]
     */
    void setSystemTimestamp(const double& timestamp) { systemTimestamp_ = timestamp; }

    /**
     * @brief Set sensor reading
     * @param reading Sensor reading
     */
    void setReading(const T& reading) { reading_ = reading_; }

    /**
     * @brief Set timestamp using move syntax
     * @param timestamp Timestamp, [s]
     */
    void setTimestamp(double&& timestamp) { timestamp_ = std::move(timestamp); }

    /**
     * @brief Set system timestamp using move syntax
     * @param timestamp System timestamp, [s]
     */
    void setSystemTimestamp(double&& timestamp) { systemTimestamp_ = std::move(timestamp); }

    /**
     * @brief Set sensor reading using move syntax
     * @param reading Sensor reading
     */
    void setReading(T&& reading) { reading_ = move(reading_); }

    /**
     * @brief Print record to output stream
     * @param os        Output stream
     * @param record    Sensor record
     * @return Output stream
     */
    friend std::ostream& operator<<(std::ostream& os, const Record<T>& record) {
        os << fmt::format("t = {:.5f} s, ", record.timestamp_);
        if (record.systemTimestamp_) {
            os << fmt::format("system t = {:.5f} s", record.systemTimestamp_);
        }
        os << record.reading_;
        return os;
    }

  private:
    double timestamp_;                       // sensor timestamp, [s]
    std::optional<double> systemTimestamp_;  // system timestamp, [s]
    T reading_;                              // sensor reading
};

/**
 * @brief Convert record to json
 */
template <typename T>
void to_json(nlohmann::json& j, const Record<T>& record) {
    j = nlohmann::json{{"Timestamp", record.timestamp()}, {"Reading", record.reading()}};
    if (record.systemTimestamp()) {
        j["SystemTimestamp"] = record.systemTimestamp().value();
    }
}

/**
 * @brief Convert json to record
 */
template <typename T>
void from_json(const nlohmann::json& j, Record<T>& record) {
    record.setTimestamp(j["Timestamp"].get<double>());
    record.setReading(j["Reading"].get<T>());
    if (j.contains("SystemTimestamp")) {
        record.setSystemTimestamp(j["SystemTimestamp"].get<double>());
    }
}

/**************************************** Type Definition ****************************************/
using ImuRecord = Record<ImuReading>;
using ImageRecord = Record<::cv::Mat>;
using RawImageRecord = Record<RawImageReading>;

using ImuData = std::vector<ImuRecord>;
using ImageData = std::vector<ImageRecord>;
using RawImageData = std::vector<RawImageRecord>;

}  // namespace core
}  // namespace libra
