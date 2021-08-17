#include "libra/core/ImuReading.h"
#include <fmt/format.h>

using namespace std;
using namespace fmt;
using namespace Eigen;
using namespace libra::core;

// Construct IMU reading with accelerometer and gyroscope reading value
ImuReading::ImuReading(const Vector3d& acc, const Vector3d& gyro) : gyro_(gyro), acc_(acc) {}

// Construct IMU reading with accelerometer and gyroscope reading value using move syntax
ImuReading::ImuReading(Vector3d&& acc, Vector3d&& gyro) : gyro_(move(gyro)), acc_(move(acc)) {}

// Set accelerometer reading
void ImuReading::setAcc(const Vector3d& acc) { acc_ = acc; }

// Set gyroscope reading
void ImuReading::setGyro(const Vector3d& gyro) { gyro_ = gyro; }

// Set accelerometer reading using move syntax
void ImuReading::setAcc(Vector3d&& acc) { acc_ = move(acc); }

// Set gyroscope reading using move syntax
void ImuReading::setGyro(Vector3d&& gyro) { gyro_ = move(gyro); }

namespace libra {
namespace core {

// Print IMU reading to output stream
ostream& operator<<(ostream& os, const ImuReading& imuReading) {
    os << format("acc = [{:.5f}, {:.5f}, {:.5f}] m/s^2, gyro = [{:.5f}, {:.5f}, {:.5f}] rad/s", imuReading.acc_[0],
                 imuReading.acc_[1], imuReading.acc_[2], imuReading.gyro_[0], imuReading.gyro_[1], imuReading.gyro_[2]);
    return os;
}

// Convert IMU reading to json
void to_json(nlohmann::json& j, const ImuReading& imu) {
    j = nlohmann::json{{"Acc", {imu.acc()[0], imu.acc()[1], imu.acc()[2]}},
                       {"Gyro", {imu.gyro()[0], imu.gyro()[1], imu.gyro()[2]}}};
}

// Convert json to IMU reading
void from_json(const nlohmann::json& j, ImuReading& imu) {
    vector<double> acc = j["Acc"].get<vector<double>>();
    vector<double> gyro = j["Gyro"].get<vector<double>>();
    imu.setAcc(Map<Vector3d>(acc.data()));
    imu.setGyro(Map<Vector3d>(gyro.data()));
}

}  // namespace core
}  // namespace libra
