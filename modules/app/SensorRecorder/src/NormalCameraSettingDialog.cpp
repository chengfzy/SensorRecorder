#include "NormalCameraSettingDialog.h"

using namespace libra::io;

// Constructor with normal camera recorder and parent widget
NormalCameraSettingDialog::NormalCameraSettingDialog(const std::shared_ptr<libra::io::NormalCameraRecorder>& recorder,
                                                     QWidget* parent)
    : QDialog(parent), recorder_(recorder) {
    DCHECK(recorder_);
    setupUI();
    setWindowIcon(QIcon(":/Icon/Setting"));
}

// Apply setting parameters
void NormalCameraSettingDialog::apply() {
    fpsSpinBox_->setValue(recorder_->setFps(fpsSpinBox_->value()));
    auto frameSize =
        recorder_->setFrameSize(Eigen::Vector2i(frameWidthSpinBox_->value(), frameHeightSpinBox_->value()));
    frameWidthSpinBox_->setValue(recorder_->frameWidth());
    frameHeightSpinBox_->setValue(recorder_->frameHeight());
}

// Setup UI
void NormalCameraSettingDialog::setupUI() {
    // FPS
    fpsSpinBox_ = new QDoubleSpinBox;
    fpsSpinBox_->setRange(0, 100);
    fpsSpinBox_->setValue(recorder_->fps());
    fpsSpinBox_->setDecimals(2);
    fpsSpinBox_->setSingleStep(1);
    // frame size
    frameWidthSpinBox_ = new QSpinBox;
    frameWidthSpinBox_->setRange(0, 5000);
    frameWidthSpinBox_->setSingleStep(100);
    frameWidthSpinBox_->setValue(recorder_->frameWidth());
    frameHeightSpinBox_ = new QSpinBox;
    frameHeightSpinBox_->setRange(0, 5000);
    frameHeightSpinBox_->setSingleStep(100);
    frameHeightSpinBox_->setValue(recorder_->frameHeight());
    // OK, Cancel buttonBox
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &NormalCameraSettingDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &NormalCameraSettingDialog::reject);
    auto applyButton = buttonBox->button(QDialogButtonBox::Apply);
    connect(applyButton, &QPushButton::clicked, this, &NormalCameraSettingDialog::apply);

    // frame size layout
    auto frameSizeLayout = new QHBoxLayout;
    frameSizeLayout->addWidget(frameWidthSpinBox_);
    frameSizeLayout->addWidget(new QLabel("X"), 0, Qt::AlignCenter);
    frameSizeLayout->addWidget(frameHeightSpinBox_);

    // setting layout
    auto mainLayout = new QFormLayout;
    mainLayout->addRow("FPS (Hz)", fpsSpinBox_);
    mainLayout->addRow("Frame Size", frameSizeLayout);
    mainLayout->addRow(buttonBox);

    setLayout(mainLayout);
}
