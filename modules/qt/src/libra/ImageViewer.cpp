#include "libra/qt/ImageViewer.h"
#include <QtWidgets>

using namespace libra::qt;

// Construct with parent widget
ImageViewer::ImageViewer(QWidget* parent)
    : QWidget(parent), imageLabel_(new QLabel), scrollArea_(new QScrollArea), scaleFactor_(1.0) {
    setupUI();
}

// Set image
void ImageViewer::setImage(const QImage& image) {
    // only reset scale factor if input image size is different
    bool isSameSize = image.size() == image_.size();

    image_ = image;
    imageLabel_->setPixmap(QPixmap::fromImage(image_));
    if (!isSameSize) {
        scaleFactor_ = 1.0;
    }

    scrollArea_->setVisible(isImageValid());
    fitToWindowAction_->setEnabled(isImageValid());
    updateActions();

    if (!isSameSize && !fitToWindowAction_->isChecked()) {
        imageLabel_->adjustSize();
    }
}

// Set the image source names
void ImageViewer::setSourceList(const QStringList& source) {
    sourceComboBox_->clear();
    sourceComboBox_->addItems(source);
    sourceComboBox_->setCurrentIndex(0);
    sourceAction_->setVisible(sourceComboBox_->count() > 0);
}

// Check whether this image is valid
bool ImageViewer::isImageValid() const { return !image_.isNull() && !image_.size().isEmpty(); }

// Setup UI
void ImageViewer::setupUI() {
    // image label
    imageLabel_->setBackgroundRole(QPalette::Background);
    imageLabel_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabel_->setScaledContents(true);

    // scroll area
    scrollArea_->setBackgroundRole(QPalette::Background);
    scrollArea_->setWidget(imageLabel_);
    scrollArea_->setWidgetResizable(false);
    scrollArea_->setVisible(false);

    // source comboBox
    sourceComboBox_ = new QComboBox;
    sourceComboBox_->setSizeAdjustPolicy(QComboBox::SizeAdjustPolicy::AdjustToContents);
    connect(sourceComboBox_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            &ImageViewer::sourceChanged);
    // Action: zoom out
    zoomOutAction_ = new QAction(QIcon(":/Icon/ZoomOut"), QString::fromLocal8Bit("Zoom Out"), this);
    zoomOutAction_->setStatusTip(QString::fromLocal8Bit("Zoom out"));
    zoomOutAction_->setShortcut(QKeySequence::ZoomOut);
    zoomOutAction_->setEnabled(false);
    connect(zoomOutAction_, &QAction::triggered, this, &ImageViewer::zoomOut);
    // Action: zoom in
    zoomInAction_ = new QAction(QIcon(":/Icon/ZoomIn"), QString::fromLocal8Bit("Zoom In"), this);
    zoomInAction_->setStatusTip(QString::fromLocal8Bit("Zoom in"));
    zoomInAction_->setShortcut(QKeySequence::ZoomIn);
    zoomInAction_->setEnabled(false);
    connect(zoomInAction_, &QAction::triggered, this, &ImageViewer::zoomIn);
    // Action: actual size
    actualSizeAction_ = new QAction(QIcon(":/Icon/ActualSize"), QString::fromLocal8Bit("Actual Size"), this);
    actualSizeAction_->setStatusTip(QString::fromLocal8Bit("Actual size"));
    actualSizeAction_->setEnabled(false);
    connect(actualSizeAction_, &QAction::triggered, this, &ImageViewer::actualSize);
    // Action: fit to window
    fitToWindowAction_ = new QAction(QIcon(":/Icon/FitToWindow"), QString::fromLocal8Bit("Fit to Window"), this);
    fitToWindowAction_->setStatusTip(QString::fromLocal8Bit("Fit to window"));
    fitToWindowAction_->setCheckable(true);
    fitToWindowAction_->setEnabled(false);
    connect(fitToWindowAction_, &QAction::triggered, this, &ImageViewer::fitToWindow);

    // toolbar
    auto toolbar = new QToolBar;
    sourceAction_ = toolbar->addWidget(sourceComboBox_);
    sourceAction_->setVisible(false);
    toolbar->addAction(zoomOutAction_);
    toolbar->addAction(zoomInAction_);
    toolbar->addAction(actualSizeAction_);
    toolbar->addAction(fitToWindowAction_);

    // main layout
    auto mainLayout = new QVBoxLayout;
    mainLayout->addWidget(toolbar, 0, Qt::AlignCenter);
    mainLayout->addWidget(scrollArea_);
    setLayout(mainLayout);
}

// Update(enable/disable) actions
void ImageViewer::updateActions() {
    bool enable = isImageValid() && !fitToWindowAction_->isChecked();
    zoomOutAction_->setEnabled(enable);
    zoomInAction_->setEnabled(enable);
    actualSizeAction_->setEnabled(enable);
}

// Zoom in
void ImageViewer::zoomIn() { scaleImage(1.1); }

// Zoom out
void ImageViewer::zoomOut() { scaleImage(0.9); }

// Actual size
void ImageViewer::actualSize() {
    imageLabel_->adjustSize();
    scaleFactor_ = 1.0;
}

// Fit image to window
void ImageViewer::fitToWindow() {
    bool enable = fitToWindowAction_->isChecked();
    scrollArea_->setWidgetResizable(enable);
    if (!enable) {
        actualSize();
    }
    updateActions();
}

// Scale image
void ImageViewer::scaleImage(const double& factor) {
    scaleFactor_ *= factor;
    imageLabel_->resize(scaleFactor_ * imageLabel_->pixmap()->size());

    adjustScrollBar(scrollArea_->horizontalScrollBar(), factor);
    adjustScrollBar(scrollArea_->verticalScrollBar(), factor);

    zoomOutAction_->setEnabled(scaleFactor_ > 0.2);
    zoomInAction_->setEnabled(scaleFactor_ < 5.);
}

// adjust scroll bar
void ImageViewer::adjustScrollBar(QScrollBar* scrollBar, const double& factor) {
    scrollBar->setValue(int(factor * scrollBar->value() + ((factor - 1) * scrollBar->pageStep() / 2)));
}