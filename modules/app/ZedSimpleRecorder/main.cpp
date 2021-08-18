#include <fmt/format.h>
#include <fmt/ranges.h>
#include <glog/logging.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <cxxopts.hpp>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <zed-open-capture/sensorcapture.hpp>
#include <zed-open-capture/videocapture.hpp>
#include "libra/util.hpp"

using namespace std;
using namespace cv;
using namespace sl_oc;
using namespace libra::util;
namespace fs = boost::filesystem;

int main(int argc, char* argv[]) {
    cout << Title("ZED Simple Recorder using Open Source Library") << endl;
    // init glog
    google::InitGoogleLogging(argv[0]);
    FLAGS_alsologtostderr = true;
    FLAGS_colorlogtostderr = true;

    // argument parser
    cxxopts::Options options(argv[0], "ZED Simple Recorder");
    // clang-format off
    options.add_options()
        ("f,folder", "save folder", cxxopts::value<string>()->default_value("./data/record"))
        ("d,deviceId", "device index", cxxopts::value<int>()->default_value("-1"))
        ("fps", "FPS", cxxopts::value<int>()->default_value("30"))
        ("resolution", "resolution", cxxopts::value<string>()->default_value("HD720"))
        ("showImage", "show image", cxxopts::value<bool>())
        ("h,help", "help message");
    // clang-format on
    auto result = options.parse(argc, argv);
    if (result.count("help")) {
        cout << options.help() << endl;
        return 0;
    }
    string rootFolder = result["folder"].as<string>();
    int deviceId = result["deviceId"].as<int>();
    int fps = result["fps"].as<int>();
    string resolution = result["resolution"].as<string>();
    bool showImg = result["showImage"].as<bool>();

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
    cout << fmt::format("save folder: {}", rootFolder) << endl;
    cout << fmt::format("device ID = {}", deviceId) << endl;
    cout << fmt::format("FPS = {} Hz", fps) << endl;
    cout << fmt::format("resolution = {}", resolution) << endl;
    cout << fmt::format("show image: {}", showImg) << endl;

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

    // create video capture
    cout << Section("Open Camera(VideoCapture)");
    video::VideoParams params;
    params.res = res;
    params.fps = static_cast<video::FPS>(fps);
    params.verbose = 1;
    video::VideoCapture video(params);
    CHECK(video.initializeVideo(deviceId)) << fmt::format("cannot open camera {}", deviceId);
    LOG(INFO) << fmt::format("serial number: {}", video.getSerialNumber());
    video.setAutoWhiteBalance(true);

    // create sensor capture
    cout << Section("Open IMU(SensorCapture)");
    sensors::SensorCapture sensor(VERBOSITY::INFO);
    // get a list of available camera with sensor
    vector<int> devices = sensor.getDeviceList();
    LOG_IF(FATAL, devices.empty()) << "cannot found any ZED cameras";
    LOG(INFO) << fmt::format("available ZED devices: {}", devices);
    // initialize sensors
    CHECK(sensor.initializeSensors(devices[0])) << fmt::format("cannot init sensor {}", devices[0]);
    LOG(INFO) << fmt::format("sensor capture connected to camera SN = {}", sensor.getSerialNumber());
    // get firmware version
    uint16_t fwMajor, fwMinor;
    sensor.getFirmwareVersion(fwMajor, fwMinor);
    LOG(INFO) << fmt::format("firmware version: {}.{}", fwMajor, fwMinor);

    // enable Camera-IMU synchronization
    video.enableSensorSync(&sensor);

    // process sensor data
    cout << Section("Obtain Frame");
    uint64_t lastCameraTimestamp{0}, lastImuTimestamp{0};
    while (true) {
        // get camera temperature
        auto temp = sensor.getLastCameraTemperatureData();
        cout << fmt::format("t = {:.5f} s, left camera temperature = {:.5f}, right camera temperature = {:.5f},",
                            temp.timestamp * 1.E-9, temp.temp_left, temp.temp_right)
             << endl;

        // get IMU data
        auto imu = sensor.getLastIMUData();
        cout << fmt::format(
                    "IMU, t = {:.5f} s, acc = [{:.5f}, {:.5f}, {:.5f}] m/s^2"
                    ", gyro = [{:.5f}, {:.5f}, {:.5f}] deg/s, temperature = {} degree, sync = {}",
                    imu.timestamp * 1.E-9, imu.aX, imu.aY, imu.aZ, imu.gX, imu.gY, imu.gZ, imu.temp, imu.sync)
             << endl;

        // get image frame
        auto frame = video.getLastFrame();
        cout << fmt::format("[{}] t = {:.5f} s", frame.frame_id, frame.timestamp * 1.E-9) << endl;

        // calculate frequency and time delay
        uint64_t cameraDeltaT = frame.timestamp - lastCameraTimestamp;
        uint64_t imuDeltaT = imu.timestamp - lastImuTimestamp;
        LOG(INFO) << fmt::format("camera delta time = {:.5f}, IMU delta time = {:.5f}", cameraDeltaT * 1.E-9,
                                 imuDeltaT * 1.E-9);
        lastCameraTimestamp = frame.timestamp;
        lastImuTimestamp = imu.timestamp;

        // convert from YUV to BGR
        Mat frameYuv(frame.height, frame.width, CV_8UC2, frame.data);
        Mat frameBgr;
        cvtColor(frameYuv, frameBgr, COLOR_YUV2BGR_YUYV);

        if (showImg) {
            imshow("Image", frameBgr);
            // exit
            auto key = static_cast<char>(waitKey(1));
            if (key == 27 || key == 'q' || key == 'Q' || key == 'x' || key == 'X') {
                break;
            }
        }
    }

    google::ShutdownGoogleLogging();
    return 0;
}
