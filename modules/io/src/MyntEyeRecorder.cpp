#include "libra/io/MyntEyeRecorder.h"
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <glog/logging.h>
#include <turbojpeg.h>
#include <boost/date_time.hpp>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;
using namespace Eigen;
using namespace boost::posix_time;
using namespace boost::gregorian;
using namespace mynteyed;
using namespace libra::util;
using namespace libra::core;
using namespace libra::io;

// Constructor
MyntEyeRecorder::MyntEyeRecorder(unsigned int index, unsigned int frameRate, const size_t& saverThreadNum)
    : deviceIndex_(index),
      frameRate_(frameRate),
      streamMode_(StreamMode::STREAM_MODE_LAST),
      streamFormat_(StreamFormat::STREAM_FORMAT_LAST),
      saverThreadNum_(saverThreadNum),
      timestampMethod_(TimestampRetrieveMethod::Sensor),
      isRightCamEnabled_(false) {}

// Destructor
MyntEyeRecorder::~MyntEyeRecorder() {
    // wait process finish
    if (isStart()) {
        stop();
        wait();
    }

    if (cam_ && cam_->IsOpened()) {
        cam_->Close();
    }
}

// Get the device list
vector<pair<unsigned int, string>> MyntEyeRecorder::getDevices() {
    LOG(INFO) << "get device information...";

    // get device information
    Camera cam;
    auto deviceInfos = cam.GetDeviceInfos();
    LOG_IF(INFO, deviceInfos.empty()) << "cannot obtain any MyntEye devices";

    // add to list
    vector<pair<unsigned int, string>> devices(deviceInfos.size());
    for (size_t i = 0; i < deviceInfos.size(); ++i) {
        devices[i].first = deviceInfos[i].index;
        devices[i].second =
            deviceInfos[i].name + ", SN" + deviceInfos[i].sn.substr(0, 6);  // device name + serial number
        LOG(INFO) << fmt::format("[{}/{}] index = {}, name = {}, serial number = {}", i + 1, deviceInfos.size(),
                                 deviceInfos[i].index, deviceInfos[i].name, deviceInfos[i].sn);
    }

    return devices;
}

// Set the device index
void MyntEyeRecorder::setDeviceIndex(unsigned int deviceIndex) { deviceIndex_ = deviceIndex; }

// Set frame rate
void MyntEyeRecorder::setFrameRate(const unsigned int& frameRate) {
    if (frameRate < 0 || frameRate_ > 60) {
        LOG(ERROR) << fmt::format("frame rate should be in range [0, 60], input is {}", frameRate);
        return;
    }
    frameRate_ = frameRate;
}

// Set the stream mode of device
void MyntEyeRecorder::setStreamMode(mynteyed::StreamMode streamMode) {
    if (streamMode == StreamMode::STREAM_MODE_LAST) {
        LOG(WARNING) << fmt::format("don't support STREAM_MODE_LAST, use the last value = {}", streamMode_);
    } else {
        streamMode_ = streamMode;
    }
}

// Set the stream format of device,
void MyntEyeRecorder::setStreamFormat(mynteyed::StreamFormat streamFormat) {
    if (streamFormat == StreamFormat::STREAM_FORMAT_LAST) {
        LOG(WARNING) << fmt::format("don't support STREAM_FORMAT_LAST, use the last value = {}", streamFormat);
    } else {
        streamFormat_ = streamFormat;
    }
}

// Set the saver thread number
void MyntEyeRecorder::setSaverThreadNum(const size_t& saverThreadNum) { saverThreadNum_ = saverThreadNum; }

// Set the method to retrieve timestamp
void MyntEyeRecorder::setTimeStampRetrieveMethod(TimestampRetrieveMethod method) { timestampMethod_ = method; }

//  Set process function for raw image record of right camera
void MyntEyeRecorder::setRightProcessFunction(const std::function<void(const core::RawImageRecord&)>& func) {
    processRightRawImg_ = func;
}

// Initialize device
void MyntEyeRecorder::init() {
    openDevice();

    // check is right camera enabled
    isRightCamEnabled_ = cam_->IsStreamDataEnabled(ImageType::IMAGE_RIGHT_COLOR);

    // create image queue
    leftImageQueue_ = make_shared<JobQueue<RawImage>>(10);
    if (isRightCamEnabled_) {
        rightImageQueue_ = make_shared<JobQueue<RawImage>>(10);
    }

    createImageSaverThread();
}

