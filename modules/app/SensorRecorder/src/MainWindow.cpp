#include "MainWindow.h"
#include <glog/logging.h>

using namespace std;
using namespace libra::util;
using namespace libra::io;
using namespace libra::qt;
using namespace cv;

MainWindow::MainWindow(const QString& saveRootFolder, QWidget* parent)
    : QMainWindow(parent),
      rootSaveFolder_(
          QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/data").absolutePath()) {
    setupUI();
    setMinimumWidth(1200);
    setMinimumHeight(sizeHint().height());
    setWindowIcon(QIcon(":/Icon/SensorRecorder"));
    setWindowTitle("Sensor Recorder");
}

// Set up UI
void MainWindow::setupUI() {
    // image viewer
    imageViewer_ = new ImageViewer;
    connect(imageViewer_, &ImageViewer::sourceChanged, [&](int index) { imageSourceIndex_ = index; });

#ifdef WITH_IDS
    // iDS camera
    idsCameraTabWidget_ = new CheckableTabWidget;
#endif

#ifdef WITH_MYNTEYE_DEPTH
    myntEyeTabWidget_ = new CheckableTabWidget;
#endif

    // normal camera
    normalCameraTabWidget_ = new CheckableTabWidget;

    // SparkFun IMU
    sparkFunImuTabWidget_ = new CheckableTabWidget;
    // SanChi IMU
    sanChiImuTabWidget_ = new CheckableTabWidget;

    // refresh to update device
    refresh();

    // root save folder
    rootSaveFolderLineEdit_ = new QLineEdit(rootSaveFolder_);
    connect(rootSaveFolderLineEdit_, &QLineEdit::textChanged, this, &MainWindow::setSaveFolder);
    // select root save folder
    auto selectFolderButton = new QPushButton(QIcon(":/Icon/Open"), "");
    selectFolderButton->setStatusTip("Select the root folder to save sensor data");
    connect(selectFolderButton, &QPushButton::clicked, [&] {
        QString folder = QFileDialog::getExistingDirectory(this, "Select the root folder to save sensor data",
                                                           QDir(rootSaveFolder_).absolutePath());
        if (!folder.isEmpty()) {
            rootSaveFolderLineEdit_->setText(folder);
        }
    });
    // setting layout
    auto settingLayout = new QGridLayout;
    settingLayout->addWidget(new QLabel("Save Folder"), 0, 0);
    settingLayout->addWidget(rootSaveFolderLineEdit_, 0, 1);
    settingLayout->addWidget(selectFolderButton, 0, 2);
    // setting widget
    settingWidget_ = new QWidget;
    settingWidget_->setLayout(settingLayout);

    // refresh
    refreshButton_ = new QPushButton(QIcon(":/Icon/Refresh"), "");
    refreshButton_->setStatusTip("Refresh to update sensor device");
    connect(refreshButton_, &QPushButton::clicked, this, &MainWindow::refresh);
    // init
    initButton_ = new QPushButton(QIcon(":/Icon/Init"), "");
    initButton_->setStatusTip("Initialize");
    connect(initButton_, &QPushButton::clicked, this, &MainWindow::init);
    // live show
    liveButton_ = new QPushButton(QIcon(":/Icon/Live"), "");
    liveButton_->setStatusTip("Start live show");
    liveButton_->setEnabled(false);
    connect(liveButton_, &QPushButton::clicked, this, &MainWindow::live);
    // capture
    captureButton_ = new QPushButton(QIcon(":/Icon/Capture"), "");
    captureButton_->setStatusTip("Capture one data");
    captureButton_->setEnabled(false);
    connect(captureButton_, &QPushButton::clicked, this, &MainWindow::capture);
    // record
    recordButton_ = new QPushButton(QIcon(":/Icon/Record"), "");
    recordButton_->setStatusTip("Record all data");
    recordButton_->setEnabled(false);
    connect(recordButton_, &QPushButton::clicked, this, &MainWindow::record);
    // control button layout
    auto controlButtonLayout = new QHBoxLayout;
    controlButtonLayout->addWidget(refreshButton_);
    controlButtonLayout->addWidget(initButton_);
    controlButtonLayout->addWidget(liveButton_);
    controlButtonLayout->addWidget(captureButton_);
    controlButtonLayout->addWidget(recordButton_);

    // control layout
    auto controlLayout = new QVBoxLayout;
    controlLayout->addWidget(settingWidget_);
    controlLayout->addLayout(controlButtonLayout);

    // control groupBox
    controlGroupBox_ = new QGroupBox("All Sensor Control");
    controlGroupBox_->setLayout(controlLayout);

    // sensor layout
    auto sensorLayout = new QVBoxLayout;
#ifdef WITH_IDS
    sensorLayout->addWidget(idsCameraTabWidget_);
#endif
#ifdef WITH_MYNTEYE_DEPTH
    sensorLayout->addWidget(myntEyeTabWidget_);
#endif
    sensorLayout->addWidget(normalCameraTabWidget_);
    sensorLayout->addWidget(sparkFunImuTabWidget_);
    sensorLayout->addWidget(sanChiImuTabWidget_);
    sensorLayout->addWidget(controlGroupBox_);
    sensorLayout->addStretch();
    // sensor widget
    auto sensorWidget = new QWidget;
    sensorWidget->setLayout(sensorLayout);
    sensorWidget->setMaximumWidth(500);

    // main widget
    auto mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainSplitter->addWidget(sensorWidget);
    mainSplitter->addWidget(imageViewer_);
    // mainSplitter->setStretchFactor(0, 1);
    setCentralWidget(mainSplitter);

    // add status bar
    statusBar();
}

