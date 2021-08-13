#include <fmt/color.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <glog/logging.h>
#include <mynteyed/camera.h>
#include <mynteyed/utils.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <cxxopts.hpp>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

using namespace std;
using namespace cv;
using namespace mynteyed;
namespace fs = boost::filesystem;

// get the section string
string section(const string& text) {
    return fmt::format(fmt::fg(fmt::color::cyan), "{:═^{}}", " " + text + " ",
                       max(100, static_cast<int>(text.size() + 12)));
}

int main(int argc, char* argv[]) {
    // argument parser
    cxxopts::Options options(argv[0], "Recorder");
    // clang-format off
    options.add_options()("f,folder", "save folder", cxxopts::value<string>()->default_value("./data"))
        ("frameRate", "frame rate", cxxopts::value<int>()->default_value("30"))
        ("streamMode", "stream mode", cxxopts::value<string>()->default_value("1280x720"))
        ("streamFormat", "stream format", cxxopts::value<string>()->default_value("MJPG"))
        ("showImage", "show image", cxxopts::value<bool>())
        ("h,help", "help message");
    // clang-format on
    auto result = options.parse(argc, argv);
    if (result.count("help")) {
        cout << options.help() << endl;
        return 0;
    }
    string rootFolder = result["folder"].as<string>();
    int frameRate = result["frameRate"].as<int>();
    string streamModeName = result["streamMode"].as<string>();
    string streamFormatName = result["streamFormat"].as<string>();
    bool showImg = result["showImage"].as<bool>();

    // check stream mode
    vector<string> streamModeNames = {"2560x720", "1280x720", "1280x480", "640x480"};
    if (find_if(streamModeNames.begin(), streamModeNames.end(),
                [&](const string& s) { return boost::iequals(s, streamModeName); }) == streamModeNames.end()) {
        cout << fmt::format("input stream mode type should be one item in {}", streamModeNames) << endl << endl;
        cout << options.help() << endl;
        return 0;
    }
    // check stream format
    vector<string> streamFormatNames = {"YUYV", "MJPG"};
    if (find_if(streamFormatNames.begin(), streamFormatNames.end(),
                [&](const string& s) { return boost::iequals(s, streamFormatName); }) == streamFormatNames.end()) {
        cout << fmt::format("input stream format should be one item in {}", streamFormatNames) << endl << endl;
        cout << options.help() << endl;
        return 0;
    }

    // print input parameters
    cout << section("Recorder") << endl;
    cout << fmt::format("save folder: {}", rootFolder) << endl;
    cout << fmt::format("frame rate = {} Hz", frameRate) << endl;
    cout << fmt::format("stream mode: {}", streamModeName) << endl;
    cout << fmt::format("stream format: {}", streamFormatName) << endl;
    cout << fmt::format("show image: {}", showImg) << endl;

    // init glog
    google::InitGoogleLogging(argv[0]);
    FLAGS_alsologtostderr = true;
    FLAGS_colorlogtostderr = true;

    // get stream mode from string
    StreamMode streamMode;
    if (boost::iequals(streamModeName, "2560x720")) {
        streamMode = StreamMode::STREAM_2560x720;
    } else if (boost::iequals(streamModeName, "1280x720")) {
        streamMode = StreamMode::STREAM_1280x720;
    } else if (boost::iequals(streamModeName, "1280x480")) {
        streamMode = StreamMode::STREAM_1280x480;
    } else if (boost::iequals(streamModeName, "640x480")) {
        streamMode = StreamMode::STREAM_640x480;
    }
    // get stream format from string
    StreamFormat streamFormat;
    if (boost::iequals(streamFormatName, "YUYV")) {
        streamFormat = StreamFormat::STREAM_YUYV;
    } else if (boost::iequals(streamFormatName, "MJPG")) {
        streamFormat = StreamFormat::STREAM_MJPG;
    }

    // create directories
    fs::path rootPath{rootFolder};
    fs::path leftPath = rootPath / "left";
    fs::path rightPath = rootPath / "right";
    // remove old file and create save folder
    if (fs::is_directory(rootPath)) {
        fs::remove_all(rootPath);
    }
    fs::create_directories(rootPath);
    fs::create_directories(leftPath);
    fs::create_directories(rightPath);

    // create IMU file name
    fs::path imuPath = rootPath / "imu.csv";
    std::fstream imuFile(imuPath.string(), ios::out);
    CHECK(imuFile.is_open()) << fmt::format("cannot create IMU file \"{}\"", imuPath.string());
    imuFile << "# Timestamp(ns), AccX(m/s^2), AccY(m/s^2), AccZ(m/s^2), GyroX(rad/s), GyroY(rad/s), GyroZ(rad/s)"
            << endl;

    // get device information(list)
    cout << section("Device Information") << endl;
    Camera cam;
    DeviceInfo deviceInfo;
    if (!util::select(cam, &deviceInfo)) {
        LOG(FATAL) << "cannot get device information";
    }
    // print out device information
    util::print_stream_infos(cam, deviceInfo.index);

    // open camera
    cout << section("Open Camera") << endl;
    LOG(INFO) << fmt::format("open device, index = {}, name = {}", deviceInfo.index, deviceInfo.name) << endl;
    // set open parameters
    OpenParams openParams(deviceInfo.index);
    openParams.framerate = frameRate;
    openParams.dev_mode = DeviceMode::DEVICE_COLOR;
    openParams.color_mode = ColorMode::COLOR_RAW;
    openParams.stream_mode = streamMode;
    openParams.color_stream_format = streamFormat;
    // open
    cam.Open(openParams);
    if (!cam.IsOpened()) {
        LOG(FATAL) << "open camera failed";
    } else {
        LOG(INFO) << "open device success";
    }

    // data enable
    cam.EnableImageInfo(true);
    cam.EnableProcessMode(ProcessMode::PROC_IMU_ALL);
    cam.EnableMotionDatas();
    bool isRightCameraEnable = cam.IsStreamDataEnabled(ImageType::IMAGE_RIGHT_COLOR);
    LOG(INFO) << fmt::format("FPS = {} Hz", cam.GetOpenParams().framerate);
    LOG(INFO) << fmt::format("is left enabled = {}", cam.IsStreamDataEnabled(ImageType::IMAGE_LEFT_COLOR));
    LOG(INFO) << fmt::format("is right enabled = {}", isRightCameraEnable);

    // obtain sensor data and save
    cout << section("Process Sensor Data") << endl;
    size_t leftImageNum{0}, rightImageNum{0};
    constexpr double kDeg2Rad = M_PI / 180.;
    constexpr double kG{9.81};
    double lastLeftTimestamp{0};
    while (true) {
        cam.WaitForStream();

        // get left stream
        auto leftStreams = cam.GetStreamDatas(ImageType::IMAGE_LEFT_COLOR);
        for (auto& leftStream : leftStreams) {
            if (leftStream.img && leftStream.img_info) {
                LOG(INFO) << fmt::format("process left image, index = {}, timestamp = {:.5f} s", leftImageNum,
                                         leftStream.img_info->timestamp * 1.E-5);

                double currentTime = leftStream.img_info->timestamp * 1.E-5;
                double delta = currentTime - lastLeftTimestamp;
                LOG_IF(WARNING, delta > 0.06) << fmt::format(
                    "lost frame, last timestamp = {:.5f} s, current timestamp  = {:.5f} s, delta time = {:.5f} s",
                    lastLeftTimestamp, currentTime, delta);
                lastLeftTimestamp = currentTime;

                // // get image info
                // auto type = leftStream.img->type();
                // auto format = leftStream.img->format();
                // auto width = leftStream.img->width();
                // auto height = leftStream.img->height();
                // auto is_buffer = leftStream.img->is_buffer();
                // auto frameId = leftStream.img->frame_id();
                // auto is_dual = leftStream.img->is_dual();
                // auto data_size = leftStream.img->data_size();
                // auto valid_size = leftStream.img->valid_size();
                // auto size = leftStream.img->size();
                // auto get_image_profile = leftStream.img->get_image_profile();

                // Mat img = leftStream.img->To(ImageFormat::COLOR_BGR)->ToMat();
                // fs::path fileName = leftPath / fmt::format("{}.jpg", leftStream.img_info->timestamp * 1.0E4);
                // imwrite(fileName.string(), img);

                // Mat img = leftStream.img->To(ImageFormat::COLOR_BGR)->ToMat();
                // fs::path fileName = leftPath / fmt::format("{}.jpg", leftStream.img_info->timestamp * 1.0E4);
                // imwrite(fileName.string(), img);

                // show
                // if (showImg || imgNum / 10 == 0) {
                //     imshow("Left", img);
                // }
            }
        }
        ++leftImageNum;

#if false
        // get right stream
        if (isRightCameraEnable) {
            auto rightStream = cam.GetStreamData(ImageType::IMAGE_RIGHT_COLOR);
            if (rightStream.img && rightStream.img_info) {
                LOG(INFO) << fmt::format("process right image, index = {}", rightImageNum);
                Mat img = rightStream.img->To(ImageFormat::COLOR_BGR)->ToMat();
                fs::path fileName = rightPath / fmt::format("{}.jpg", rightStream.img_info->timestamp * 1.0E4);
                imwrite(fileName.string(), img);

                // // show
                // if (showImg || rightImageNum / 10 == 0) {
                //     imshow("Right", img);
                // }
            }
            ++rightImageNum;
        }

        // get IMU
        auto motionData = cam.GetMotionDatas();
        for (auto& motion : motionData) {
            if (motion.imu) {
                imuFile << fmt::format("{},{},{}, {},{},{},{}", motion.imu->timestamp * 1.0E4,
                                       motion.imu->gyro[0] * kDeg2Rad, motion.imu->gyro[1] * kDeg2Rad,
                                       motion.imu->gyro[2] * kDeg2Rad, motion.imu->accel[0] * kG,
                                       motion.imu->accel[1] * kG, motion.imu->accel[2] * kG)
                        << endl;
            }
        }

#endif

        // exit
        // auto key = static_cast<char>(waitKey(1));
        // if (key == 27 || key == 'q' || key == 'Q' || key == 'x' || key == 'X') {
        //     break;
        // }
    }

    imuFile.close();
    cam.Close();

    google::ShutdownGoogleLogging();
    return 0;
}