// The main run function
void MyntEyeRecorder::run() {
    LOG(INFO) << fmt::format("Mynt Eye camera recording...");
    while (true) {
        if (isStop()) {
            // stop and close camera
            LOG(INFO) << "stop Mynt Eye camera recording";
            cam_->Close();
            cam_.reset();  // reset to stop watch dog

            // wait queue
            leftImageQueue_->wait();
            leftImageQueue_->stop();
            if (rightImageQueue_) {
                rightImageQueue_->wait();
                rightImageQueue_->stop();
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

            // reset image queue and clear thread
            leftImageSaverThreads_.clear();
            rightImageSaverThreads_.clear();

            break;
        }

        // wait stream
        cam_->WaitForStreams();

        // capture left image
        auto leftStream = cam_->GetStreamData(ImageType::IMAGE_LEFT_COLOR);
        if (leftStream.img) {
            RawImage raw;
            raw.timestamp = getTimestamp(leftStream.img_info);
            raw.img = leftStream.img->To(ImageFormat::COLOR_BGR);  // YUYV to BGR
            leftImageQueue_->push(move(raw));
        }

        // capture right image if enable
        if (isRightCamEnabled_) {
            auto rightStream = cam_->GetStreamData(ImageType::IMAGE_RIGHT_COLOR);
            if (rightStream.img) {
                RawImage raw;
                raw.timestamp = getTimestamp(rightStream.img_info);
                raw.img = rightStream.img->To(ImageFormat::COLOR_BGR);  // YUYV to BGR
                rightImageQueue_->push(move(raw));
            }
        }

        // read IMU data
        auto motionData = cam_->GetMotionDatas();
        for (auto& motion : motionData) {
            if (motion.imu) {
                static const double kDeg2Rad = M_PI / 180.;
                double timestamp = motion.imu->timestamp * 1.0E-5;                              // 0.01 ms => s
                Vector3d acc = Map<Vector3f>(motion.imu->accel).cast<double>() * Constant::kG;  // g => m/s^2
                Vector3d gyro = Map<Vector3f>(motion.imu->gyro).cast<double>() * kDeg2Rad;      // deg/s => rad/s
                ImuRecord imu(move(timestamp), ImuReading(move(acc), move(gyro)));
                processImu_(imu);
            }
        }
    }
}

// Open and set device
void MyntEyeRecorder::openDevice() {
    LOG(INFO) << "open and set Mynt Eye device";

    // creat cam
    if (!cam_) {
        cam_ = make_shared<Camera>();
    }

    // set open parameters
    OpenParams openParams(deviceIndex_);
    openParams.framerate = frameRate_;
    openParams.dev_mode = DeviceMode::DEVICE_COLOR;  // only image without depth
    openParams.color_mode = ColorMode::COLOR_RAW;    // undistored image
    openParams.stream_mode = streamMode_;            // stream mode
    openParams.color_stream_format = streamFormat_;  // stream format

    // open
    cam_->Open(openParams);
    CHECK(cam_->IsOpened()) << fmt::format("cannot open camera with device index = {}", deviceIndex_);

    // obtain parameters
    openParams = cam_->GetOpenParams();
    frameRate_ = openParams.framerate;
    streamMode_ = openParams.stream_mode;
    streamFormat_ = openParams.color_stream_format;
    // logging
    LOG(INFO) << fmt::format("frame rate = {} Hz", frameRate_);
    LOG(INFO) << fmt::format("stream mode = {}", streamMode_);
    LOG(INFO) << fmt::format("stream format = {}", streamFormat_);

    // enable image info
    cam_->EnableImageInfo(true);
    // endable IMU
    cam_->EnableMotionDatas();
    cam_->EnableProcessMode(ProcessMode::PROC_IMU_ALL);

    // get camera intrinsics and extrinsics
    StreamIntrinsics streamIntrinsics = cam_->GetStreamIntrinsics(streamMode_);
    LOG(INFO) << fmt::format("left camera intrinsics: {}", streamIntrinsics.left);
    LOG(INFO) << fmt::format("right camera intrinsics: {}", streamIntrinsics.right);
    StreamExtrinsics streamExtrinsics = cam_->GetStreamExtrinsics(streamMode_);
    LOG(INFO) << fmt::format("camera intrinsics: {}", streamExtrinsics);
    // get IMU intrinsics and extrinsics
    MotionIntrinsics motionIntrinsics = cam_->GetMotionIntrinsics();
    LOG(INFO) << fmt::format("IMU intrinsics: {}", motionIntrinsics);
    MotionExtrinsics motionExtrinsics = cam_->GetMotionExtrinsics();
    LOG(INFO) << fmt::format("IMU extrinsics: {}", motionExtrinsics);
}

// Create thread to save image, first compress image using turbo jpeg, then save to file
void MyntEyeRecorder::createImageSaverThread() {
    // create thread for left image
    if (isRightCamEnabled_) {
        LOG(INFO) << fmt::format("create image saver thread for left camera, thread num = {}", saverThreadNum_);
    } else {
        LOG(INFO) << fmt::format("create image saver thread, thread num = {}", saverThreadNum_);
    }
    for (size_t i = 0; i < saverThreadNum_; ++i) {
        leftImageSaverThreads_.emplace_back(thread([&]() {
            tjhandle compressor = tjInitCompress();

            while (true) {
                // take job and check it's valid
                auto job = leftImageQueue_->pop();
                if (!job.isValid()) {
                    break;
                }

                // compress image from BGR to jpeg
                RawImageRecord record;
                record.setTimestamp(job.data().timestamp);
                if (tjCompress2(compressor, job.data().img->data(), job.data().img->width(), 0,
                                job.data().img->height(), TJPF_BGR, &record.reading().buffer(),
                                &record.reading().size(), TJSAMP_444, 95, TJFLAG_FASTDCT) != 0) {
                    LOG(ERROR) << fmt::format("turbo jpeg compress error: {}", tjGetErrorStr2(compressor));
                }

                // process raw image
                if (processRawImg_) {
                    processRawImg_(record);
                }

                // release turbo jpeg data buffer
                tjFree(record.reading().buffer());
            }

            // destory compressor
            tjDestroy(compressor);
        }));
    }

    if (isRightCamEnabled_) {
        LOG(INFO) << fmt::format("create image saver thread for right camera, thread num = {}", saverThreadNum_);
        for (size_t i = 0; i < saverThreadNum_; ++i) {
            rightImageSaverThreads_.emplace_back(thread([&]() {
                tjhandle compressor = tjInitCompress();

                while (true) {
                    // take job and check it's valid
                    auto job = rightImageQueue_->pop();
                    if (!job.isValid()) {
                        break;
                    }

                    // compress image from BGR to jpeg
                    RawImageRecord record;
                    record.setTimestamp(job.data().timestamp);
                    if (tjCompress2(compressor, job.data().img->data(), job.data().img->width(), 0,
                                    job.data().img->height(), TJPF_BGR, &record.reading().buffer(),
                                    &record.reading().size(), TJSAMP_444, 95, TJFLAG_FASTDCT) != 0) {
                        LOG(ERROR) << fmt::format("turbo jpeg compress error: {}", tjGetErrorStr2(compressor));
                    }

                    // process raw image
                    if (processRightRawImg_) {
                        processRightRawImg_(record);
                    }

                    // release turbo jpeg data buffer
                    tjFree(record.reading().buffer());
                }

                // destory compressor
                tjDestroy(compressor);
            }));
        }
    }
}

// Get the timestamp based on timestamp retrieve method
double MyntEyeRecorder::getTimestamp(const shared_ptr<mynteyed::ImgInfo>& imageInfo) {
    switch (timestampMethod_) {
        case TimestampRetrieveMethod::Sensor:
            DCHECK(imageInfo) << "input null pointer";
            return imageInfo->timestamp * 1.0E-5;  // 0.01 ms => s
            break;

        case TimestampRetrieveMethod::Host:
            auto now = chrono::duration_cast<chrono::nanoseconds>(chrono::system_clock::now().time_since_epoch());
            return now.count() * 1.0E-9;
            break;
    }
}