#ifdef WITH_MYNTEYE_DEPTH
// Set the Image Source For Mynt object
void MainWindow::setImageSourceForMynt(bool isRightCameEnabled) {
    if (isRightCameEnabled) {
        singleSensor_ = false;
        auto widget = qobject_cast<MyntEyeWidget*>(sender());
        auto index = myntEyeTabWidget_->indexOf(widget);
        imageSourceWidgets_.append(widget);
        imageSourceWidgets_.append(widget);
        QStringList cameraNames;
        cameraNames.append(myntEyeTabWidget_->tabText(index) + " Left");  // set left cam name
        cameraNames.append(myntEyeTabWidget_->tabText(index) + " Right");
        imageViewer_->setSourceList(cameraNames);
    } else {
        singleSensor_ = true;
        imageSourceWidgets_.clear();
        imageViewer_->setSourceList(QStringList());
    }
}
#endif

// Show image
void MainWindow::showImage(const QImage& image) {
    // FIXME: it still received a valid image after stop in multiple sensor mode
    // show empty in below conditions:
    // 1. Single sensor mode
    // 2. Empty image
    // 3. Multiple sensor and the sender is cooresponding to the selected index
    if (singleSensor_ || image.isNull() ||
        (0 <= imageSourceIndex_ && imageSourceIndex_ < imageSourceWidgets_.size() &&
         qobject_cast<ISensorWidget*>(sender()) == imageSourceWidgets_[imageSourceIndex_])) {
        imageViewer_->setImage(image);
    }
}

