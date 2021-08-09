#pragma once
#include <QComboBox>
#include <QLabel>
#include <QScrollArea>
#include <QWidget>

namespace libra {
namespace qt {

/**
 * @brief Image viewer widget to show image
 */
class ImageViewer : public QWidget {
    Q_OBJECT

  public:
    /**
     * @brief Construct with parent widget
     * @param parent Parent widget
     */
    explicit ImageViewer(QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~ImageViewer() override = default;

  signals:
    /**
     * @brief Signals if image source is changed
     *
     * @param index Source index
     */
    void sourceChanged(int index);

  public:
    /**
     * @brief Set the image
     * @param image Image
     */
    void setImage(const QImage& image);

    /**
     * @brief Set the image source names
     *
     * @param source Image source names
     */
    void setSourceList(const QStringList& source);

    // some getter
    inline const QImage& image() const { return image_; }
    inline const double& scaleFactor() const { return scaleFactor_; }
    inline QAction* zoomOutAction() const { return zoomOutAction_; }
    inline QAction* zoomInAction() const { return zoomInAction_; }
    inline QAction* actualSizeAction() const { return actualSizeAction_; }
    inline QAction* fitToWindowAction() const { return fitToWindowAction_; }

    /**
     * @brief Check whether this image is valid
     *
     * @return True if image in viewer is valid, otherwise return false
     */
    bool isImageValid() const;

  private:
    /**
     * @brief Setup UI
     */
    void setupUI();

    /**
     * @brief Update(enable/disable) actions
     */
    void updateActions();

    /**
     * @brief Zoom out
     */
    void zoomOut();

    /**
     * @brief Zoom in
     */
    void zoomIn();

    /**
     * @brief Actual size
     */
    void actualSize();

    /**
     * @brief Fit image to window
     */
    void fitToWindow();

    /**
     * @brief Scale image
     *
     * @param factor Scale factor
     */
    void scaleImage(const double& factor);

    /**
     * @brief adjust scroll bar
     */
    void adjustScrollBar(QScrollBar* scrollBar, const double& factor);

  private:
    QLabel* imageLabel_ = nullptr;          // label to show image
    QScrollArea* scrollArea_ = nullptr;     // scroll area
    QComboBox* sourceComboBox_ = nullptr;   // image source comboBox
    QAction* sourceAction_ = nullptr;       // action for image source
    QAction* zoomOutAction_ = nullptr;      // zoom out action
    QAction* zoomInAction_ = nullptr;       // zoom in action
    QAction* actualSizeAction_ = nullptr;   // actual size action
    QAction* fitToWindowAction_ = nullptr;  // fit to window action

    QImage image_;        // image
    double scaleFactor_;  // scale factor
};

}  // namespace qt
}  // namespace libra