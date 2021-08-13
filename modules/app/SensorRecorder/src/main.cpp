#include <fmt/format.h>
#include <fmt/ranges.h>
#include <glog/logging.h>
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
namespace fs = boost::filesystem;

/**
 * @brief Image save format
 */
enum class ImageSaveFormat {
    Kalibr,  // kalibr format, <timestamp(ns)>
    Index,   // index
};

int main(int argc, char* argv[]) {
    // argument parser
    cxxopts::Options options(argv[0], "Sensor Recorder without GUI");
    // clang-format off
    options.add_options()("f,folder", "save folder", cxxopts::value<string>()->default_value("./data"))
        ("frameRate", "frame rate", cxxopts::value<int>()->default_value("30"))
        ("streamMode", "stream mode", cxxopts::value<string>()->default_value("1280x720"))
        ("streamFormat", "stream format", cxxopts::value<string>()->default_value("MJPG"))
        ("saverThreadNum", "thread number to save images for each camera", cxxopts::value<int>()->default_value("2"))
        ("onlyLeft", "only process left camera", cxxopts::value<bool>())
        ("showImage", "show image or not", cxxopts::value<bool>())
        ("h,help", "help message");
    // clang-format on
    auto result = options.parse(argc, argv);
    if (result.count("help")) {
        cout << options.help() << endl;
        return 0;
    }
    string saveRootFolder = result["folder"].as<string>();
    int frameRate = result["frameRate"].as<int>();
    string streamModeName = result["streamMode"].as<string>();
    string streamFormatName = result["streamFormat"].as<string>();
    int saverThreadNum = result["saverThreadNum"].as<int>();
    // this option only used for RP4+YUYV, which cannot open only left camera
    bool onlyLeft = result["onlyLeft"].as<bool>();
    bool showImage = result["showImage"].as<bool>();

    // check stream mode
    vector<string> streamModeNames = {"2560x720", "1280x720", "1280x480", "640x480"};
    if (find_if(streamModeNames.begin(), streamModeNames.end(),
                [&](const string& s) { return boost::iequals(s, streamModeName); }) == streamModeNames.end()) {
        cout << format("input detector type should be one item in {}", streamModeNames) << endl << endl;
        cout << options.help() << endl;
        return 0;
    }
    // check stream format
    vector<string> streamFormatNames = {"YUYV", "MJPG"};
    if (find_if(streamFormatNames.begin(), streamFormatNames.end(),
                [&](const string& s) { return boost::iequals(s, streamFormatName); }) == streamFormatNames.end()) {
        cout << format("input stream format should be one item in {}", streamFormatNames) << endl << endl;
        cout << options.help() << endl;
        return 0;
    }

    cout << Title("Sensor Recorder without GUI");
    cout << format("save folder: {}", saveRootFolder) << endl;
    cout << format("frame rate = {} Hz", frameRate) << endl;
    cout << format("stream mode: {}", streamModeName) << endl;
    cout << format("stream format: {}", streamFormatName) << endl;
    cout << format("saver thread number = {}", saverThreadNum) << endl;
    cout << format("only process left camera = {}", onlyLeft) << endl;
    cout << format("show image = {}", showImage) << endl;
    ImageSaveFormat saveFormat = ImageSaveFormat::Kalibr;  // save format

    // init glog
    google::InitGoogleLogging(argv[0]);
    FLAGS_alsologtostderr = true;
    FLAGS_colorlogtostderr = true;

    // get device
    cout << Section("Get MYNT-EYE Device");
    auto devices = MyntEyeRecorder::getDevices();
    if (devices.empty()) {
        return 0;
    }

    // get stream mode from string
    mynteyed::StreamMode streamMode;
    if (boost::iequals(streamModeName, "2560x720")) {
        streamMode = mynteyed::StreamMode::STREAM_2560x720;
    } else if (boost::iequals(streamModeName, "1280x720")) {
        streamMode = mynteyed::StreamMode::STREAM_1280x720;
    } else if (boost::iequals(streamModeName, "1280x480")) {
        streamMode = mynteyed::StreamMode::STREAM_1280x480;
    } else if (boost::iequals(streamModeName, "640x480")) {
        streamMode = mynteyed::StreamMode::STREAM_640x480;
    }
    // get stream format from string
    mynteyed::StreamFormat streamFormat;
    if (boost::iequals(streamFormatName, "YUYV")) {
        streamFormat = mynteyed::StreamFormat::STREAM_YUYV;
    } else if (boost::iequals(streamFormatName, "MJPG")) {
        streamFormat = mynteyed::StreamFormat::STREAM_MJPG;
    }

    // set and init
    cout << Section("Start Camera");
    auto recorder = make_shared<MyntEyeRecorder>(devices.front().first);
    recorder->setFrameRate(frameRate);
    recorder->setStreamMode(streamMode);
    recorder->setStreamFormat(streamFormat);
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
    if (!onlyLeft && recorder->isRightCamEnabled()) {
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

#if true
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
    if (!onlyLeft && recorder->isRightCamEnabled()) {
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
        // format: timestamp(ns), gyro(rad/s), acc(m/s^2)
        imuFileStream << format("{:.0f},{:.10f},{:.10f},{:.10f},{:.10f},{:.10f},{:.10f}", imu.timestamp() * 1.0E9,
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
            if (!onlyLeft && recorder->isRightCamEnabled()) {
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