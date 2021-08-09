#pragma once
#include <QtWidgets>
#include <memory>
#include "ISensorWidget.hpp"
#include "SensorCaptureMode.hpp"
#include "libra/io/SparkFunImuRecorder.h"

/**
 * @brief SparkFun IMU widget
 */
class SparkFunImuWidget : public ISensorWidget {
    Q_OBJECT

  public:
    /**
     * @brief Constructor
     *
     * @param parent  parent widget
     */
    explicit SparkFunImuWidget(QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~SparkFunImuWidget() override = default;

  signals:
    /**
     * @brief Signal if sensor working state is changed
     *
     * @param working True for sensor is working, otherwise return false
     */
    void sensorStateChanged(bool working);

  public:
    /**
     * @brief Set save file
     *
     * @param file  File to save IMU data
     */
    void setSaveFile(const QString& file);

    /**
     * @brief Set the IMU device list
     *
     * @param imus IMU device list
     */
    void setDevices(const QStringList& imus);

    /**
     * @brief Set current selected IMU device
     *
     * @param index Current selected IMU device index
     */
    void setCurrentDevice(const std::size_t& index);

    /**
     * @brief Initialize
     */
    void init() override;

    /**
     * @brief Show live IMU reading
     */
    void live() override;

    /**
     * @brief Capture one IMU reading
     */
    void capture() override;

    /**
     * @brief Record IMU readings
     */
    void record() override;

  private:
    /**
     * @brief Setup UI
     */
    void setupUI();

    /**
     * @brief Enable setting widget
     *
     * @param enable  True for enable, false for disable
     */
    void enableSettingWidget(bool enable);

    /**
     * @brief Stop live show
     */
    void stopLive();

  private:
    std::shared_ptr<libra::io::SparkFunImuRecorder> recorder_;  // imu recorder
    QString saveFile_;                                          // save file
    std::fstream fileStream_;                                   // file stream to save IMU data
    SensorCaptureMode captureMode_ = SensorCaptureMode::None;   // capture mode

    QComboBox* deviceComboBox_ = nullptr;           // device comboBox
    QComboBox* timestampMethodComboBox_ = nullptr;  // timestamp retrieve method
    QLineEdit* saveFileLineEdit_ = nullptr;         // save file lineEdit
    QPushButton* selectFileButton_ = nullptr;       // select save file Button
    QPushButton* initButton_ = nullptr;             // init IMU button
    QPushButton* liveButton_ = nullptr;             // live show button
    QPushButton* captureButton_ = nullptr;          // capture reading button
    QPushButton* recordButton_ = nullptr;           // record readings button
    bool isLive_ = false;                           // flag to indict whether IMU is in live mode
    bool isRecord_ = false;                         // flag to indict whether IMU is in record mode
};