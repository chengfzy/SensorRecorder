#include <fmt/format.h>
#include <fmt/ranges.h>
#include <glog/logging.h>
#include <sched.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <condition_variable>
#include <cxxopts.hpp>
#include <iostream>
#include <mutex>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include "libra/io.hpp"

using namespace std;
using namespace fmt;
using namespace libra::core;
using namespace libra::io;
using namespace libra::util;
using namespace sl_oc;
namespace fs = boost::filesystem;

/**
 * @brief Image save format
 */
enum class ImageSaveFormat {
    Kalibr,  // kalibr format, <timestamp(ns)>
    Index,   // index
};

int main(int argc, char* argv[]) {
    cout << Title("ZED Sensor Recorder using Open Source Library") << endl;
    // init glog
    google::InitGoogleLogging(argv[0]);
    FLAGS_alsologtostderr = true;
    FLAGS_colorlogtostderr = true;

    // argument parser
    cxxopts::Options options(argv[0], "ZED Sensor Recorder");
    // clang-format off
    options.add_options()
        ("f,folder", "save folder", cxxopts::value<string>()->default_value("./data/record"))
        ("fps", "FPS", cxxopts::value<int>()->default_value("30"))
        ("resolution", "resolution", cxxopts::value<string>()->default_value("HD720"))
        ("saverThreadNum", "thread number to save images for each camera", cxxopts::value<int>()->default_value("2"))
        ("showImage", "show image", cxxopts::value<bool>())
        ("h,help", "help message");
    // clang-format on
    auto result = options.parse(argc, argv);
    if (result.count("help")) {
        cout << options.help() << endl;
        return 0;
    }
    string saveRootFolder = result["folder"].as<string>();
    int fps = result["fps"].as<int>();
    string resolution = result["resolution"].as<string>();
    int saverThreadNum = result["saverThreadNum"].as<int>();
    bool showImage = result["showImage"].as<bool>();

    // check fps
    vector<int> fpsList = {15, 30, 60, 100};
    if (find_if(fpsList.begin(), fpsList.end(), [&](int v) { return fps == v; }) == fpsList.end()) {
        cout << fmt::format("input FPS should be one item in {}", fpsList) << endl;
        cout << options.help() << endl;
        return 0;
    }

    // check resultion
    vector<string> resolutions = {"HD2K", "HD1080", "HD720", "VGA"};
    if (find_if(resolutions.begin(), resolutions.end(),
                [&](const string& v) { return boost::iequals(v, resolution); }) == resolutions.end()) {
        cout << fmt::format("input resolution should be one item in {}", resolutions) << endl << endl;
        cout << options.help() << endl;
        return 0;
    }

    // print input parameters
    cout << Section("Input Parameters");
    cout << fmt::format("save folder: {}", saveRootFolder) << endl;
    cout << fmt::format("FPS = {} Hz", fps) << endl;
    cout << fmt::format("resolution = {}", resolution) << endl;
    cout << fmt::format("saver thread number = {}", saverThreadNum) << endl;
    cout << fmt::format("show image: {}", showImage) << endl;
    ImageSaveFormat saveFormat = ImageSaveFormat::Kalibr;  // save format

    // parse resolution
    video::RESOLUTION res;
    if (resolution == "HD2K") {
        res = video::RESOLUTION::HD2K;
    } else if (resolution == "HD1080") {
        res = video::RESOLUTION::HD1080;
    } else if (resolution == "HD720") {
        res = video::RESOLUTION::HD720;
    } else if (resolution == "VGA") {
        res = video::RESOLUTION::VGA;
    }

#if 0
    // set CPU affinity
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    if (sched_setaffinity(0, sizeof(mask), &mask) < 0) {
        LOG(ERROR) << "set CPU affinity failed";
    }
#endif

    // get device
    cout << Section("Get ZED Device");
    auto devices = ZedOpenRecorder::getDevices();
    if (devices.empty()) {
        return 0;
    }

    // set and init
    cout << Section("Start Camera");
    auto recorder = make_shared<ZedOpenRecorder>();
    recorder->setFps(static_cast<video::FPS>(fps));
    recorder->setResolution(res);
    recorder->setSaverThreadNum(saverThreadNum);

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

    // remove old files
    fs::remove_all(rootPath);

    // create left save folder
    cout << format("left image path: {}", leftImageSavePath.string()) << endl;
    if (!fs::create_directories(leftImageSavePath)) {
        LOG(ERROR) << format("cannot create folder \"{}\" to save left image", leftImageSavePath.string());
    }

    // create right save folder
    if (false && recorder->isRightCamEnabled()) {
        cout << format("right image path: {}", rightImageSavePath.string()) << endl;
        if (!fs::create_directories(rightImageSavePath)) {
            LOG(ERROR) << format("cannot create folder \"{}\" to save right image", rightImageSavePath.string());
        }
    }

    // IMU file stream
    cout << format("IMU path: {}", imuSavePath.string()) << endl;
    fstream imuFileStream;

    // some variables
    atomic_long leftImageIndex{0};  // image index
    atomic_long rightImageIndex{0};
    // mutex and condition variable indict whether to show this image
    bool showImageReady{false};  // flag to indict the image is ready
    mutex showImageMutex;
    condition_variable showImageCv;
    cv::Mat leftImage, rightImage;

    // set callback function
    recorder->addCallback(ZedOpenRecorder::CallBackStarted, [&]() {
        // open IMU file
        imuFileStream.open(imuSavePath.string(), ios::out);
        CHECK(imuFileStream.is_open()) << format("cannot open file \"{}\" to save IMU data", imuSavePath.string());
        // write header
        imuFileStream << "#SensorTimestamp[ns],SystemTimestamp[ns],GyroX[rad/s],GyroY[rad/s],GyroZ[rad/s]"
                         ",AccX[m/s^2],AccY[m/s^2],AccZ[m/s^2]"
                      << endl;
    });

    // close file when finished
    recorder->addCallback(ZedOpenRecorder::CallBackFinished, [&] { imuFileStream.close(); });

    // set process function for left camera
    recorder->setProcessFunction([&](const RawImageRecord& raw) {
        LOG_EVERY_N(INFO, 100) << fmt::format("process left image, index = {}, timestamp = {:.5f} s", leftImageIndex,
                                              raw.timestamp());

        // save file name
        string fileName;
        switch (saveFormat) {
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
            leftImage = cv::imdecode(buf, cv::IMREAD_UNCHANGED);

            unique_lock<mutex> lock(showImageMutex);
            showImageReady = true;
            showImageCv.notify_one();
        }

#if false
        // remove old files
        if (leftImageIndex != 0 && leftImageIndex % 200000 == 0) {
            LOG(WARNING) << fmt::format("remove old left images, index = {}", leftImageIndex);
            // remove old files
            fs::remove_all(leftImageSavePath);

            // create left save folder
            cout << format("left image path: {}", leftImageSavePath.string()) << endl;
            if (!fs::create_directories(leftImageSavePath)) {
                LOG(ERROR) << format("cannot create folder \"{}\" to save left image", leftImageSavePath.string());
            }
        }

#endif

        ++leftImageIndex;
    });

    // set process function for right camera
    if (false) {
        // set process function, for right camera
        recorder->setRightProcessFunction([&](const RawImageRecord& raw) {
            LOG_EVERY_N(INFO, 100) << fmt::format("process right image, index = {}, timestamp = {:.5f} s",
                                                  rightImageIndex, raw.timestamp());
            // save file name
            string fileName;
            switch (saveFormat) {
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
                rightImage = cv::imdecode(buf, cv::IMREAD_UNCHANGED);

                unique_lock<mutex> lock(showImageMutex);
                showImageReady = true;
                showImageCv.notify_one();
            }

            ++rightImageIndex;
        });
    }

    // set process funcion for IMU
    recorder->setProcessFunction([&](const ImuRecord& imu) {
        // format: sensor timestamp(ns), system timstamp(ns), gyro(rad/s), acc(m/s^2)
        imuFileStream << format("{:.0f},{:.0f},{:.10f},{:.10f},{:.10f},{:.10f},{:.10f},{:.10f}",
                                imu.timestamp() * 1.0E9, imu.systemTimestamp().value_or(0) * 1.0E9,
                                imu.reading().gyro()[0], imu.reading().gyro()[1], imu.reading().gyro()[2],
                                imu.reading().acc()[0], imu.reading().acc()[1], imu.reading().acc()[2])
                      << endl;
    });

    // start
    recorder->start();

    // wait stop
    while (true) {
        {
            unique_lock<mutex> lock(showImageMutex);
            showImageCv.wait(lock, [&]() { return showImageReady; });
            showImageReady = false;
        }

        // show image
        if (showImage) {
            cv::imshow("Left Image", leftImage);
            if (recorder->isRightCamEnabled()) {
                cv::imshow("Right Image", rightImage);
            }
            int ret = cv::waitKey(1);

            if (ret == 'Q' || ret == 'q') {
                recorder->stop();
                recorder->wait();
                break;
            }
        }
    }

    return 0;
}