// Set save folder for every sensor recorder
void MainWindow::setSaveFolder(const QString& rootSaveFolder) {
    rootSaveFolder_ = rootSaveFolder;

#ifdef WITH_IDS
    // update save folder for iDS
    for (size_t i = 0; i < idsCameraTabWidget_->count(); ++i) {
        qobject_cast<IdsCameraWidget*>(idsCameraTabWidget_->widget(i))
            ->setSaveFolder(QDir(rootSaveFolder_ + QString("/camIds%1").arg(i)).absolutePath());
    }
#endif
#ifdef WITH_MYNTEYE_DEPTH
    // update save folder for MyntEye
    for (size_t i = 0; i < myntEyeTabWidget_->count(); ++i) {
        qobject_cast<MyntEyeWidget*>(myntEyeTabWidget_->widget(i))
            ->setLeftImageSaveFolder(QDir(rootSaveFolder_ + QString("/camMyntEye%1Left").arg(i)).absolutePath());
        qobject_cast<MyntEyeWidget*>(myntEyeTabWidget_->widget(i))
            ->setRightImageSaveFolder(QDir(rootSaveFolder_ + QString("/camMyntEye%1Right").arg(i)).absolutePath());
        qobject_cast<MyntEyeWidget*>(myntEyeTabWidget_->widget(i))
            ->setImuSaveFile(QDir(rootSaveFolder_ + QString("/imuMyntEye%1.csv").arg(i)).absolutePath());
    }
#endif
    // update save folder for normal camera
    for (size_t i = 0; i < normalCameraTabWidget_->count(); ++i) {
        qobject_cast<NormalCameraWidget*>(normalCameraTabWidget_->widget(i))
            ->setSaveFolder(QDir(rootSaveFolder_ + QString("/camNormal%1").arg(i)).absolutePath());
    }

    // update save file for SparkFun IMU
    for (size_t i = 0; i < sparkFunImuTabWidget_->count(); ++i) {
        qobject_cast<SparkFunImuWidget*>(sparkFunImuTabWidget_->widget(i))
            ->setSaveFile(QDir(rootSaveFolder_ + QString("/imuSparkFun%1.csv").arg(i)).absolutePath());
    }

    // update save file for SanChi IMU
    for (size_t i = 0; i < sanChiImuTabWidget_->count(); ++i) {
        qobject_cast<SanChiImuWidget*>(sanChiImuTabWidget_->widget(i))
            ->setSaveFile(QDir(rootSaveFolder_ + QString("/imuSanChi%1.csv").arg(i)).absolutePath());
    }
}

// Enable sensor widgets except the input one
void MainWindow::enableSensorWidgetExcept(bool enable, QWidget* widget) {
    // define min function
    auto enableTabWidgetFunc = [&](CheckableTabWidget* tabWidget) {
        for (size_t i = 0; i < tabWidget->count(); ++i) {
            if (widget != tabWidget->widget(i)) {
                tabWidget->setTabEnabled(i, enable);
            } else {
                tabWidget->checkBox(i)->setEnabled(enable);
            }
        }
    };

#ifdef WITH_IDS
    enableTabWidgetFunc(idsCameraTabWidget_);
#endif
#ifdef WITH_MYNTEYE_DEPTH
    enableTabWidgetFunc(myntEyeTabWidget_);
#endif
    enableTabWidgetFunc(normalCameraTabWidget_);
    enableTabWidgetFunc(sparkFunImuTabWidget_);
    enableTabWidgetFunc(sanChiImuTabWidget_);

    // enable control groupBox
    controlGroupBox_->setEnabled(enable);
}

// Enable setting widget
void MainWindow::enableSettingWidget(bool enable) {
    // define min function
    auto enableTabWidgetFunc = [&](CheckableTabWidget* tabWidget) {
        for (size_t i = 0; i < tabWidget->count(); ++i) {
            tabWidget->setTabEnabled(i, enable);
        }
    };

#ifdef WITH_IDS
    enableTabWidgetFunc(idsCameraTabWidget_);
#endif
#ifdef WITH_MYNTEYE_DEPTH
    enableTabWidgetFunc(myntEyeTabWidget_);
#endif
    enableTabWidgetFunc(normalCameraTabWidget_);
    enableTabWidgetFunc(sparkFunImuTabWidget_);
    enableTabWidgetFunc(sanChiImuTabWidget_);

    settingWidget_->setEnabled(enable);
    refreshButton_->setEnabled(enable);
    initButton_->setEnabled(enable);
}

