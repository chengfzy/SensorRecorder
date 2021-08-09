#include "MyntEyeWidget.h"
#include <fmt/format.h>
#include <glog/logging.h>
#include <opencv2/imgcodecs.hpp>
#include "libra/qt.hpp"

using namespace std;
using namespace cv;
using namespace libra::util;
using namespace libra::core;
using namespace libra::io;
using namespace libra::qt;
using namespace mynteyed;

// declare type
Q_DECLARE_METATYPE(TimestampRetrieveMethod);
Q_DECLARE_METATYPE(StreamMode);

// Constructor
MyntEyeWidget::MyntEyeWidget(QWidget* parent)
    : ISensorWidget(parent),
      leftImgSaveFolder_(
          QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/camMyntEyeLeft").absolutePath()),
      rightImgSaveFolder_(QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/camMyntEyeRight")
                              .absolutePath()),
      imuSaveFile_(QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/imuMyntEye.csv")
                       .absolutePath()) {
    setupUI();
}

// Is the right camera is enabled
bool MyntEyeWidget::isRightCamEnabled() const { return recorder_ && recorder_->isRightCamEnabled(); }

// Set the selected camera list
void MyntEyeWidget::setDevices(const vector<pair<unsigned int, string>>& cameras) {
    deviceComboBox_->clear();
    for (auto& v : cameras) {
        deviceComboBox_->addItem(QString::fromStdString(v.second), QVariant(v.first));
    }
    deviceComboBox_->setCurrentIndex(0);
}

// Set current selected camera
void MyntEyeWidget::setCurrentDevice(const size_t& index) {
    if (index < deviceComboBox_->count()) {
        deviceComboBox_->setCurrentIndex(index);
    }
}

// Set left image save folder
void MyntEyeWidget::setLeftImageSaveFolder(const QString& folder) { leftImgSaveFolderLineEdit_->setText(folder); }

// Set right image save folder
void MyntEyeWidget::setRightImageSaveFolder(const QString& folder) { rightImgSaveFolderLineEdit_->setText(folder); }

// Set IMU save file
void MyntEyeWidget::setImuSaveFile(const QString& file) { imuSaveFileLineEdit_->setText(file); }

// Initialize
void MyntEyeWidget::init() {
    // reset index
    leftImageIndex_ = 0;
    leftSaveIndex_ = 0;
    rightImageIndex_ = 0;
    rightSaveIndex_ = 0;

    // create recorder
    if (recorder_) {
        recorder_->setDeviceIndex(deviceComboBox_->currentData().toUInt());
    } else {
        recorder_ = make_shared<MyntEyeRecorder>(deviceComboBox_->currentData().toUInt());
    }

    // set
    recorder_->setFrameRate(frameRateSpinBox_->value());
    recorder_->setStreamMode(streamModeComboBox_->currentData().value<StreamMode>());
    recorder_->setSaverThreadNum(saverThreadNumSpinBox_->value());
    recorder_->setTimeStampRetrieveMethod(timestampMethodComboBox_->currentData().value<TimestampRetrieveMethod>());

    // init
    emit newLeftImage(QImage());  // clear image viewer before init
    emit newRightImage(QImage());
    recorder_->init();

    // create save folder before run
    recorder_->addCallback(MyntEyeRecorder::CallBackStarted, [&]() {
        // remove old jpg files in left save folder
        QDir dirLeft(leftImgSaveFolder_);
        dirLeft.removeRecursively();

        // create left save folder if not exist
        if (!dirLeft.mkpath(leftImgSaveFolder_)) {
            LOG(ERROR) << fmt::format("cannot create folder \"{}\" to save captured image",
                                      leftImgSaveFolder_.toStdString());
        }

        // create right save folder
        if (recorder_->isRightCamEnabled()) {
            // remove old jpg files in right save folder
            QDir dirRight(rightImgSaveFolder_);
            dirRight.removeRecursively();

            // create right save folder if not exist
            if (!dirRight.mkpath(rightImgSaveFolder_)) {
                LOG(ERROR) << fmt::format("cannot create folder \"{}\" to save captured image",
                                          rightImgSaveFolder_.toStdString());
            }
        }

        // create IMU save folder if not exist
        QDir dir;
        QFileInfo fileInfo(imuSaveFile_);
        LOG_IF(ERROR, !dir.mkpath(fileInfo.absoluteDir().path()))
            << fmt::format("cannot create folder \"{}\" to save IMU data", fileInfo.absoluteDir().path().toStdString());

        // open IMU file
        imuFileStream_.open(imuSaveFile_.toStdString(), ios::out);
        CHECK(imuFileStream_.is_open()) << fmt::format("cannot open file \"{}\" to save IMU data",
                                                       imuSaveFile_.toStdString());
        // write header
        imuFileStream_
            << "# timestamp(ns), gyro X(rad/s), gyro Y(rad/s), gyro Z(rad/s), acc X(m/s^2), acc Y(m/s^2), acc Z(m/s^2)"
            << endl;
    });

    // close file when finished
    recorder_->addCallback(MyntEyeRecorder::CallBackFinished, [&] { imuFileStream_.close(); });

    // set process function for left camera
    recorder_->setProcessFunction([&](const RawImageRecord& raw) {
        // save to file
        if (captureMode_ != SensorCaptureMode::None) {
            // save file name
            string fileName;
            switch (saveFormat_) {
                case ImageSaveFormat::Kalibr:
                    fileName = fmt::format("{}/{:.0f}.jpg", leftImgSaveFolder_.toStdString(), raw.timestamp() * 1E9);
                    break;
                case ImageSaveFormat::Index: {
                    fileName = fmt::format("{}/{:06d}.jpg", leftImgSaveFolder_.toStdString(), leftSaveIndex_);
                    ++leftSaveIndex_;
                }
                default:
                    break;
            }
            // save to file
            fstream fs(fileName, ios::out | ios::binary);
            if (!fs.is_open()) {
                LOG(ERROR) << fmt::format("cannot create file \"{}\"", fileName);
            }
            fs.write(reinterpret_cast<const char*>(raw.reading().buffer()), raw.reading().size());
            fs.close();

            // change mode for next image
            if (captureMode_ == SensorCaptureMode::Once) {
                captureMode_ = SensorCaptureMode::None;
            }
        }

        // send image per 10 images
        if (0 == leftImageIndex_ % 10) {
            Mat buf(1, raw.reading().size(), CV_8UC1, (void*)raw.reading().buffer());
            emit newLeftImage(matToQImage(imdecode(buf, IMREAD_UNCHANGED)));
        }

        ++leftImageIndex_;
    });

    // set process function for right camera
    if (recorder_->isRightCamEnabled()) {
        emit rightCamEnabled(true);

        // set process function, for right camera
        recorder_->setRightProcessFunction([&](const RawImageRecord& raw) {
            // save to file
            if (captureMode_ != SensorCaptureMode::None) {
                // save file name
                string fileName;
                switch (saveFormat_) {
                    case ImageSaveFormat::Kalibr:
                        fileName =
                            fmt::format("{}/{:.0f}.jpg", rightImgSaveFolder_.toStdString(), raw.timestamp() * 1.0E9);
                        break;
                    case ImageSaveFormat::Index: {
                        fileName = fmt::format("{}/{:06d}.jpg", rightImgSaveFolder_.toStdString(), rightSaveIndex_);
                        ++rightSaveIndex_;
                    }
                    default:
                        break;
                }
                // save to file
                fstream fs(fileName, ios::out | ios::binary);
                if (!fs.is_open()) {
                    LOG(ERROR) << fmt::format("cannot create file \"{}\"", fileName);
                }
                fs.write(reinterpret_cast<const char*>(raw.reading().buffer()), raw.reading().size());
                fs.close();

                // change mode for next image
                if (captureMode_ == SensorCaptureMode::Once) {
                    captureMode_ = SensorCaptureMode::None;
                }
            }

            // send image per 10 images
            if (0 == rightImageIndex_ % 10) {
                Mat buf(1, raw.reading().size(), CV_8UC1, (void*)raw.reading().buffer());
                emit newRightImage(matToQImage(imdecode(buf, IMREAD_UNCHANGED)));
            }

            ++rightImageIndex_;
        });
    }

    // set process funcion for IMU
    recorder_->setProcessFunction([&](const ImuRecord& imu) {
        if (captureMode_ != SensorCaptureMode::None) {
            // format: timestamp(ns), gyro(rad/s), acc(m/s^2)
            imuFileStream_ << fmt::format("{:.0f},{:.10f},{:.10f},{:.10f},{:.10f},{:.10f},{:.10f}",
                                          imu.timestamp() * 1.0E9, imu.reading().gyro()[0], imu.reading().gyro()[1],
                                          imu.reading().gyro()[2], imu.reading().acc()[0], imu.reading().acc()[1],
                                          imu.reading().acc()[2])
                           << endl;
        }

        // change mode for next reading
        if (captureMode_ == SensorCaptureMode::Once) {
            captureMode_ = SensorCaptureMode::None;
        }
    });

    // disable UI
    enableSettingWidget(false);
    liveButton_->setEnabled(true);

    // send start signals
    emit sensorStateChanged(true);
}

// Show live image
void MyntEyeWidget::live() {
    if (isLive_) {
        // stop live show
        stopLive();
    } else {
        // start live show
        recorder_->start();

        // update UI
        liveButton_->setIcon(QIcon(":/Icon/Stop"));
        liveButton_->setStatusTip("Stop live show");
        captureButton_->setEnabled(true);
        recordButton_->setEnabled(true);

        // set flag
        isLive_ = true;
    }
}

// Capture one image
void MyntEyeWidget::capture() { captureMode_ = SensorCaptureMode::Once; }

// Record image
void MyntEyeWidget::record() {
    if (isRecord_) {
        // stop record images
        stopLive();
    } else {
        // start record images
        captureMode_ = SensorCaptureMode::All;

        // update UI
        recordButton_->setIcon(QIcon(":/Icon/Stop"));
        recordButton_->setStatusTip("Stop record images");
        captureButton_->setEnabled(false);

        // set flag
        isRecord_ = true;
    }
}

// Setup UI
void MyntEyeWidget::setupUI() {
    // camera list
    deviceComboBox_ = new QComboBox;
    deviceComboBox_->addItem("0");  // add default device name, only the ID
    deviceComboBox_->setEditable(false);
    deviceComboBox_->setCurrentIndex(0);
    // frame rate
    frameRateSpinBox_ = new QSpinBox;
    frameRateSpinBox_->setRange(0, 100);
    frameRateSpinBox_->setValue(30);
    // stream mode
    streamModeComboBox_ = new QComboBox;
    streamModeComboBox_->addItem("640x480 Left", QVariant::fromValue(StreamMode::STREAM_640x480));
    streamModeComboBox_->addItem("1280x480 Left+Right", QVariant::fromValue(StreamMode::STREAM_1280x480));
    streamModeComboBox_->addItem("1280x720 Left", QVariant::fromValue(StreamMode::STREAM_1280x720));
    streamModeComboBox_->addItem("2560x720 Left+Right", QVariant::fromValue(StreamMode::STREAM_2560x720));
    streamModeComboBox_->setCurrentIndex(3);
    connect(streamModeComboBox_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [&](int) {
        auto streamMode = streamModeComboBox_->currentData().value<StreamMode>();
        bool enableRight = streamMode == StreamMode::STREAM_1280x480 || streamMode == StreamMode::STREAM_2560x720 ||
                           streamMode == StreamMode::STREAM_MODE_LAST;
        rightImgSaveFolderLineEdit_->setEnabled(enableRight);
        selectRightFolderButton_->setEnabled(enableRight);
    });
    // saver thread number
    saverThreadNumSpinBox_ = new QSpinBox;
    saverThreadNumSpinBox_->setRange(1, 5);
    saverThreadNumSpinBox_->setValue(2);
    // save format
    saveFormatComboBox_ = new QComboBox;
    saveFormatComboBox_->addItem("Kalibr", QVariant::fromValue(ImageSaveFormat::Kalibr));
    saveFormatComboBox_->addItem("Index", QVariant::fromValue(ImageSaveFormat::Index));
    saveFormatComboBox_->setCurrentText(0);
    connect(saveFormatComboBox_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            [&](int) { saveFormat_ = saveFormatComboBox_->currentData().value<ImageSaveFormat>(); });
    // timestamp retrieve method
    timestampMethodComboBox_ = new QComboBox;
    timestampMethodComboBox_->addItem("From Sensor", QVariant::fromValue(TimestampRetrieveMethod::Sensor));
    timestampMethodComboBox_->addItem("From Host", QVariant::fromValue(TimestampRetrieveMethod::Host));
    timestampMethodComboBox_->setCurrentIndex(0);
    // left image save folder
    leftImgSaveFolderLineEdit_ = new QLineEdit(leftImgSaveFolder_);
    connect(leftImgSaveFolderLineEdit_, &QLineEdit::textChanged,
            [&](const QString& folder) { leftImgSaveFolder_ = folder; });
    // select left image save folder
    selectLeftFolderButton_ = new QPushButton(QIcon(":/Icon/Open"), "");
    selectLeftFolderButton_->setStatusTip("Select folder to save camera images");
    connect(selectLeftFolderButton_, &QPushButton::clicked, [&] {
        QString folder = QFileDialog::getExistingDirectory(this, "Select folder to save left camera images",
                                                           QDir(leftImgSaveFolder_).absolutePath());
        if (!folder.isEmpty()) {
            leftImgSaveFolderLineEdit_->setText(folder);
        }
    });
    // right image save folder
    rightImgSaveFolderLineEdit_ = new QLineEdit(rightImgSaveFolder_);
    connect(rightImgSaveFolderLineEdit_, &QLineEdit::textChanged,
            [&](const QString& folder) { rightImgSaveFolder_ = folder; });
    // select right image save folder
    selectRightFolderButton_ = new QPushButton(QIcon(":/Icon/Open"), "");
    selectRightFolderButton_->setStatusTip("Select folder to save right camera images");
    connect(selectRightFolderButton_, &QPushButton::clicked, [&] {
        QString folder = QFileDialog::getExistingDirectory(this, "Select folder to save right camera images",
                                                           QDir(rightImgSaveFolder_).absolutePath());
        if (!folder.isEmpty()) {
            rightImgSaveFolderLineEdit_->setText(folder);
        }
    });
    // IMU save file
    imuSaveFileLineEdit_ = new QLineEdit(imuSaveFile_);
    connect(imuSaveFileLineEdit_, &QLineEdit::textChanged, [&](const QString& file) { imuSaveFile_ = file; });
    // select IMU save file
    selectImuFileButton_ = new QPushButton(QIcon(":/Icon/Open"), "");
    selectImuFileButton_->setStatusTip("Select file to save IMU data");
    connect(selectImuFileButton_, &QPushButton::clicked, [&] {
        QString file =
            QFileDialog::getSaveFileName(this, "Select file to save IMU data", QDir(imuSaveFile_).absolutePath(),
                                         "CSV files (*.csv);;Text files (*.txt)");
        if (!file.isEmpty()) {
            imuSaveFileLineEdit_->setText(file);
        }
    });

    // setting layout
    auto settingLayout = new QGridLayout;
    settingLayout->addWidget(new QLabel("Camera Device"), 0, 0);
    settingLayout->addWidget(deviceComboBox_, 0, 1, 1, 2);
    settingLayout->addWidget(new QLabel("Frame Rate(Hz)"), 1, 0);
    settingLayout->addWidget(frameRateSpinBox_, 1, 1, 1, 2);
    settingLayout->addWidget(new QLabel("Stream Mode"), 2, 0);
    settingLayout->addWidget(streamModeComboBox_, 2, 1, 1, 2);
    settingLayout->addWidget(new QLabel("Saver Thread Number"), 3, 0);
    settingLayout->addWidget(saverThreadNumSpinBox_, 3, 1, 1, 2);
    settingLayout->addWidget(new QLabel("Save Format"), 4, 0);
    settingLayout->addWidget(saveFormatComboBox_, 4, 1, 1, 2);
    settingLayout->addWidget(new QLabel("Timestamp Retrieve Method"), 5, 0);
    settingLayout->addWidget(timestampMethodComboBox_, 5, 1, 1, 2);
    settingLayout->addWidget(new QLabel("Left Camera Save Folder"), 6, 0);
    settingLayout->addWidget(leftImgSaveFolderLineEdit_, 6, 1);
    settingLayout->addWidget(selectLeftFolderButton_, 6, 2);
    settingLayout->addWidget(new QLabel("Right Camera Save Folder"), 7, 0);
    settingLayout->addWidget(rightImgSaveFolderLineEdit_, 7, 1);
    settingLayout->addWidget(selectRightFolderButton_, 7, 2);
    settingLayout->addWidget(new QLabel("IMU Save File"), 8, 0);
    settingLayout->addWidget(imuSaveFileLineEdit_, 8, 1);
    settingLayout->addWidget(selectImuFileButton_, 8, 2);

    // init camera button
    initButton_ = new QPushButton(QIcon(":/Icon/Init"), "");
    initButton_->setStatusTip("Initialize");
    connect(initButton_, &QPushButton::clicked, this, &MyntEyeWidget::init);
    // live show
    liveButton_ = new QPushButton(QIcon(":/Icon/Live"), "");
    liveButton_->setStatusTip("Start live show");
    liveButton_->setEnabled(false);
    connect(liveButton_, &QPushButton::clicked, this, &MyntEyeWidget::live);
    // capture
    captureButton_ = new QPushButton(QIcon(":/Icon/Capture"), "");
    captureButton_->setStatusTip("Capture one image");
    captureButton_->setEnabled(false);
    connect(captureButton_, &QPushButton::clicked, this, &MyntEyeWidget::capture);
    // record
    recordButton_ = new QPushButton(QIcon(":/Icon/Record"), "");
    recordButton_->setStatusTip("Record all images");
    recordButton_->setEnabled(false);
    connect(recordButton_, &QPushButton::clicked, this, &MyntEyeWidget::record);

    // control layout
    auto controlLayout = new QHBoxLayout;
    controlLayout->addWidget(initButton_);
    controlLayout->addWidget(liveButton_);
    controlLayout->addWidget(captureButton_);
    controlLayout->addWidget(recordButton_);

    // main layout
    auto mainLayout = new QVBoxLayout;
    mainLayout->addLayout(settingLayout);
    mainLayout->addLayout(controlLayout);
    setLayout(mainLayout);
}

// Enable setting widget
void MyntEyeWidget::enableSettingWidget(bool enable) {
    deviceComboBox_->setEnabled(enable);
    frameRateSpinBox_->setEnabled(enable);
    streamModeComboBox_->setEnabled(enable);
    saverThreadNumSpinBox_->setEnabled(enable);
    saveFormatComboBox_->setEnabled(enable);
    timestampMethodComboBox_->setEnabled(enable);
    leftImgSaveFolderLineEdit_->setEnabled(enable);
    selectLeftFolderButton_->setEnabled(enable);
    rightImgSaveFolderLineEdit_->setEnabled(enable);
    selectRightFolderButton_->setEnabled(enable);
    imuSaveFileLineEdit_->setEnabled(enable);
    selectImuFileButton_->setEnabled(enable);
    initButton_->setEnabled(enable);
}

// Stop live show
void MyntEyeWidget::stopLive() {
    captureMode_ = SensorCaptureMode::None;

    // stop live show
    recorder_->stop();
    recorder_->wait();

    // disable right camera
    emit rightCamEnabled(false);

    // clear image viewer
    emit newLeftImage(QImage());
    emit newRightImage(QImage());

    enableSettingWidget(true);
    liveButton_->setIcon(QIcon(":/Icon/Live"));
    liveButton_->setStatusTip("Start live show");
    liveButton_->setEnabled(false);
    captureButton_->setEnabled(false);
    recordButton_->setIcon(QIcon(":/Icon/Record"));
    recordButton_->setStatusTip("Record all images");
    recordButton_->setEnabled(false);

    // reset flag
    isLive_ = false;
    isRecord_ = false;

    // send signals
    emit sensorStateChanged(false);
}