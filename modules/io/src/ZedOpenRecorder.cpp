#include "libra/io/ZedOpenRecorder.h"
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <glog/logging.h>
#include <jpeglib.h>
#include <turbojpeg.h>
#include <boost/date_time.hpp>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace Eigen;
using namespace boost::posix_time;
using namespace boost::gregorian;
using namespace sl_oc;
using namespace sl_oc::sensors;
using namespace sl_oc::video;
using namespace libra::util;
using namespace libra::core;
using namespace libra::io;

// Constructor
ZedOpenRecorder::ZedOpenRecorder(int index, const size_t& saverThreadNum)
    : deviceIndex_(index),
      fps_(video::FPS::FPS_30),
      resolution_(video::RESOLUTION::HD720),
      saverThreadNum_(saverThreadNum),
      isRightCamEnabled_(false) {
    imuCapture_ = make_shared<SensorCapture>(VERBOSITY::WARNING);
}

// Destructor
ZedOpenRecorder::~ZedOpenRecorder() {
    // wait process finish
    if (isStart()) {
        stop();
        wait();
    }
}

// Get the device list
vector<pair<unsigned int, string>> ZedOpenRecorder::getDevices() {
    LOG(INFO) << "get device information...";

    // get device information
    SensorCapture sensor;
    auto deviceInfos = sensor.getDeviceList();
    LOG_IF(INFO, deviceInfos.empty()) << "cannot obtain any ZED devices";

    // add to list
    vector<pair<unsigned int, string>> devices(deviceInfos.size());
    for (size_t i = 0; i < deviceInfos.size(); ++i) {
        devices[i].first = deviceInfos[i];              // SN number
        devices[i].second = to_string(deviceInfos[i]);  // SN number string
        LOG(INFO) << fmt::format("[{}/{}] serial number = {}", i + 1, deviceInfos.size(), deviceInfos[i]);
    }

    return devices;
}

// Set the device index
void ZedOpenRecorder::setDeviceIndex(int deviceIndex) { deviceIndex_ = deviceIndex; }

// Set fps
void ZedOpenRecorder::setFps(sl_oc::video::FPS fps) {
    if (fps == video::FPS::LAST) {
        LOG(WARNING) << fmt::format("don't support FPS::LAST, use the last value = {}", fps_);
    } else {
        fps_ = fps;
    }
}

// Set resolution
void ZedOpenRecorder::setResolution(sl_oc::video::RESOLUTION resolution) {
    if (resolution == video::RESOLUTION::LAST) {
        LOG(WARNING) << fmt::format("don't support RESOLUTION::LAST, use the last value = {}", resolution_);
    } else {
        resolution_ = resolution;
    }
}

// Set the saver thread number
void ZedOpenRecorder::setSaverThreadNum(const size_t& saverThreadNum) { saverThreadNum_ = saverThreadNum; }

//  Set process function for raw image record of right camera
void ZedOpenRecorder::setRightProcessFunction(const std::function<void(const core::RawImageRecord&)>& func) {
    processRightRawImg_ = func;
}

// Initialize device
void ZedOpenRecorder::init() {
    openDevice();

    // create IMU capture thread
    LOG(INFO) << "create IMU capture thread";
    imuCaptureThread_ = thread([&] {
        while (true) {
            if (isStop()) {
                LOG(INFO) << "stop IMU recording";
                break;
            }

            // read IMU data
            auto imu = imuCapture_->getLastIMUData();
            if (imu.valid == data::Imu::ImuStatus::NEW_VAL) {
                RawImu raw;
                raw.systemTime = chrono::system_clock::now();
                raw.imu = move(imu);
                // LOG(INFO) << fmt::format("IMU queue size = {}", leftImageQueue_->size());
                imuQueue_->push(move(raw));
            }
        }
    });

    // check is right camera enabled
    isRightCamEnabled_ = processRightRawImg_.operator bool();

    // create image queue
    leftImageQueue_ = make_shared<JobQueue<Frame>>(30);
    if (isRightCamEnabled_) {
        rightImageQueue_ = make_shared<JobQueue<Frame>>(30);
    }
    // create IMU queue
    imuQueue_ = make_shared<JobQueue<RawImu>>(300);

    // create threads to save image and IMU
    createSaverThread();
}

