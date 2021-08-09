#pragma once
#include <QCheckBox>
#include <QTabWidget>
#include <QtCore>

namespace libra {
namespace qt {

/**
 * @brief Checkable tab widget, add a QCheckBox in QTabWidget
 */
class CheckableTabWidget : public QTabWidget {
    Q_OBJECT

  public:
    /**
     * @brief Construct with parent widget
     * @param parent Parent widget
     */
    explicit CheckableTabWidget(QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~CheckableTabWidget() override = default;

  signals:
    /**
     * @brief Signals if the check in one tab is changed
     *
     * @param tabIndex  Tab index
     * @param state     Check state
     */
    void stateChanged(int tabIndex, int state);

  public:
    /**
     * @brief Get the checkbox at index
     *
     * @param index The tab index
     */
    QCheckBox* checkBox(int index);

    /**
     * @brief Add a tab with given page and label to this tab widget
     *
     * @param page  Given page
     * @param label Label
     * @return The index of the tab in the tab bar
     */
    int addTab(QWidget* page, const QString& label);

    /**
     * @brief Add a tab with given page, icon and label to this tab widget
     *
     * @param page  Given page
     * @param icon  Icon
     * @param label Label
     * @return The index of the tab in the tab bar
     */
    int addTab(QWidget* page, const QIcon& icon, const QString& label);

    /**
     * @brief Remove all pages
     */
    void clear();

    /**
     * @brief Enable given tab pag
     *
     * @param index   Index of the tab
     * @param enable  True for enable, false for disable
     */
    void setTabEnabled(int index, bool enable);

  private:
    QVector<QCheckBox*> checkBoxes_;  // checkboxes in tab bar
};

}  // namespace qt
}  // namespace libra