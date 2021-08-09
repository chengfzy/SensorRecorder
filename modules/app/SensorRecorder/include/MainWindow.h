#pragma once
#include <QtWidgets>
#include <fstream>
#include <memory>
#include "NormalCameraWidget.h"
#include "SanChiImuWidget.h"
#include "SensorCaptureMode.hpp"
#include "SparkFunImuWidget.h"
#include "libra/io.hpp"
#include "libra/qt.hpp"
#ifdef WITH_IDS
#include "IdsCameraWidget.h"
#endif
#ifdef WITH_MYNTEYE_DEPTH
#include "MyntEyeWidget.h"
#endif

/**
 * @brief Main window for IMU camera recorder
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    /**
     * @brief Construct main window with root save folder and parent widget
     * @param saveRootFolder    Root folder path for save data
     * @param parent            Parent widget
     */
    explicit MainWindow(
        const QString& saveRootFolder = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "./data",
        QWidget* parent = nullptr);

    ~MainWindow() override = default;

  private:
    /**
     * @brief Set up UI
     */
    void setupUI();

#ifdef WITH_MYNTEYE_DEPTH
    /**
     * @brief Set the Image Source For Mynt object
     *
     * @param isRightCameEnabled  Is right camera enabled
     */
    void setImageSourceForMynt(bool isRightCameEnabled);
#endif

    /**
     * @brief Show image
     *
     * @param image Image
     */
    void showImage(const QImage& image);

    /**
     * @brief Set save folder for every sensor recorder
     *
     * @param rootSaveFolder  Root saver folder
     */
    void setSaveFolder(const QString& rootSaveFolder);

    /**
     * @brief Enable sensor widgets except the input one
     *
     * @param enable True for enable all sensor widgets, false for disable sensor widgets
     * @param widget The excepted sensor widget
     */
    void enableSensorWidgetExcept(bool enable, QWidget* widget = nullptr);

    /**
     * @brief Enable setting widget
     *
     * @param enable  True for enable, false for disable
     */
    void enableSettingWidget(bool enable);

    /**
     * @brief Refresh to update sensor device
     */
    void refresh();

    /**
     * @brief Initialize all sensors
     */
    void init();

    /**
     * @brief Live show
     */
    void live();

    /**
     * @brief Stop live show
     */
    void stopLive();

    /**
     * @brief Capture one reading from all selected sensors
     */
    void capture();

    /**
     * @brief Record all readings from all selected sensors
     */
    void record();

    /**
     * @brief Get the matched device names(with same prefix) in system
     *
     * @param prefix    Searched prefix
     * @return The searched device names
     */
    QStringList getDevices(const QString& prefix);

  private:
    libra::qt::ImageViewer* imageViewer_ = nullptr;  // image viewer
    QString rootSaveFolder_;                         // root save folder

#ifdef WITH_IDS
    libra::qt::CheckableTabWidget* idsCameraTabWidget_ = nullptr;  // iDS camera tabWidget
#endif
#ifdef WITH_MYNTEYE_DEPTH
    libra::qt::CheckableTabWidget* myntEyeTabWidget_ = nullptr;  // MyntEye tabwidget
#endif
    libra::qt::CheckableTabWidget* normalCameraTabWidget_ = nullptr;  // normal camera tabWidget
    libra::qt::CheckableTabWidget* sparkFunImuTabWidget_ = nullptr;   // SparkFun IMU tabWidget
    libra::qt::CheckableTabWidget* sanChiImuTabWidget_ = nullptr;     // SanChi IMU tabWidget

    QLineEdit* rootSaveFolderLineEdit_ = nullptr;  // root save folder line edit
    QPushButton* refreshButton_ = nullptr;         // refresh sensor button
    QPushButton* initButton_ = nullptr;            // init all sensor button
    QPushButton* liveButton_ = nullptr;            // live show all sensor button
    QPushButton* captureButton_ = nullptr;         // capture all sensor button
    QPushButton* recordButton_ = nullptr;          // record all sensor button
    QWidget* settingWidget_ = nullptr;             // setting widget
    QGroupBox* controlGroupBox_ = nullptr;         // control groupBox

    QVector<ISensorWidget*> selectCameraWidgets_;  // selected camera widget
    QVector<ISensorWidget*> selectImuWidgets_;     // selected IMU widget
    QVector<ISensorWidget*> imageSourceWidgets_;   // image source camera widget, the candidates for show image
    int imageSourceIndex_ = 0;                     // image source index, used for show image
    bool singleSensor_ = true;                     // record for single sensor
    bool isLive_ = false;                          // flag to indict whether all sensors is in live mode
    bool isRecord_ = false;                        // flag to indict whether all sensors is in record mode
};
