#include "libra/io/NormalCameraRecorder.h"
#include <fmt/format.h>
#include <glog/logging.h>

using namespace std;
using namespace Eigen;
using namespace libra::util;
using namespace libra::core;
using namespace libra::io;

// Construct with device name and image saver thread number
NormalCameraRecorder::NormalCameraRecorder(const string& device, const size_t& saverThreadNum)
    : device_(device), saverThreadNum_(saverThreadNum_), fps_(0), frameSize_(Vector2i::Zero()) {}

// Destructor
NormalCameraRecorder::~NormalCameraRecorder() {
    // wait process finish
    if (isStart()) {
        stop();
        wait();
    }

    // close video capture
    if (videoCapture_.isOpened()) {
        videoCapture_.release();
    }
}

// Set the Camera device name. Should be set before `init()`
void NormalCameraRecorder::setDevice(const string& device) { device_ = device; }

// Set the saver thread number. Should be set before `init()`
void NormalCameraRecorder::setSaverThreadNum(const size_t& saverThreadNum) { saverThreadNum_ = saverThreadNum; }

// Set the FPS to device
const double& NormalCameraRecorder::setFps(const double& fps) {
    CHECK(videoCapture_.isOpened()) << "should \"init()\" firstly";

    if (fps_ != fps) {
        videoCapture_.set(cv::CAP_PROP_FPS, fps);
        fps_ = videoCapture_.get(cv::CAP_PROP_FPS);
        LOG(INFO) << fmt::format("set FPS = {} Hz", fps_);
    }
    return fps_;
}

// Set the frame size of device
const Eigen::Vector2i& NormalCameraRecorder::setFrameSize(const Vector2i& size) {
    CHECK(videoCapture_.isOpened()) << "should \"init()\" firstly";

    if (frameSize_ != size) {
        videoCapture_.set(cv::CAP_PROP_FRAME_WIDTH, size[0]);
        videoCapture_.set(cv::CAP_PROP_FRAME_HEIGHT, size[1]);
        frameSize_[0] = videoCapture_.get(cv::CAP_PROP_FRAME_WIDTH);
        frameSize_[1] = videoCapture_.get(cv::CAP_PROP_FRAME_HEIGHT);
        LOG(INFO) << fmt::format("set frame size = {} x {}", frameSize_[0], frameSize_[1]);
    }
    return frameSize_;
}

// Initialize camera
void NormalCameraRecorder::init() {
    LOG(INFO) << fmt::format("init normal camera, device: \"{}\"", device_);

    // open camera
    videoCapture_.open(device_);
    CHECK(videoCapture_.isOpened()) << fmt::format("cannot open camera {}", device_);

    // preset larger frame size to obtain the max image
    videoCapture_.set(cv::CAP_PROP_FRAME_WIDTH, 3000);
    videoCapture_.set(cv::CAP_PROP_FRAME_HEIGHT, 3000);

    // get FPS and frame size
    fps_ = videoCapture_.get(cv::CAP_PROP_FPS);
    frameSize_[0] = videoCapture_.get(cv::CAP_PROP_FRAME_WIDTH);
    frameSize_[1] = videoCapture_.get(cv::CAP_PROP_FRAME_HEIGHT);
    LOG(INFO) << fmt::format("FPS = {} Hz, frame size = {} x {}", fps_, frameSize_[0], frameSize_[1]);

    // reset image queue
    imageQueue_ = make_shared<JobQueue<ImageRecord>>(10);  // use image queue size of 10

    // create image thread
    createImageSaverThread();
}

// The main run function
void NormalCameraRecorder::run() {
    LOG(INFO) << fmt::format("normal camera \"{}\" recording", device_);
    while (true) {
        if (isStop()) {
            // stop capture
            LOG(INFO) << fmt::format("stop normal camera \"{}\" recording", device_);
            videoCapture_.release();

            // wait queue
            imageQueue_->wait();
            imageQueue_->stop();

            // wait saver thread
            for (auto& t : imageSaverThreads_) {
                if (t.joinable()) {
                    t.join();
                }
            }

            break;
        }

        // capture image
        ImageRecord record;
        if (videoCapture_.read(record.reading())) {
            auto now = chrono::duration_cast<chrono::nanoseconds>(chrono::system_clock::now().time_since_epoch());
            record.setTimestamp(now.count() * 1.0E-9);
            imageQueue_->push(move(record));
        }
    }
}

// Create thread to save image to save image to file
void NormalCameraRecorder::createImageSaverThread() {
    LOG(INFO) << fmt::format("create image saver thread for normal camera \"{}\", thread number = {}", device_,
                             saverThreadNum_);
    LOG(INFO) << Section("Create Image Saver Thread", false);
    LOG(INFO) << fmt::format("thread number: {}", saverThreadNum_);

    for (size_t i = 0; i < saverThreadNum_; ++i) {
        imageSaverThreads_.emplace_back(thread([&]() {
            while (true) {
                // taken job and check it's valid. If job queue is stop, job will be invalid, then the process finish
                auto job = imageQueue_->pop();
                if (!job.isValid()) {
                    break;
                }

                // process image
                processImg_(job.data());
            }
        }));
    }
}
