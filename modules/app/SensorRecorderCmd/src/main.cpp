#include <fmt/format.h>
#include <glog/logging.h>
#include <boost/filesystem.hpp>
#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include "libra/io.hpp"

using namespace std;
using namespace fmt;
namespace fs = boost::filesystem;
using namespace libra::core;
using namespace libra::io;
using namespace libra::util;

/**
 * @brief Image save format
 */
enum class ImageSaveFormat {
    Kalibr,  // kalibr format, <timestamp(ns)>
    Index,   // index
};

int main(int argc, const char* argv[]) {
    cout << Title("Sensor Recorder without GUI");
    string saveRootFolder{"./data01"};
    ImageSaveFormat saveFormat_ = ImageSaveFormat::Kalibr;

    // get device
    cout << Section("Get MYNT-EYE Device");
    auto devices = MyntEyeRecorder::getDevices();
    if (devices.empty()) {
        return 0;
    }

    // set and init
    cout << Section("Start Camera");
    auto recorder = make_shared<MyntEyeRecorder>(devices.front().first);
    recorder->setFrameRate(30);
    recorder->setStreamMode(mynteyed::StreamMode::STREAM_1280x720);
    recorder->setSaverThreadNum(2);
    recorder->setTimeStampRetrieveMethod(TimestampRetrieveMethod::Sensor);
    // init
    recorder->init();

    // create save folder
    fs::path rootPath = fs::weakly_canonical(saveRootFolder);
    cout << format("root path: {}", rootPath.string()) << endl;
    // create directories if not exist
    if (!fs::exists(rootPath)) {
        fs::create_directories(rootPath);
    }
    auto leftImageSavePath = fs::weakly_canonical(rootPath / "left");
    auto rightImageSavePath = fs::weakly_canonical(rootPath / "right");
    auto imuSavePath = rootPath / "imu.csv";
    cout << format("left image path: {}", leftImageSavePath.string()) << endl;
    cout << format("right image path: {}", rightImageSavePath.string()) << endl;
    cout << format("IMU path: {}", imuSavePath.string()) << endl;

    // remove old files
    fs::remove_all(leftImageSavePath);
    if (!fs::create_directories(leftImageSavePath)) {
        LOG(ERROR) << format("cannot create folder \"{}\" to save left image", leftImageSavePath.string());
    }

    // create right save folder
    if (recorder->isRightCamEnabled()) {
        // remove old jpg files in right save folder
        fs::remove_all(rightImageSavePath);
        if (!fs::create_directories(rightImageSavePath)) {
            LOG(ERROR) << format("cannot create folder \"{}\" to save right image", rightImageSavePath.string());
        }
    }

    // IMU file stream
    fstream imuFileStream;

    // some index
    size_t leftImageIndex{0};
    size_t rightImageIndex{0};

    // set callback function
    // create save folder before run
    recorder->addCallback(MyntEyeRecorder::CallBackStarted, [&]() {
        // open IMU file
        imuFileStream.open(imuSavePath.string(), ios::out);
        CHECK(imuFileStream.is_open()) << format("cannot open file \"{}\" to save IMU data", imuSavePath.string());
        // write header
        imuFileStream << "# timestamp(ns), gyro X(rad/s), gyro Y(rad/s), gyro Z(rad/s), acc X(m/s^2), acc "
                         "Y(m/s^2), acc Z(m/s^2)"
                      << endl;
    });

    // close file when finished
    recorder->addCallback(MyntEyeRecorder::CallBackFinished, [&] { imuFileStream.close(); });

    // set process function for left camera
    recorder->setProcessFunction([&](const RawImageRecord& raw) {
        // save file name
        string fileName;
        switch (saveFormat_) {
            case ImageSaveFormat::Kalibr:
                fileName = format("{}/{:.0f}.jpg", leftImageSavePath.string(), raw.timestamp() * 1E9);
                break;
            case ImageSaveFormat::Index: {
                fileName = format("{}/{:06d}.jpg", leftImageSavePath.string(), leftImageIndex);
            }
            default:
                break;
        }
        // save to file
        fstream fs(fileName, ios::out | ios::binary);
        if (!fs.is_open()) {
            LOG(ERROR) << format("cannot create file \"{}\"", fileName);
        }
        fs.write(reinterpret_cast<const char*>(raw.reading().buffer()), raw.reading().size());
        fs.close();

        // send image per 10 images
        if (0 == leftImageIndex % 10) {
            cv::Mat buf(1, raw.reading().size(), CV_8UC1, (void*)raw.reading().buffer());
            cv::Mat img = cv::imdecode(buf.clone(), cv::IMREAD_UNCHANGED);
            cv::imshow("Left Image", img);
            cv::waitKey(1);
        }

        ++leftImageIndex;
    });

    // set process function for right camera
    if (recorder->isRightCamEnabled()) {
        // set process function, for right camera
        recorder->setRightProcessFunction([&](const RawImageRecord& raw) {
            // save file name
            string fileName;
            switch (saveFormat_) {
                case ImageSaveFormat::Kalibr:
                    fileName = format("{}/{:.0f}.jpg", rightImageSavePath.string(), raw.timestamp() * 1.0E9);
                    break;
                case ImageSaveFormat::Index: {
                    fileName = format("{}/{:06d}.jpg", rightImageSavePath.string(), rightImageIndex);
                }
                default:
                    break;
            }
            // save to file
            fstream fs(fileName, ios::out | ios::binary);
            if (!fs.is_open()) {
                LOG(ERROR) << format("cannot create file \"{}\"", fileName);
            }
            fs.write(reinterpret_cast<const char*>(raw.reading().buffer()), raw.reading().size());
            fs.close();

            // send image per 10 images
            if (0 == rightImageIndex % 10) {
                cv::Mat buf(1, raw.reading().size(), CV_8UC1, (void*)raw.reading().buffer());
                cv::imshow("Right Image", cv::imdecode(buf, cv::IMREAD_UNCHANGED));
                cv::waitKey(1);
            }

            ++rightImageIndex;
        });
    }

    // set process funcion for IMU
    recorder->setProcessFunction([&](const ImuRecord& imu) {
        // format: timestamp(ns), gyro(rad/s), acc(m/s^2)
        imuFileStream << format("{:.0f},{:.10f},{:.10f},{:.10f},{:.10f},{:.10f},{:.10f}", imu.timestamp() * 1.0E9,
                                imu.reading().gyro()[0], imu.reading().gyro()[1], imu.reading().gyro()[2],
                                imu.reading().acc()[0], imu.reading().acc()[1], imu.reading().acc()[2])
                      << endl;
    });

    recorder->start();

    while (!recorder->isStop()) {
        sleep(10);
    }
    // recorder->stop();

    return 0;
}