// Refresh to update sensor device
void MainWindow::refresh() {
#ifdef WITH_IDS
    // iDS camera
    auto idsCameras = IdsCameraRecorder::getCameraList();
    idsCameraTabWidget_->clear();
    idsCameraTabWidget_->setVisible(!idsCameras.empty());
    for (size_t i = 0; i < idsCameras.size(); ++i) {
        auto cameraWidget = new IdsCameraWidget;
        cameraWidget->setDevices(idsCameras);
        cameraWidget->setCurrentDevice(i);
        cameraWidget->setSaveFolder(QDir(rootSaveFolder_ + QString("/camIds%1").arg(i)).absolutePath());
        // connection
        connect(cameraWidget, &IdsCameraWidget::newImage, this, &MainWindow::showImage);
        connect(cameraWidget, &IdsCameraWidget::sensorStateChanged, this, [&](bool working) {
            if (singleSensor_) {
                enableSensorWidgetExcept(!working, qobject_cast<QWidget*>(sender()));
            }
        });
        // add to tab
        idsCameraTabWidget_->addTab(cameraWidget, QString("iDS Camera #%1").arg(i));
    }
#endif

#ifdef WITH_MYNTEYE_DEPTH
    // MyntEye device
    auto myntEye = MyntEyeRecorder::getDevices();
    myntEyeTabWidget_->clear();
    myntEyeTabWidget_->setVisible(!myntEye.empty());
    for (size_t i = 0; i < myntEye.size(); ++i) {
        auto myntEyeWidget = new MyntEyeWidget;
        myntEyeWidget->setDevices(myntEye);
        myntEyeWidget->setCurrentDevice(i);
        myntEyeWidget->setLeftImageSaveFolder(
            QDir(rootSaveFolder_ + QString("/camMyntEye%1Left").arg(i)).absolutePath());
        myntEyeWidget->setRightImageSaveFolder(
            QDir(rootSaveFolder_ + QString("/camMyntEye%1Right").arg(i)).absolutePath());
        myntEyeWidget->setImuSaveFile(QDir(rootSaveFolder_ + QString("/imuMyntEye%1.csv").arg(i)).absolutePath());
        // connection
        connect(myntEyeWidget, &MyntEyeWidget::rightCamEnabled, this, &MainWindow::setImageSourceForMynt);
        connect(myntEyeWidget, &MyntEyeWidget::newLeftImage, this, [&](const QImage& image) {
            // < 0 for single camera working(without right one)
            if (imageSourceIndex_ < 0 || imageSourceIndex_ % 2 == 0) {
                showImage(image);
            }
        });
        connect(myntEyeWidget, &MyntEyeWidget::newRightImage, this, [&](const QImage& image) {
            if (imageSourceIndex_ % 2 == 1) {
                showImage(image);
            }
        });
        connect(myntEyeWidget, &MyntEyeWidget::sensorStateChanged, this, [&](bool working) {
            if (singleSensor_) {
                enableSensorWidgetExcept(!working, qobject_cast<QWidget*>(sender()));
            }
        });
        // add to tab
        myntEyeTabWidget_->addTab(myntEyeWidget, QString("MyntEye Device #%1").arg(i));
    }
#endif

    // normal camera
    auto normalCameras = getDevices("video*");
    normalCameraTabWidget_->clear();
    normalCameraTabWidget_->setVisible(!normalCameras.empty());
    for (size_t i = 0; i < normalCameras.size(); ++i) {
        auto cameraWidget = new NormalCameraWidget;
        cameraWidget->setDevices(normalCameras);
        cameraWidget->setCurrentDevice(i);
        cameraWidget->setSaveFolder(QDir(rootSaveFolder_ + QString("/camNormal%1").arg(i)).absolutePath());
        // connection
        connect(cameraWidget, &NormalCameraWidget::newImage, this, &MainWindow::showImage);
        connect(cameraWidget, &NormalCameraWidget::sensorStateChanged, this, [&](bool working) {
            if (singleSensor_) {
                enableSensorWidgetExcept(!working, qobject_cast<QWidget*>(sender()));
            }
        });
        // add to tab
        normalCameraTabWidget_->addTab(cameraWidget, QString("Normal Camera #%1").arg(i));
    }

    // SparkFun IMU
    auto sparkFunImus = getDevices("ttyACM*");
    sparkFunImuTabWidget_->clear();
    sparkFunImuTabWidget_->setVisible(!sparkFunImus.empty());
    for (size_t i = 0; i < sparkFunImus.size(); ++i) {
        auto imuWidget = new SparkFunImuWidget;
        imuWidget->setDevices(sparkFunImus);
        imuWidget->setCurrentDevice(i);
        imuWidget->setSaveFile(QDir(rootSaveFolder_ + QString("/imuSparkFun%1.csv").arg(i)).absolutePath());
        // connection
        connect(imuWidget, &SparkFunImuWidget::sensorStateChanged, this, [&](bool working) {
            if (singleSensor_) {
                enableSensorWidgetExcept(!working, qobject_cast<QWidget*>(sender()));
            }
        });
        // add to tab
        sparkFunImuTabWidget_->addTab(imuWidget, QString("SparkFun IMU #%1").arg(i));
    }

    // SanChi IMU, only support one SanChi IMU
    auto sanChiImus = getDevices("ttyUSB*");
    sanChiImuTabWidget_->clear();
    sanChiImuTabWidget_->setVisible(!sanChiImus.empty());
    if (!sanChiImus.empty()) {
        auto imuWidget = new SanChiImuWidget;
        imuWidget->setDevices(sanChiImus);
        imuWidget->setCurrentDevice(0);
        imuWidget->setSaveFile(QDir(rootSaveFolder_ + "/imuSanChi.csv").absolutePath());
        // connection
        connect(imuWidget, &SanChiImuWidget::sensorStateChanged, this, [&](bool working) {
            if (singleSensor_) {
                enableSensorWidgetExcept(!working, qobject_cast<QWidget*>(sender()));
            }
        });
        // add to tab
        sanChiImuTabWidget_->addTab(imuWidget, "SanChi IMU");
    }
}

