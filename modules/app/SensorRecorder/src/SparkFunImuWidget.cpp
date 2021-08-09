#include "SparkFunImuWidget.h"
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

// declare type
Q_DECLARE_METATYPE(TimestampRetrieveMethod);

// Constructor
SparkFunImuWidget::SparkFunImuWidget(QWidget* parent)
    : ISensorWidget(parent),
      saveFile_(QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/imuSparkFun.csv")
                    .absolutePath()) {
    setupUI();
}

// Set the IMU device list
void SparkFunImuWidget::setDevices(const QStringList& devices) {
    deviceComboBox_->clear();
    deviceComboBox_->addItems(devices);
    deviceComboBox_->setCurrentIndex(0);
}

// Set current selected IMU device
void SparkFunImuWidget::setCurrentDevice(const size_t& index) {
    if (index < deviceComboBox_->count()) {
        deviceComboBox_->setCurrentIndex(index);
    }
}

// Set save file
void SparkFunImuWidget::setSaveFile(const QString& file) { saveFileLineEdit_->setText(file); }

// Initialize
void SparkFunImuWidget::init() {
    // create recorder
    if (recorder_) {
        recorder_->setDevice(deviceComboBox_->currentText().toStdString());
    } else {
        recorder_ = make_shared<SparkFunImuRecorder>(deviceComboBox_->currentText().toStdString());
    }

    // create save folder if not exist
    QDir dir;
    QFileInfo fileInfo(saveFile_);
    LOG_IF(ERROR, !dir.mkpath(fileInfo.absoluteDir().path()))
        << fmt::format("cannot create folder \"{}\" to save IMU data", fileInfo.absoluteDir().path().toStdString());

    // open file when started
    recorder_->addCallback(SparkFunImuRecorder::CallBackStarted, [&] {
        fileStream_.open(saveFile_.toStdString(), ios::out);
        CHECK(fileStream_.is_open()) << fmt::format("cannot open file \"{}\" to save IMU data",
                                                    saveFile_.toStdString());
        // write header
        fileStream_
            << "# timestamp(ns), gyro X(rad/s), gyro Y(rad/s), gyro Z(rad/s), acc X(m/s^2), acc Y(m/s^2), acc Z(m/s^2)"
            << endl;
    });
    // close file when finished
    recorder_->addCallback(SparkFunImuRecorder::CallBackFinished, [&] { fileStream_.close(); });

    // set process funcion
    recorder_->setProcessFunction([&](const ImuRecord& imu) {
        if (captureMode_ != SensorCaptureMode::None) {
            // format: timestamp(ns), gyro(rad/s), acc(m/s^2)
            fileStream_ << fmt::format("{:.0f},{:.10f},{:.10f},{:.10f},{:.10f},{:.10f},{:.10f}",
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

    // set
    recorder_->setTimeStampRetrieveMethod(timestampMethodComboBox_->currentData().value<TimestampRetrieveMethod>());

    // init
    recorder_->init();

    // disable UI
    enableSettingWidget(false);
    liveButton_->setEnabled(true);

    // send start signals
    emit sensorStateChanged(true);
}

// Show live IMU reading
void SparkFunImuWidget::live() {
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

// Capture one IMU reading
void SparkFunImuWidget::capture() { captureMode_ = SensorCaptureMode::Once; }

// Record IMU reading
void SparkFunImuWidget::record() {
    if (isRecord_) {
        // stop record readings
        stopLive();
    } else {
        // start record readings
        captureMode_ = SensorCaptureMode::All;

        // update UI
        recordButton_->setIcon(QIcon(":/Icon/Stop"));
        recordButton_->setStatusTip("Stop record readings");
        captureButton_->setEnabled(false);

        // set flag
        isRecord_ = true;
    }
}

// Setup UI
void SparkFunImuWidget::setupUI() {
    // device
    deviceComboBox_ = new QComboBox;
    deviceComboBox_->addItem("/dev/ttyACM0");  // add default device name
    deviceComboBox_->setEditable(true);
    deviceComboBox_->setCurrentIndex(0);
    // timestamp retrieve method
    timestampMethodComboBox_ = new QComboBox;
    timestampMethodComboBox_->addItem("From Sensor", QVariant::fromValue(TimestampRetrieveMethod::Sensor));
    timestampMethodComboBox_->addItem("From Host", QVariant::fromValue(TimestampRetrieveMethod::Host));
    timestampMethodComboBox_->setCurrentIndex(0);
    // save file
    saveFileLineEdit_ = new QLineEdit(saveFile_);
    connect(saveFileLineEdit_, &QLineEdit::textChanged, [&](const QString& file) { saveFile_ = file; });
    // select save file
    selectFileButton_ = new QPushButton(QIcon(":/Icon/Open"), "");
    selectFileButton_->setStatusTip("Select file to save IMU data");
    connect(selectFileButton_, &QPushButton::clicked, [&] {
        QString file =
            QFileDialog::getSaveFileName(this, "Select file to save IMU data", QDir(saveFile_).absolutePath(),
                                         "CSV files (*.csv);;Text files (*.txt)");
        if (!file.isEmpty()) {
            saveFileLineEdit_->setText(file);
        }
    });

    // setting layout
    auto settingLayout = new QGridLayout;
    settingLayout->addWidget(new QLabel("Device Name"), 0, 0);
    settingLayout->addWidget(deviceComboBox_, 0, 1, 1, 2);
    settingLayout->addWidget(new QLabel("Timestamp Retrieve Method"), 1, 0);
    settingLayout->addWidget(timestampMethodComboBox_, 1, 1, 1, 2);
    settingLayout->addWidget(new QLabel("Save File"), 2, 0);
    settingLayout->addWidget(saveFileLineEdit_, 2, 1);
    settingLayout->addWidget(selectFileButton_, 2, 2);

    // init IMU button
    initButton_ = new QPushButton(QIcon(":/Icon/Init"), "");
    initButton_->setStatusTip("Initialize");
    connect(initButton_, &QPushButton::clicked, this, &SparkFunImuWidget::init);
    // live show
    liveButton_ = new QPushButton(QIcon(":/Icon/Live"), "");
    liveButton_->setStatusTip("Start live show");
    liveButton_->setEnabled(false);
    connect(liveButton_, &QPushButton::clicked, this, &SparkFunImuWidget::live);
    // capture
    captureButton_ = new QPushButton(QIcon(":/Icon/Capture"), "");
    captureButton_->setStatusTip("Capture one reading");
    captureButton_->setEnabled(false);
    connect(captureButton_, &QPushButton::clicked, this, &SparkFunImuWidget::capture);
    // record
    recordButton_ = new QPushButton(QIcon(":/Icon/Record"), "");
    recordButton_->setStatusTip("Record all readings");
    recordButton_->setEnabled(false);
    connect(recordButton_, &QPushButton::clicked, this, &SparkFunImuWidget::record);

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
void SparkFunImuWidget::enableSettingWidget(bool enable) {
    deviceComboBox_->setEnabled(enable);
    saveFileLineEdit_->setEnabled(enable);
    selectFileButton_->setEnabled(enable);
    initButton_->setEnabled(enable);
}

// Stop live show
void SparkFunImuWidget::stopLive() {
    captureMode_ = SensorCaptureMode::None;

    // stop live show
    recorder_->stop();
    recorder_->wait();

    enableSettingWidget(true);
    liveButton_->setIcon(QIcon(":/Icon/Live"));
    liveButton_->setStatusTip("Start live show");
    liveButton_->setEnabled(false);
    captureButton_->setEnabled(false);
    recordButton_->setIcon(QIcon(":/Icon/Record"));
    recordButton_->setStatusTip("Record all readings");
    recordButton_->setEnabled(false);

    // reset flag
    isLive_ = false;
    isRecord_ = false;

    // send start signals
    emit sensorStateChanged(false);
}