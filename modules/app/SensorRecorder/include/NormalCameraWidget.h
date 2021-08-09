#pragma once
#include <QtWidgets>
#include <memory>
#include "ISensorWidget.hpp"
#include "ImageSaveFormat.hpp"
#include "SensorCaptureMode.hpp"
#include "libra/io/NormalCameraRecorder.h"

/**
 * @brief Normal camera widget
 */
class NormalCameraWidget : public ISensorWidget {
    Q_OBJECT

  public:
    /**
     * @brief Constructor
     *
     * @param parent  parent widget
     */
    explicit NormalCameraWidget(QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~NormalCameraWidget() override = default;

  signals:
    /**
     * @brief Signal if sensor working state is changed
     *
     * @param working True for sensor is working, otherwise return false
     */
    void sensorStateChanged(bool working);

  public:
    /**
     * @brief Set the camera device list
     *
     * @param cameras Camera device list
     */
    void setDevices(const QStringList& cameras);

    /**
     * @brief Set current selected camera device
     *
     * @param index Current selected camera  device index
     */
    void setCurrentDevice(const std::size_t& index);

    /**
     * @brief Set save folder
     *
     * @param folder  Folder to save images
     */
    void setSaveFolder(const QString& folder);

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
     * @brief Signal if got(captured) one image, only used for display in image viewer
     *
     * @param img The captured image
     */
    void newImage(const QImage& img);

  private:
    /**
     * @brief Setup UI
     */
    void setupUI();

    /**
     * @brief Enable config widget
     *
     * @param enable  True for enable, false for disable
     */
    void enableConfigWidget(bool enable);

    /**
     * @brief Stop live show
     */
    void stopLive();

  private:
    std::shared_ptr<libra::io::NormalCameraRecorder> recorder_;  // camera recorder
    QString saveFolder_;                                         // save folder
    SensorCaptureMode captureMode_ = SensorCaptureMode::None;    // capture mode
    ImageSaveFormat saveFormat_ = ImageSaveFormat::Kalibr;       // image save format
    std::size_t imageIndex_ = 0;                                 // image index
    std::size_t saveIndex_ = 0;                                  // save image index

    QComboBox* deviceComboBox_ = nullptr;        // camera device comboBox
    QSpinBox* saverThreadNumSpinBox_ = nullptr;  // saver thread number spinBox
    QDoubleSpinBox* fpsSpinBox_ = nullptr;       // FPS spinBox
    QSpinBox* frameWidthSpinBox_ = nullptr;      // frame width
    QSpinBox* frameHeightSpinBox_ = nullptr;     // frame height
    QComboBox* saveFormatComboBox_ = nullptr;    // save format comboBox
    QLineEdit* saveFolderLineEdit_ = nullptr;    // save folder lineEdit
    QPushButton* selectFolderButton_ = nullptr;  // select save folder Button
    QPushButton* initButton_ = nullptr;          // init camera button
    QPushButton* settingButton_ = nullptr;       // setting camera button
    QPushButton* liveButton_ = nullptr;          // live show button
    QPushButton* captureButton_ = nullptr;       // capture image button
    QPushButton* recordButton_ = nullptr;        // record images button
    bool isLive_ = false;                        // flag to indict whether camera is in live mode
    bool isRecord_ = false;                      // flag to indict whether camera is in record mode
};