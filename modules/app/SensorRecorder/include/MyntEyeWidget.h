#pragma once
#include <QtWidgets>
#include <memory>
#include "ISensorWidget.hpp"
#include "ImageSaveFormat.hpp"
#include "SensorCaptureMode.hpp"
#include "libra/io/MyntEyeRecorder.h"

/**
 * @brief Mynt Eye sensor widget
 */
class MyntEyeWidget : public ISensorWidget {
    Q_OBJECT

  public:
    /**
     * @brief Constructor
     *
     * @param parent  parent widget
     */
    explicit MyntEyeWidget(QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~MyntEyeWidget() override = default;

  signals:
    /**
     * @brief Signal if sensor working state is changed
     *
     * @param working True for sensor is working, otherwise return false
     */
    void sensorStateChanged(bool working);

  public:
    /**
     * @brief Is the right camera is enabled
     *
     * @return  True for the right camera is enabled, false for not.
     */
    bool isRightCamEnabled() const;

    /**
     * @brief Set the camera device list
     *
     * @param cameras Camera device list with ID and name. <ID, name>
     */
    void setDevices(const std::vector<std::pair<unsigned int, std::string>>& cameras);

    /**
     * @brief Set current selected camera device
     *
     * @param index Current selected camera  device index
     */
    void setCurrentDevice(const std::size_t& index);

    /**
     * @brief Set left image save folder
     *
     * @param folder  Folder to save left images
     */
    void setLeftImageSaveFolder(const QString& folder);

    /**
     * @brief Set right image save folder
     *
     * @param folder  Folder to save right images
     */
    void setRightImageSaveFolder(const QString& folder);

    /**
     * @brief Set IMU save file
     *
     * @param file  File to save IMU data
     */
    void setImuSaveFile(const QString& file);

    /**
     * @brief Initialize
     */
    void init() override;

    /**
     * @brief Show live image
     */
    void live() override;

    /**
     * @brief Capture one image
     */
    void capture() override;

    /**
     * @brief Record images
     */
    void record() override;

  signals:

    /**
     * @brief Signal if right camera is enabled or distable
     *
     * @param enable True for enabled, false for disable
     */
    void rightCamEnabled(bool enable);

    /**
     * @brief Signal if got(captured) one left image, only used for display in image viewer
     *
     * @param img The image captured by left camera
     */
    void newLeftImage(const QImage& img);

    /**
     * @brief Signal if got(captured) one right image, only used for display in image viewer
     *
     * @param img The image captured by right camera
     */
    void newRightImage(const QImage& img);

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
    std::shared_ptr<libra::io::MyntEyeRecorder> recorder_;     // camera recorder
    QString leftImgSaveFolder_;                                // left image save folder
    QString rightImgSaveFolder_;                               // right image save folder
    QString imuSaveFile_;                                      // IMU save file
    std::fstream imuFileStream_;                               // file stream to save IMU data
    SensorCaptureMode captureMode_ = SensorCaptureMode::None;  // capture mode
    ImageSaveFormat saveFormat_ = ImageSaveFormat::Kalibr;     // image save format
    std::size_t leftImageIndex_ = 0;                           // left image index
    std::size_t leftSaveIndex_ = 0;                            // left save image index
    std::size_t rightImageIndex_ = 0;                          // right image index
    std::size_t rightSaveIndex_ = 0;                           // right save image index

    QComboBox* deviceComboBox_ = nullptr;              // camera device comboBox
    QSpinBox* frameRateSpinBox_ = nullptr;             // frame rate spinBox
    QComboBox* streamModeComboBox_ = nullptr;          // stream mode comboBox
    QSpinBox* saverThreadNumSpinBox_ = nullptr;        // saver thread number spinBox
    QComboBox* saveFormatComboBox_ = nullptr;          // image save format comboBox
    QComboBox* timestampMethodComboBox_ = nullptr;     // timestamp retrieve method
    QLineEdit* leftImgSaveFolderLineEdit_ = nullptr;   // left image save folder lineEdit
    QPushButton* selectLeftFolderButton_ = nullptr;    // select left image save folder Button
    QLineEdit* rightImgSaveFolderLineEdit_ = nullptr;  // right image save folder lineEdit
    QPushButton* selectRightFolderButton_ = nullptr;   // select right image save folder Button
    QLineEdit* imuSaveFileLineEdit_ = nullptr;         // IMU save file lineEdit
    QPushButton* selectImuFileButton_ = nullptr;       // select IMU save file Button
    QPushButton* initButton_ = nullptr;                // init camera button
    QPushButton* liveButton_ = nullptr;                // live show button
    QPushButton* captureButton_ = nullptr;             // capture image button
    QPushButton* recordButton_ = nullptr;              // record images button
    QGroupBox* groupBox_;                              // group box
    bool isLive_ = false;                              // flag to indict whether camera is in live mode
    bool isRecord_ = false;                            // flag to indict whether camera is in record mode
};