// Initialize all sensors
void MainWindow::init() {
    // remove old jpg files in save folder
    QDir dir(rootSaveFolder_);
    dir.removeRecursively();
    // create save folder if not exist, don't use `createDirIfNotExist()`, boost conflict with iDS lib
    if (!dir.mkpath(rootSaveFolder_)) {
        LOG(ERROR) << fmt::format("cannot create folder \"{}\" to save sensor data", rootSaveFolder_.toStdString());
    }

    // record for multiple sensors
    singleSensor_ = false;

    // set selected sensor widget and names
    selectCameraWidgets_.clear();
    selectImuWidgets_.clear();
    imageSourceWidgets_.clear();
    QStringList cameraNames;
    QStringList imuNames;

    // define min function
    auto initCameraFunc = [&](CheckableTabWidget* tabWidget) {
        for (size_t i = 0; i < tabWidget->count(); ++i) {
            if (tabWidget->checkBox(i)->isChecked()) {
                qobject_cast<ISensorWidget*>(tabWidget->widget(i))->init();
                selectCameraWidgets_.append(qobject_cast<ISensorWidget*>(tabWidget->widget(i)));
                imageSourceWidgets_.append(qobject_cast<ISensorWidget*>(tabWidget->widget(i)));
                cameraNames.append(tabWidget->tabText(i));
            }
        }
    };
    auto initImuFunc = [&](CheckableTabWidget* tabWidget) {
        for (size_t i = 0; i < tabWidget->count(); ++i) {
            if (tabWidget->checkBox(i)->isChecked()) {
                qobject_cast<ISensorWidget*>(tabWidget->widget(i))->init();
                selectImuWidgets_.append(qobject_cast<ISensorWidget*>(tabWidget->widget(i)));
                imuNames.append(tabWidget->tabText(i));
            }
        }
    };

    // init selected sensors
#ifdef WITH_IDS
    initCameraFunc(idsCameraTabWidget_);
#endif

#ifdef WITH_MYNTEYE_DEPTH
    // init myntEye camera
    for (size_t i = 0; i < myntEyeTabWidget_->count(); ++i) {
        if (myntEyeTabWidget_->checkBox(i)->isChecked()) {
            qobject_cast<ISensorWidget*>(myntEyeTabWidget_->widget(i))->init();
            selectCameraWidgets_.append(qobject_cast<ISensorWidget*>(myntEyeTabWidget_->widget(i)));

            // update image source widget and correconding names
            imageSourceWidgets_.append(qobject_cast<ISensorWidget*>(myntEyeTabWidget_->widget(i)));
            if (qobject_cast<MyntEyeWidget*>(myntEyeTabWidget_->widget(i))->isRightCamEnabled()) {
                cameraNames.append(myntEyeTabWidget_->tabText(i) + " Left");  // set left cam name
                imageSourceWidgets_.append(qobject_cast<ISensorWidget*>(myntEyeTabWidget_->widget(i)));
                cameraNames.append(myntEyeTabWidget_->tabText(i) + " Right");
            } else {
                cameraNames.append(myntEyeTabWidget_->tabText(i));  // set left cam name
            }
        }
    }
#endif

    initCameraFunc(normalCameraTabWidget_);
    initImuFunc(sparkFunImuTabWidget_);
    initImuFunc(sanChiImuTabWidget_);

    // update UI
    enableSettingWidget(false);
    liveButton_->setEnabled(true);
    imageViewer_->setSourceList(cameraNames);
}

