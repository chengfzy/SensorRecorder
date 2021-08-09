#pragma once
#include <QtWidgets>
#include "libra/io/NormalCameraRecorder.h"

class NormalCameraSettingDialog : public QDialog {
  public:
    /**
     * @brief Constructor with normal camera recorder and parent widget
     *
     * @param recorder  Normal camera widget
     * @param parent    Parent widget
     */
    explicit NormalCameraSettingDialog(const std::shared_ptr<libra::io::NormalCameraRecorder>& recorder,
                                       QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~NormalCameraSettingDialog() = default;

  public:
    /**
     * @brief Apply setting parameters
     */
    void apply();

  private:
    /**
     * @brief Setup UI
     */
    void setupUI();

  private:
    std::shared_ptr<libra::io::NormalCameraRecorder> recorder_;  // camera recorder
    QDoubleSpinBox* fpsSpinBox_ = nullptr;                       // FPS spinBox
    QSpinBox* frameWidthSpinBox_ = nullptr;                      // frame width
    QSpinBox* frameHeightSpinBox_ = nullptr;                     // frame height
};