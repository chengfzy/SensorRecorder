#include "libra/io/SanChiImuRecorder.h"
#include <fmt/format.h>
#include <glog/logging.h>
#include <chrono>
#include <iomanip>
#include <iostream>

using namespace std;
using namespace std::chrono;
using namespace Eigen;
using namespace boost::asio;
using namespace libra::util;
using namespace libra::core;
using namespace libra::io;

// Constructor with command, which used for the command without any other input
SanChiImuRecorder::CommandMessage::CommandMessage(SanChiImuRecorder::CommandMessage::Command cmd) {
    // parse command to setting length and message
    switch (cmd) {
        case Command::Start:
            len_ = 4;
            data_ = {0x01};
            break;
        case Command::Stop:
            len_ = 4;
            data_ = {0x02};
            break;
        default:
            LOG(FATAL) << "the constructor `explicit CommandMessage(Command cmd)` don't support the input command";
    }
}

// Constructor with command and data, which used for the command with data input.
SanChiImuRecorder::CommandMessage::CommandMessage(SanChiImuRecorder::CommandMessage::Command cmd,
                                                  unsigned short value) {
    // parse command and value to setting length and message
    switch (cmd) {
        case Command::SetFreq:
            len_ = 5;
            CHECK(value % 10 == 0 && 1 <= value / 10 && value / 10 <= 10)
                << "the input frequency value should be from 10 to 100";
            data_ = {0xa8, static_cast<unsigned char>(value)};
            break;
        default:
            LOG(FATAL) << "the constructor `explicit CommandMessage(Command cmd, unsigned short value)` don't support "
                          "the input command";
    }
}

// Get message of this command
vector<unsigned char> SanChiImuRecorder::CommandMessage::message() const {
    // calculate the check sum
    unsigned char sum{len_};
    for (auto& v : data_) {
        sum += v;
    }

    // message: [header(A55A), length, data, check sum, end(AA)]
    vector<unsigned char> message = {0xA5, 0x5A, len_};
    message.insert(message.end(), data_.begin(), data_.end());
    message.emplace_back(sum);
    message.emplace_back(0xAA);

    return message;
}

// Constructor with IMU device and output frequency
SanChiImuRecorder::SanChiImuRecorder(const string& device, unsigned short freq)
    : device_(device), timestampMethod_(TimestampRetrieveMethod::Sensor), port_(io_) {
    // check frequency value
    if (freq % 10 == 0 && 1 <= freq / 10 && freq / 10 <= 10) {
        freq_ = freq;
    } else {
        LOG(ERROR) << "the input frequency value should be from 10 to 100";
    }
}

// Default destructor, stop and wait thread finish
SanChiImuRecorder::~SanChiImuRecorder() {
    // wait process finish
    if (isStart()) {
        stop();
        wait();
    }

    // close device
    closeDevice();
}

// Set the IMU device name
void SanChiImuRecorder::setDevice(const string& device) { device_ = device; }

// Initialize IMU device
void SanChiImuRecorder::init() {
    LOG(INFO) << Section("SanChi IMU Init", false);

    // open serial port
    port_.open(device_);
    CHECK(port_.is_open()) << fmt::format("cannot open IMU device {}", device_);
    // set
    port_.set_option(serial_port::baud_rate(115200));
    port_.set_option(serial_port::character_size(8));
    port_.set_option(serial_port::parity(serial_port::parity::type::none));
    port_.set_option(serial_port::stop_bits(serial_port::stop_bits::type::one));
}

// Set IMU frequency, note the setting only could be set after call `init()`
void SanChiImuRecorder::setFrequency(unsigned short freq) {
    if (freq % 10 != 0 || 1 > freq / 10 || freq / 10 > 10) {
        LOG(ERROR) << "the input frequency value should be from 10 to 100";
        return;
    }

    freq_ = freq;
    LOG(INFO) << fmt::format("set frequency = {} Hz", freq_);
    port_.write_some(buffer(CommandMessage(CommandMessage::Command::SetFreq, freq_).message()));
}

// Set the method to retrieve timestamp
void SanChiImuRecorder::setTimeStampRetrieveMethod(TimestampRetrieveMethod method) { timestampMethod_ = method; }

// The whole run function
void SanChiImuRecorder::run() {
    LOG(INFO) << Section("SanChi IMU Recording", false);

    // send START command
    port_.write_some(buffer(CommandMessage(CommandMessage::Command::Start).message()));

    while (true) {
        if (isStop()) {
            // send STOP command
            port_.write_some(buffer(CommandMessage(CommandMessage::Command::Stop).message()));
            // close device
            closeDevice();

            break;
        }

        // read data from IMU and parse
        ImuRecord record;
        vector<unsigned char> raw(40);
        read(port_, buffer(raw), transfer_exactly(40));
        if (parseImuData(record, raw)) {
            processImu_(record);
        }
    }
}

