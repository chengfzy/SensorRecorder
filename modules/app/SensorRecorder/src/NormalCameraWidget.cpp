#include "NormalCameraWidget.h"
#include <fmt/format.h>
#include <glog/logging.h>
#include <opencv2/imgcodecs.hpp>
#include "NormalCameraSettingDialog.h"
#include "libra/qt.hpp"

using namespace std;
using namespace cv;
using namespace libra::util;
using namespace libra::core;
using namespace libra::io;
using namespace libra::qt;

// Constructor
NormalCameraWidget::NormalCameraWidget(QWidget* parent)
    : ISensorWidget(parent),
      saveFolder_(
          QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/camNormal").absolutePath()) {
    setupUI();
}

// Set the selected camera list
void NormalCameraWidget::setDevices(const QStringList& cameras) {
    deviceComboBox_->clear();
    deviceComboBox_->addItems(cameras);
    deviceComboBox_->setCurrentIndex(0);
}

// Set current selected camera
void NormalCameraWidget::setCurrentDevice(const size_t& index) {
    if (index < deviceComboBox_->count()) {
        deviceComboBox_->setCurrentIndex(index);
    }
}

// Set save folder
void NormalCameraWidget::setSaveFolder(const QString& folder) { saveFolderLineEdit_->setText(folder); }

// Initialize
void NormalCameraWidget::init() {
    // remove old jpg files in save folder
    QDir dir(saveFolder_);
    dir.removeRecursively();
    // create save folder if not exist, don't use `createDirIfNotExist()`, boost conflict with iDS lib
    if (!dir.mkpath(saveFolder_)) {
        LOG(ERROR) << fmt::format("cannot create folder \"{}\" to save captured image", saveFolder_.toStdString());
    }

    // reset index
    imageIndex_ = 0;
    saveIndex_ = 0;

    // create recorder
    if (recorder_) {
        recorder_->setDevice(deviceComboBox_->currentText().toStdString());
    } else {
        recorder_ = make_shared<NormalCameraRecorder>(deviceComboBox_->currentText().toStdString());
    }

    // set
    recorder_->setSaverThreadNum(saverThreadNumSpinBox_->value());

    // set process function
    recorder_->setProcessFunction([&](const ImageRecord& record) {
        // save for file
        if (captureMode_ != SensorCaptureMode::None) {
            // save file name
            string fileName;
            switch (saveFormat_) {
                case ImageSaveFormat::Kalibr:
                    fileName = fmt::format("{}/{:.0f}.jpg", saveFolder_.toStdString(), record.timestamp() * 1.0E9);
                    break;
                case ImageSaveFormat::Index: {
                    fileName = fmt::format("{}/{:06d}.jpg", saveFolder_.toStdString(), saveIndex_);
                    ++saveIndex_;
                }
                default:
                    break;
            }
            // save to file
            imwrite(fileName, record.reading());

            // change mode for next image
            if (captureMode_ == SensorCaptureMode::Once) {
                captureMode_ = SensorCaptureMode::None;
            }
        }

        // send image per 10 images
        if (0 == imageIndex_ % 10) {
            emit newImage(matToQImage(record.reading()));
        }

        ++imageIndex_;
    });

    // init
    emit newImage(QImage());  // clear image viewer before init
    recorder_->init();

    // disable UI
    enableConfigWidget(false);
    settingButton_->setEnabled(true);
    liveButton_->setEnabled(true);

    // send start signals
    emit sensorStateChanged(true);
}

// Show live image
void NormalCameraWidget::live() {
    if (isLive_) {
        // stop live show
        stopLive();
    } else {
        // start live show
        recorder_->start();

        // update UI
        liveButton_->setIcon(QIcon(":/Icon/Stop"));
        liveButton_->setStatusTip("Stop live show");
        settingButton_->setEnabled(false);
        captureButton_->setEnabled(true);
        recordButton_->setEnabled(true);

        // set flag
        isLive_ = true;
    }
}

// Capture one image
void NormalCameraWidget::capture() { captureMode_ = SensorCaptureMode::Once; }

