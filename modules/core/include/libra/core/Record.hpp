#pragma once
#include <opencv2/core.hpp>
#include <vector>
#include "ImuReading.h"
#include "RawImageReading.hpp"

namespace libra {
namespace core {

/**
 * @brief Sensor record, include timestamp and corresponding sensor reading.
 * @tparam T    Sensor reading
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
    // some getter
    inline const double& timestamp() const { return timestamp_; }
    inline const T& reading() const { return reading_; }
    inline T& reading() { return reading_; }

    /**
     * @brief Set timestamp
     * @param timestamp Timestamp
     */
    void setTimestamp(const double& timestamp) { timestamp_ = timestamp; }

    /**
     * @brief Print record to output stream
     * @param os        Output stream
     * @param record    Sensor record
     * @return Output stream
     */
    friend std::ostream& operator<<(std::ostream& os, const Record<T>& record) {
        os << fmt::format("t = {:.5f} s, ", record.timestamp_) << record.reading_;
        return os;
    }

  private:
    double timestamp_;  // timestamp, [s]
    T reading_;         // sensor reading
};

/**
 * @brief Convert record to json
 */
template <typename T>
void to_json(nlohmann::json& j, const Record<T>& record) {
    j = nlohmann::json{{"TimeStamp", record.timestamp()}, {"Reading", record.reading()}};
}

/**
 * @brief Convert json to record
 */
template <typename T>
void from_json(const nlohmann::json& j, Record<T>& record) {
    auto timestamp = j["TimeStamp"].get<double>();
    T reading = j["Reading"].get<T>();
    record.setTimestamp(timestamp);
    record.reading() = reading;
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