// The main run function
void ZedOpenRecorder::run() {
    LOG(INFO) << fmt::format("ZED camera recording using Open Capture library...");
#if defined(DebugTest)
    int lastTime{0};
#endif

    while (true) {
        if (isStop()) {
            // stop and close camera
            LOG(INFO) << "stop ZED recording";

            // wait queue
            leftImageQueue_->wait();
            leftImageQueue_->stop();
            if (rightImageQueue_) {
                rightImageQueue_->wait();
                rightImageQueue_->stop();
            }
            imuQueue_->wait();
            imuQueue_->stop();

            // wait IMU capture thread
            if (imuCaptureThread_.joinable()) {
                imuCaptureThread_.join();
            }

            // wait saver thread
            for (auto& t : leftImageSaverThreads_) {
                if (t.joinable()) {
                    t.join();
                }
            }
            for (auto& t : rightImageSaverThreads_) {
                if (t.joinable()) {
                    t.join();
                }
            }
            if (imuSaverThread_.joinable()) {
                imuSaverThread_.join();
            }

            // reset image queue and clear thread
            leftImageSaverThreads_.clear();
            rightImageSaverThreads_.clear();

            break;
        }

        // capture image
        auto frame = cameraCapture_->getLastFrame();
        if (frame.data != nullptr) {
#if defined(DebugTest)
            LOG(INFO) << fmt::format("obtain left frame, t = {} ns", frame.timestamp);
            int delta = frame.timestamp - lastTime;
            LOG_IF(WARNING, delta > 60E6) << fmt::format("lost frame, t0 = {}, t1 = {}, deltaT = {}, N ~ {}", lastTime,
                                                         frame.timestamp, delta, delta / 33E6);
            lastTime = frame.timestamp;
#endif
            LOG_IF(INFO, leftImageQueue_->size() >= 10) << fmt::format("left queue size = {}", leftImageQueue_->size());
            leftImageQueue_->push(move(frame));
        }

        // capture right image if enable
        if (isRightCamEnabled_) {
            // TODO
        }
    }
}

// Open and set device
void ZedOpenRecorder::openDevice() {
    LOG(INFO) << "open and set ZED device";

    // create IMU capture
    imuCapture_ = make_shared<SensorCapture>(VERBOSITY::WARNING);
    CHECK(imuCapture_->initializeSensors(deviceIndex_))
        << fmt::format("cannot open IMU capture with device index = {}", deviceIndex_);
    LOG(INFO) << fmt::format("IMU serial number = {}", imuCapture_->getSerialNumber());
    // get firmware version
    uint16_t fwMajor, fwMinor;
    imuCapture_->getFirmwareVersion(fwMajor, fwMinor);
    LOG(INFO) << fmt::format("firmware version: {}.{}", fwMajor, fwMinor);

    // create video capture
    video::VideoParams videoParams;
    videoParams.fps = fps_;
    videoParams.res = resolution_;
#if defined(DebugTest)
    videoParams.verbose = 1;
#endif
    cameraCapture_ = make_shared<VideoCapture>(videoParams);
    CHECK(cameraCapture_->initializeVideo(-1)) << fmt::format("cannot open camera with device index {}", deviceIndex_);
    LOG(INFO) << fmt::format("camera serial number: {}", cameraCapture_->getSerialNumber());
    // auto white balance
    cameraCapture_->setAutoWhiteBalance(true);
    // sensor synchronize
    cameraCapture_->enableSensorSync(imuCapture_.get());

    // obtain parameters
    int width{0}, height{0};
    cameraCapture_->getFrameSize(width, height);
    // logging
    LOG(INFO) << fmt::format("frame rate = {} Hz", static_cast<int>(fps_));
    LOG(INFO) << fmt::format("frame size = {}x{}", width, height);
}

// Create thread to save image and IMU
void ZedOpenRecorder::createSaverThread() {
    createImageSaverThread();

    // create thread to save IMU
    createImuSaverThread();
}

