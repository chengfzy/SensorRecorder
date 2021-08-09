#include "libra/io/SparkFunImuRecorder.h"
#include <fmt/format.h>
#include <glog/logging.h>
#include <boost/algorithm/string.hpp>
#include <chrono>

using namespace std;
using namespace std::chrono;
using namespace Eigen;
using namespace boost;
using namespace boost::asio;
using namespace libra::util;
using namespace libra::core;
using namespace libra::io;

// Constructor with IMU device
SparkFunImuRecorder::SparkFunImuRecorder(const string& device)
    : device_(device), timestampMethod_(TimestampRetrieveMethod::Sensor), port_(io_) {}

// Default destructor, stop and wait thread finish
SparkFunImuRecorder::~SparkFunImuRecorder() {
    // wait process finish
    if (isStart()) {
        stop();
        wait();
    }

    // close device
    closeDevice();
}

// Set the IMU device name
void SparkFunImuRecorder::setDevice(const string& device) { device_ = device; }

// Set the method to retrieve timestamp
void SparkFunImuRecorder::setTimeStampRetrieveMethod(TimestampRetrieveMethod method) { timestampMethod_ = method; }

// Initialize IMU device
void SparkFunImuRecorder::init() {
    LOG(INFO) << Section("SparkFun Init", false);

    // open serial port
    port_.open(device_);
    CHECK(port_.is_open()) << fmt::format("cannot open IMU device {}", device_);
    // set
    port_.set_option(serial_port::baud_rate(9600));
    port_.set_option(serial_port::character_size(8));
    port_.set_option(serial_port::parity(serial_port::parity::type::none));
    port_.set_option(serial_port::stop_bits(serial_port::stop_bits::type::one));
}

// The whole run function
void SparkFunImuRecorder::run() {
    LOG(INFO) << Section("SparkFun Recording", false);
    while (true) {
        if (isStop()) {
            // close device
            closeDevice();

            break;
        }

        // read data and parse
        ImuRecord record;
        size_t size = read_until(port_, portBuffer_, "\r\n");
        if (parseImuData(record, {buffers_begin(portBuffer_.data()), buffers_begin(portBuffer_.data()) + size})) {
            processImu_(record);
        }

        // taken parsed data from buffer
        portBuffer_.consume(size);
    }
}

// Parse IMU data and set to ImuRecord
bool SparkFunImuRecorder::parseImuData(libra::core::ImuRecord& record, const string& raw) {
    vector<string> tokens;
    split(tokens, raw, is_any_of(","));
    if (tokens.size() != 12) {
        LOG(ERROR) << fmt::format("received data size({}) not match to desired one(12)", tokens.size()) << ": " << raw;
        return false;
    }

    // set timestamp
    switch (timestampMethod_) {
        case TimestampRetrieveMethod::Sensor:
            record.setTimestamp(stod(tokens[1]) * 1.0E-3);
            break;
        case TimestampRetrieveMethod::Host:
            auto now = duration_cast<nanoseconds>(system_clock::now().time_since_epoch());
            record.setTimestamp(now.count() * 1.0E-9);
            break;
    }

    // set acc and gyro
    static const double kGyroCoeff = 1. / 180. * M_PI;
    record.reading().setAcc(Vector3d(stod(tokens[2]), stod(tokens[3]), stod(tokens[4])) * Constant::kG);
    record.reading().setGyro(Vector3d(stod(tokens[5]), stod(tokens[6]), stod(tokens[7])) * kGyroCoeff);
    return true;
}

//  Close serial port of IMU
void SparkFunImuRecorder::closeDevice() { port_.close(); }