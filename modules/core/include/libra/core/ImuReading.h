#pragma once
#include <Eigen/Core>
#include <nlohmann/json.hpp>
#include <ostream>

namespace libra {
namespace core {

/**
 * @brief IMU reading, include an acceleration reading and a gyroscope(angular velocity) reading.
 */
class ImuReading {
  public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    /**
     * @brief Construct IMU reading with accelerometer and gyroscope reading value
     * @param acc   Accelerometer reading, unit: m/s^2
     * @param gyro  Gyroscope reading, unit: rad/s
     */
    explicit ImuReading(const Eigen::Vector3d& acc = Eigen::Vector3d::Zero(),
                        const Eigen::Vector3d& gyro = Eigen::Vector3d::Zero());

    /**
     * @brief Construct IMU reading with accelerometer and gyroscope reading value using move syntax
     * @param acc   Accelerometer reading, unit: m/s^2
     * @param gyro  Gyroscope reading, unit: rad/s
     */
    ImuReading(Eigen::Vector3d&& acc, Eigen::Vector3d&& gyro);

    /**
     * @brief Default destructor
     */
    ~ImuReading() = default;

  public:
    /**
     * @brief Get accelerometer reading
     * @return Accelerometer reading
     */
    inline const Eigen::Vector3d& acc() const { return acc_; }

    /**
     * @brief Get gyroscope reading
     * @return Gyroscope reading
     */
    inline const Eigen::Vector3d& gyro() const { return gyro_; }

    /**
     * @brief Set accelerometer reading
     * @param acc Accelerometer reading
     */
    void setAcc(const Eigen::Vector3d& acc);

    /**
     * @brief Set gyroscope reading
     * @param gyro Gyroscope reading
     */
    void setGyro(const Eigen::Vector3d& gyro);

    /**
     * @brief Set accelerometer reading using move syntax
     * @param acc Accelerometer reading
     */
    void setAcc(Eigen::Vector3d&& acc);

    /**
     * @brief Set gyroscope reading using move syntax
     * @param gyro Gyroscope reading
     */
    void setGyro(Eigen::Vector3d&& gyro);

    /**
     * @brief Print IMU reading to output stream
     * @param os            Output stream
     * @param imuReading    IMU reading
     * @return Output stream
     */
    friend std::ostream& operator<<(std::ostream& os, const ImuReading& imuReading);

  private:
    Eigen::Vector3d acc_;   // accelerometer reading, [m/s^2]
    Eigen::Vector3d gyro_;  // gyroscope reading, angular velocity, [rad/s]
};

/**
 * @brief Convert IMU reading to json
 * @param j     Json
 * @param imu   IMU reading
 */
void to_json(nlohmann::json& j, const ImuReading& imu);

/**
 * @brief Convert json to IMU reading
 * @param j     Json
 * @param imu   IMU reading
 */
void from_json(const nlohmann::json& j, ImuReading& imu);

}  // namespace core
}  // namespace libra
