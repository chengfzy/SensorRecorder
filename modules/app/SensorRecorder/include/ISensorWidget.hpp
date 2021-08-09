#pragma once
#include <QWidget>

/**
 * @brief Sensor widget interface.
 */
class ISensorWidget : public QWidget {
    Q_OBJECT

  public:
    /**
     * @brief Constructor
     *
     * @param parent  parent widget
     */
    explicit ISensorWidget(QWidget* parent = nullptr) : QWidget(parent) {}

    /**
     * @brief Destructor
     */
    ~ISensorWidget() override = default;

  public:
    /**
     * @brief Initialize
     */
    virtual void init() = 0;

    /**
     * @brief Show live sensor reading
     */
    virtual void live() = 0;

    /**
     * @brief Capture one sensor reading
     */
    virtual void capture() = 0;

    /**
     * @brief Record sensor readings
     */
    virtual void record() = 0;
};