// Parse IMU data and set to ImuRecord
bool SanChiImuRecorder::parseImuData(ImuRecord& record, const vector<unsigned char>& raw) {
    // set timestamp
    if (timestampMethod_ == TimestampRetrieveMethod::Host) {
        auto now = duration_cast<nanoseconds>(system_clock::now().time_since_epoch());
        record.setTimestamp(now.count() * 1.0E-9);
    }

    // // check data size
    // if (40 != raw.size()) {
    //     LOG(ERROR) << fmt::format("received data size({}) don't match to the desired value(40)", raw.size());
    //     return false;
    // }

    // check header: [0,1]
    if (0xA5 != raw[0] || 0x5A != raw[1]) {
        LOG(ERROR) << fmt::format("data header(0x{:02X}{:02X}) is broken, should be 0xA55A", raw[0], raw[1]);
#if defined(DebugTest)
        // print raw data
        cout << "raw data: ";
        for (auto& v : raw) {
            cout << hex << uppercase << setw(2) << setfill('0') << static_cast<unsigned short>(v) << " ";
        }
        cout << endl;
#endif

        // If A55A is in other index, this check will fail. So I find the A55A index, read the remained data and drop it
        // TODO: there will exist another case: 5A ... A5
        for (size_t i = 0; i < raw.size(); ++i) {
            if (0xA5 == raw[i] && i + 1 < raw.size() && 0x5A == raw[i + 1]) {
                // read the remain data
                vector<unsigned char> remainData(i);
                read(port_, buffer(remainData), transfer_exactly(i));
#if defined(DebugTest)
                // print raw data
                cout << "remain data: ";
                for (auto& v : remainData) {
                    cout << hex << uppercase << setw(2) << setfill('0') << static_cast<unsigned short>(v) << " ";
                }
                cout << endl;
#endif

                break;
            }
        }

        return false;
    }

    // check length, [2]
    if (37 != raw[2]) {
        LOG(ERROR) << fmt::format("data length({}) don't match to the desired value(37)", raw[2]);
        return false;
    }

    // check checksum, 38, last 3rd
    unsigned char sum = accumulate(raw.begin() + 2, raw.end() - 2, 0);
    if (sum != *(raw.end() - 2)) {
        LOG(ERROR) << fmt::format("data check sum don't match, 0x{:02X} != 0x{:02X}", sum, *(raw.end() - 2));
        return false;
    }

    // check end, 39, last 2rd
    if (0xAA != *(raw.end() - 1)) {
        LOG(ERROR) << fmt::format("data end(0x{:02X}) don't match to the desired value(0XAA)", *(raw.end() - 1));
        return false;
    }

    // parse data
    // yaw, pitch, roll, [3-8], unit: 0.1 deg, PASS

    // acc in X, Y, Z, [9-14], unit: *16384 g
    record.reading().setAcc(Vector3d(static_cast<short>((raw[9] << 8) + raw[10]) / 16384.,
                                     static_cast<short>((raw[11] << 8) + raw[12]) / 16384.,
                                     static_cast<short>((raw[13] << 8) + raw[14]) / 16384.) *
                            Constant::kG);

    // gyro in X, Y, Z, [15-20], unit: *32.8 deg/s
    static const double kGyroCoeff = 1. / 32.8 / 180. * M_PI;
    record.reading().setGyro(Vector3d(static_cast<short>((raw[15] << 8) + raw[16]) * kGyroCoeff,
                                      static_cast<short>((raw[17] << 8) + raw[18]) * kGyroCoeff,
                                      static_cast<short>((raw[19] << 8) + raw[20]) * kGyroCoeff));

    // timestamp since power on, [33-36], unit: ms
    if (timestampMethod_ == TimestampRetrieveMethod::Sensor) {
        record.setTimestamp(((raw[33] << 24) + (raw[34] << 16) + (raw[35] << 8) + raw[36]) * 1.0e-3);
    }

    // magnet in X, Y, Z, [21-26], unit: *390 Gs, PASS
    // air pressure height_ in X, Y, Z, [27-30], unit: 0.01 m, PASS
    // temperature, [31-32], unit: 0.01 C, PASS
    // magnet state flag, 37, PASS

    return true;
}

//  Close serial port of IMU
void SanChiImuRecorder::closeDevice() { port_.close(); }