// Create thread for save image
void ZedOpenRecorder::createImageSaverThread() {
    // compress image and then save
    auto yuyvFunc = [](shared_ptr<JobQueue<Frame>>& imageQueue,
                       const function<void(const RawImageRecord&)>& processFunc) {
        while (true) {
            // take job and check it's valid
            auto job = imageQueue->pop();
            if (!job.isValid()) {
                break;
            }

            // compress image from BGR to jpeg
            RawImageRecord record;
            record.setTimestamp(job.data().timestamp * 1.0E-9);  // ns => s

            jpeg_compress_struct cinfo;
            jpeg_error_mgr jerr;
            JSAMPROW rowPtr[1];
            cinfo.err = jpeg_std_error(&jerr);
            jpeg_create_compress(&cinfo);
            jpeg_mem_dest(&cinfo, &record.reading().buffer(), &record.reading().size());
            cinfo.image_width = (job.data().width / 2) & -1;
            cinfo.image_height = job.data().height & -1;
            cinfo.input_components = 3;
            cinfo.in_color_space = JCS_YCbCr;
            cinfo.dct_method = JDCT_IFAST;

            jpeg_set_defaults(&cinfo);
            jpeg_set_quality(&cinfo, 95, TRUE);
            jpeg_start_compress(&cinfo, TRUE);

            vector<uint8_t> tmprowbuf(job.data().width / 2 * 3);
            JSAMPROW row_pointer[1];
            row_pointer[0] = &tmprowbuf[0];
            while (cinfo.next_scanline < cinfo.image_height) {
                unsigned i, j;
                unsigned offset = cinfo.next_scanline * cinfo.image_width * 2 * 2;  // offset to the correct row
                for (i = 0, j = 0; i < cinfo.image_width * 2; i += 4, j += 6) {
                    // input strides by 4 bytes, output strides by 6 (2 pixels)
                    tmprowbuf[j + 0] = job.data().data[offset + i + 0];  // Y (unique to this pixel)
                    tmprowbuf[j + 1] = job.data().data[offset + i + 1];  // U (shared between pixels)
                    tmprowbuf[j + 2] = job.data().data[offset + i + 3];  // V (shared between pixels)
                    tmprowbuf[j + 3] = job.data().data[offset + i + 2];  // Y (unique to this pixel)
                    tmprowbuf[j + 4] = job.data().data[offset + i + 1];  // U (shared between pixels)
                    tmprowbuf[j + 5] = job.data().data[offset + i + 3];  // V (shared between pixels)
                }
                jpeg_write_scanlines(&cinfo, row_pointer, 1);
            }

            jpeg_finish_compress(&cinfo);

            // process raw image
            if (processFunc) {
                processFunc(record);
            }

            // release data
            jpeg_destroy_compress(&cinfo);
        }
    };

    // create thread for left image
    if (isRightCamEnabled_) {
        LOG(INFO) << fmt::format("create image saver thread for left camera, thread num = {}", saverThreadNum_);
    } else {
        LOG(INFO) << fmt::format("create image saver thread, thread num = {}", saverThreadNum_);
    }
    for (size_t i = 0; i < saverThreadNum_; ++i) {
        leftImageSaverThreads_.emplace_back(thread([&]() { yuyvFunc(leftImageQueue_, processRawImg_); }));
    }

    // create stread for right image
    if (isRightCamEnabled_) {
        LOG(INFO) << fmt::format("create image saver thread for right camera, thread num = {}", saverThreadNum_);
        leftImageSaverThreads_.emplace_back(thread([&]() { yuyvFunc(rightImageQueue_, processRightRawImg_); }));
    }
}

// Create thread for save IMU
void ZedOpenRecorder::createImuSaverThread() {
    LOG(INFO) << "create IMU saver thread";
    imuSaverThread_ = thread([&] {
        while (true) {
            // take job and check it's valid
            auto job = imuQueue_->pop();
            if (!job.isValid()) {
                break;
            }

            // convert unit
            static const double kDeg2Rad = M_PI / 180.;
            double sensorTimestamp = job.data().imu.timestamp * 1.0E-9;  // ns => s
            double systemTimestamp =
                chrono::duration_cast<chrono::nanoseconds>(job.data().systemTime.time_since_epoch()).count() * 1.0E-9;
            Vector3d acc(job.data().imu.aX, job.data().imu.aY, job.data().imu.aZ);
            Vector3d gyro(job.data().imu.gX, job.data().imu.gY, job.data().imu.gZ);
            gyro = gyro * kDeg2Rad;
            ImuRecord imu(move(sensorTimestamp), ImuReading(move(acc), move(gyro)));
            imu.setSystemTimestamp(move(systemTimestamp));

            // process IMU
            if (processImu_) {
                processImu_(imu);
            }
        }
    });
}