// Live show
void MainWindow::live() {
    // live show selected sensors
    for (auto& v : selectCameraWidgets_) {
        v->live();
    }
    for (auto& v : selectImuWidgets_) {
        v->live();
    }

    // update UI
    if (isLive_) {
        stopLive();
    } else {
        liveButton_->setIcon(QIcon(":/Icon/Stop"));
        liveButton_->setStatusTip("Stop live show");
        captureButton_->setEnabled(true);
        recordButton_->setEnabled(true);

        // set flag
        isLive_ = true;
    }
}

// Stop live show
void MainWindow::stopLive() {
    // update UI
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

    // clear source names after stop
    selectCameraWidgets_.clear();
    selectImuWidgets_.clear();
    imageSourceWidgets_.clear();
    imageViewer_->setSourceList(QStringList());

    // reset for single sensor
    singleSensor_ = true;
}

// Capture one reading from all selected sensors
void MainWindow::capture() {
    for (auto& v : selectCameraWidgets_) {
        v->capture();
    }
    for (auto& v : selectImuWidgets_) {
        v->capture();
    }
}

// Record all readings from all selected sensors
void MainWindow::record() {
    for (auto& v : selectCameraWidgets_) {
        v->record();
    }
    for (auto& v : selectImuWidgets_) {
        v->record();
    }

    // update UI
    if (isRecord_) {
        stopLive();
    } else {
        recordButton_->setIcon(QIcon(":/Icon/Stop"));
        recordButton_->setStatusTip("Stop record images");
        captureButton_->setEnabled(false);

        // set flag
        isRecord_ = true;
    }
}

// Get the matched files(with same prefix) in directory
QStringList MainWindow::getDevices(const QString& prefix) {
    QDir dir("/dev");
    auto deviceNames = dir.entryInfoList({prefix}, QDir::System);
    QStringList devices;
    for (auto& v : deviceNames) {
        devices.append(v.absoluteFilePath());
    }
    return devices;
}