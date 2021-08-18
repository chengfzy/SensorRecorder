#include "libra/io/MyntEyeRecorder.h"
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <glog/logging.h>
#include <jpeglib.h>
#include <turbojpeg.h>
#include <boost/date_time.hpp>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace Eigen;
// using namespace cv;
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
    leftImageQueue_ = make_shared<JobQueue<RawImage>>(30);
    if (isRightCamEnabled_) {
        rightImageQueue_ = make_shared<JobQueue<RawImage>>(30);
    }
    // create IMU queue
    imuQueue_ = make_shared<JobQueue<RawImu>>(300);

    // create threads to save image and IMU
    createSaverThread();
}

// The main run function
void MyntEyeRecorder::run() {
    LOG(INFO) << fmt::format("Mynt Eye camera recording...");
#if defined(DebugTest)
    int lastTime{0};
#endif

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
            imuQueue_->wait();
            imuQueue_->stop();

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

        // wait stream
        cam_->WaitForStreams();

        // capture left image
        auto leftStream = cam_->GetStreamData(ImageType::IMAGE_LEFT_COLOR);
        if (leftStream.img) {
#if defined(DebugTest)
            LOG(INFO) << fmt::format("obtain left frame, t = {}", leftStream.img_info->timestamp);
            int delta = leftStream.img_info->timestamp - lastTime;
            LOG_IF(WARNING, delta > 6000) << fmt::format("lost frame, t0 = {}, t1 = {}, deltaT = {}, N ~ {}", lastTime,
                                                         leftStream.img_info->timestamp, delta, delta / 3300.);
            lastTime = leftStream.img_info->timestamp;
#endif
            RawImage raw;
            raw.timestamp = move(leftStream.img_info->timestamp);
            raw.img = move(leftStream.img);
            LOG_IF(INFO, leftImageQueue_->size() >= 10) << fmt::format("left queue size = {}", leftImageQueue_->size());
            leftImageQueue_->push(move(raw));
        }

        // capture right image if enable
        if (isRightCamEnabled_) {
            auto rightStream = cam_->GetStreamData(ImageType::IMAGE_RIGHT_COLOR);
            if (rightStream.img) {
                RawImage raw;
                raw.timestamp = move(rightStream.img_info->timestamp);
                raw.img = move(rightStream.img);
                rightImageQueue_->push(move(raw));
            }
        }

        // read IMU data
        auto motionData = cam_->GetMotionDatas();
        for (auto& motion : motionData) {
            if (motion.imu) {
                RawImu raw;
                raw.systemTime = chrono::system_clock::now();
                raw.imu = motion.imu;
                // LOG(INFO) << fmt::format("IMU queue size = {}", leftImageQueue_->size());
                imuQueue_->push(move(raw));
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
    DLOG(INFO) << fmt::format("left camera intrinsics: {}", streamIntrinsics.left);
    DLOG(INFO) << fmt::format("right camera intrinsics: {}", streamIntrinsics.right);
    StreamExtrinsics streamExtrinsics = cam_->GetStreamExtrinsics(streamMode_);
    DLOG(INFO) << fmt::format("camera intrinsics: {}", streamExtrinsics);
    // get IMU intrinsics and extrinsics
    MotionIntrinsics motionIntrinsics = cam_->GetMotionIntrinsics();
    DLOG(INFO) << fmt::format("IMU intrinsics: {}", motionIntrinsics);
    MotionExtrinsics motionExtrinsics = cam_->GetMotionExtrinsics();
    DLOG(INFO) << fmt::format("IMU extrinsics: {}", motionExtrinsics);
}

// Create thread to save image and IMU
void MyntEyeRecorder::createSaverThread() {
    createImageSaverThread();

    // create thread to save IMU
    createImuSaverThread();
}

// Create thread for save image
void MyntEyeRecorder::createImageSaverThread() {
    // save function for MJPG format
    auto jpegFunc = [](shared_ptr<JobQueue<RawImage>>& imageQueue,
                       const function<void(const RawImageRecord&)>& processFunc) {
        while (true) {
            // take job and check it's valid
            auto job = imageQueue->pop();
            if (!job.isValid()) {
                break;
            }

            // convert unit
            RawImageRecord record;
            record.setTimestamp(job.data().timestamp * 1.0E-5);  // 0.01 ms => s
            record.reading().buffer() = job.data().img->data();
            record.reading().size() = job.data().img->valid_size();

            // process raw image
            if (processFunc) {
                processFunc(record);
            }
        }
    };

    // compress image and then save
#if false
    // compress using jpeglib
    auto yuyvFunc = [](shared_ptr<JobQueue<RawImage>>& imageQueue,
                       const function<void(const RawImageRecord&)>& processFunc) {
        while (true) {
            // take job and check it's valid
            auto job = imageQueue->pop();
            if (!job.isValid()) {
                break;
            }

            // compress image from BGR to jpeg
            RawImageRecord record;
            record.setTimestamp(job.data().timestamp * 1.0E-5);  // 0.01 ms => s

            jpeg_compress_struct cinfo;
            jpeg_error_mgr jerr;
            JSAMPROW rowPtr[1];
            cinfo.err = jpeg_std_error(&jerr);
            jpeg_create_compress(&cinfo);
            jpeg_mem_dest(&cinfo, &record.reading().buffer(), &record.reading().size());
            cinfo.image_width = job.data().img->width() & -1;
            cinfo.image_height = job.data().img->height() & -1;
            cinfo.input_components = 3;
            cinfo.in_color_space = JCS_YCbCr;
            cinfo.dct_method = JDCT_IFAST;

            jpeg_set_defaults(&cinfo);
            jpeg_set_quality(&cinfo, 95, TRUE);
            jpeg_start_compress(&cinfo, TRUE);

            vector<uint8_t> tmprowbuf(job.data().img->width() * 3);
            JSAMPROW row_pointer[1];
            row_pointer[0] = &tmprowbuf[0];
            while (cinfo.next_scanline < cinfo.image_height) {
                unsigned i, j;
                unsigned offset = cinfo.next_scanline * cinfo.image_width * 2;  // offset to the correct row
                for (i = 0, j = 0; i < cinfo.image_width * 2; i += 4, j += 6) {
                    // input strides by 4 bytes, output strides by 6 (2 pixels)
                    tmprowbuf[j + 0] = job.data().img->data()[offset + i + 0];  // Y (unique to this pixel)
                    tmprowbuf[j + 1] = job.data().img->data()[offset + i + 1];  // U (shared between pixels)
                    tmprowbuf[j + 2] = job.data().img->data()[offset + i + 3];  // V (shared between pixels)
                    tmprowbuf[j + 3] = job.data().img->data()[offset + i + 2];  // Y (unique to this pixel)
                    tmprowbuf[j + 4] = job.data().img->data()[offset + i + 1];  // U (shared between pixels)
                    tmprowbuf[j + 5] = job.data().img->data()[offset + i + 3];  // V (shared between pixels)
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
#else
    // convert YUYV(YUV422 Packed) to YUV(YUV422 Planar), then compress using turbo-jpeg
    auto yuyvFunc = [](shared_ptr<JobQueue<RawImage>>& imageQueue,
                       const function<void(const RawImageRecord&)>& processFunc) {
        tjhandle compressor = tjInitCompress();
        vector<unsigned char> yuvData;

        while (true) {
            // take job and check it's valid
            auto job = imageQueue->pop();
            if (!job.isValid()) {
                break;
            }

            // resize buffer if don't match
            int wh = job.data().img->width() * job.data().img->height();
            int length = 2 * wh;
            if (yuvData.size() != length) {
                yuvData.resize(length);
            }

            // convert YUYV(YUV422 Packed) to YUV(YUV422 Planar)
            unsigned char* pY = yuvData.data();
            unsigned char* pU = yuvData.data() + wh;
            unsigned char* pV = yuvData.data() + wh + wh / 2;
            uint8_t* imgData = job.data().img->data();
            for (int i = 0; i < length;) {
                *pY = imgData[i++];
                *pU = imgData[i++];
                ++pY;
                ++pU;
                *pY = imgData[i++];
                *pV = imgData[i++];
                ++pY;
                ++pV;
            }

            // compress image using turbojpeg
            RawImageRecord record;
            record.setTimestamp(job.data().timestamp * 1.0E-5);  // 0.01 ms => s
            if (tjCompressFromYUV(compressor, yuvData.data(), job.data().img->width(), 1, job.data().img->height(),
                                  TJSAMP_422, &record.reading().buffer(), &record.reading().size(), 95,
                                  TJFLAG_FASTDCT) != 0) {
                LOG(ERROR) << fmt::format("turbo jpeg compress error: {}", tjGetErrorStr2(compressor));
            }

            // process raw image
            if (processFunc) {
                processFunc(record);
            }

            // release turbo jpeg data buffer
            tjFree(record.reading().buffer());
        }

        // destory compressor
        tjDestroy(compressor);
    };
#endif

    // create thread for left image
    if (isRightCamEnabled_) {
        LOG(INFO) << fmt::format("create image saver thread for left camera, thread num = {}", saverThreadNum_);
    } else {
        LOG(INFO) << fmt::format("create image saver thread, thread num = {}", saverThreadNum_);
    }
    for (size_t i = 0; i < saverThreadNum_; ++i) {
        if (streamFormat_ == StreamFormat::STREAM_MJPG) {
            leftImageSaverThreads_.emplace_back(thread([&]() { jpegFunc(leftImageQueue_, processRawImg_); }));
        } else if (streamFormat_ == StreamFormat::STREAM_YUYV) {
            leftImageSaverThreads_.emplace_back(thread([&]() { yuyvFunc(leftImageQueue_, processRawImg_); }));
        }
    }

    // create stread for right image
    if (isRightCamEnabled_) {
        LOG(INFO) << fmt::format("create image saver thread for right camera, thread num = {}", saverThreadNum_);
        if (streamFormat_ == StreamFormat::STREAM_MJPG) {
            leftImageSaverThreads_.emplace_back(thread([&]() { jpegFunc(rightImageQueue_, processRightRawImg_); }));
        } else if (streamFormat_ == StreamFormat::STREAM_YUYV) {
            leftImageSaverThreads_.emplace_back(thread([&]() { yuyvFunc(rightImageQueue_, processRightRawImg_); }));
        }
    }
}

// Create thread for save IMU
void MyntEyeRecorder::createImuSaverThread() {
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
            double sensorTimestamp = job.data().imu->timestamp * 1.0E-5;  // 0.01 ms => s
            double systemTimestamp =
                chrono::duration_cast<chrono::nanoseconds>(job.data().systemTime.time_since_epoch()).count() * 1.0E-9;
            Vector3d acc = Map<Vector3f>(job.data().imu->accel).cast<double>() * Constant::kG;  // g => m/s^2
            Vector3d gyro = Map<Vector3f>(job.data().imu->gyro).cast<double>() * kDeg2Rad;      // deg/s => rad/s
            ImuRecord imu(move(sensorTimestamp), ImuReading(move(acc), move(gyro)));
            imu.setSystemTimestamp(move(systemTimestamp));

            // process IMU
            if (processImu_) {
                processImu_(imu);
            }
        }
    });
}