// Record image
void NormalCameraWidget::record() {
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
void NormalCameraWidget::setupUI() {
    // camera list
    deviceComboBox_ = new QComboBox;
    deviceComboBox_->addItem("/dev/video0");  // add default device name
    deviceComboBox_->setEditable(true);
    deviceComboBox_->setCurrentIndex(0);
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
    // save folder
    saveFolderLineEdit_ = new QLineEdit(saveFolder_);
    connect(saveFolderLineEdit_, &QLineEdit::textChanged, [&](const QString& folder) { saveFolder_ = folder; });
    // select save folder
    selectFolderButton_ = new QPushButton(QIcon(":/Icon/Open"), "");
    selectFolderButton_->setStatusTip("Select folder to save camera images");
    connect(selectFolderButton_, &QPushButton::clicked, [&] {
        QString folder = QFileDialog::getExistingDirectory(this, "Select folder to save camera images",
                                                           QDir(saveFolder_).absolutePath());
        if (!folder.isEmpty()) {
            saveFolderLineEdit_->setText(folder);
        }
    });

    // config layout
    auto configLayout = new QGridLayout;
    configLayout->addWidget(new QLabel("Camera Device"), 0, 0);
    configLayout->addWidget(deviceComboBox_, 0, 1, 1, 2);
    configLayout->addWidget(new QLabel("Saver Thread Number"), 1, 0);
    configLayout->addWidget(saverThreadNumSpinBox_, 1, 1, 1, 2);
    configLayout->addWidget(new QLabel("Save Format"), 2, 0);
    configLayout->addWidget(saveFormatComboBox_, 2, 1, 1, 2);
    configLayout->addWidget(new QLabel("Save Folder"), 3, 0);
    configLayout->addWidget(saveFolderLineEdit_, 3, 1);
    configLayout->addWidget(selectFolderButton_, 3, 2);

    // init
    initButton_ = new QPushButton(QIcon(":/Icon/Init"), "");
    initButton_->setStatusTip("Initialize");
    connect(initButton_, &QPushButton::clicked, this, &NormalCameraWidget::init);
    // setting
    settingButton_ = new QPushButton(QIcon(":/Icon/Setting"), "");
    settingButton_->setStatusTip("Setting");
    settingButton_->setEnabled(false);
    connect(settingButton_, &QPushButton::clicked, [&]() {
        auto dialog = new NormalCameraSettingDialog(recorder_, this);
        dialog->setWindowTitle(QString("Normal Camera #%1").arg(deviceComboBox_->currentIndex()));
        if (dialog->exec() == QDialog::Accepted) {
            dialog->apply();
        }
    });
    // live show
    liveButton_ = new QPushButton(QIcon(":/Icon/Live"), "");
    liveButton_->setStatusTip("Start live show");
    liveButton_->setEnabled(false);
    connect(liveButton_, &QPushButton::clicked, this, &NormalCameraWidget::live);
    // capture
    captureButton_ = new QPushButton(QIcon(":/Icon/Capture"), "");
    captureButton_->setStatusTip("Capture one image");
    captureButton_->setEnabled(false);
    connect(captureButton_, &QPushButton::clicked, this, &NormalCameraWidget::capture);
    // record
    recordButton_ = new QPushButton(QIcon(":/Icon/Record"), "");
    recordButton_->setStatusTip("Record all images");
    recordButton_->setEnabled(false);
    connect(recordButton_, &QPushButton::clicked, this, &NormalCameraWidget::record);

    // control layout
    auto controlLayout = new QHBoxLayout;
    controlLayout->addWidget(initButton_);
    controlLayout->addWidget(settingButton_);
    controlLayout->addWidget(liveButton_);
    controlLayout->addWidget(captureButton_);
    controlLayout->addWidget(recordButton_);

    // main layout
    auto mainLayout = new QVBoxLayout;
    mainLayout->addLayout(configLayout);
    mainLayout->addLayout(controlLayout);
    setLayout(mainLayout);
}

// Enable config widget
void NormalCameraWidget::enableConfigWidget(bool enable) {
    deviceComboBox_->setEnabled(enable);
    saverThreadNumSpinBox_->setEnabled(enable);
    saveFormatComboBox_->setEnabled(enable);
    saveFolderLineEdit_->setEnabled(enable);
    selectFolderButton_->setEnabled(enable);
    initButton_->setEnabled(enable);
}

// Stop live show
void NormalCameraWidget::stopLive() {
    captureMode_ = SensorCaptureMode::None;

    // stop live show
    recorder_->stop();
    recorder_->wait();

    // clear image viewer
    emit newImage(QImage());

    enableConfigWidget(true);
    liveButton_->setIcon(QIcon(":/Icon/Live"));
    liveButton_->setStatusTip("Start live show");
    settingButton_->setEnabled(false);
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