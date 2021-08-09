#include "libra/qt/CheckableTabWidget.h"
#include <fmt/format.h>
#include <glog/logging.h>
#include <QtWidgets>

using namespace libra::qt;

// Construct with parent widget
CheckableTabWidget::CheckableTabWidget(QWidget* parent) : QTabWidget(parent) {}

// Get the checkbox at index
QCheckBox* CheckableTabWidget::checkBox(int index) {
    CHECK(0 <= index && index < checkBoxes_.size())
        << fmt::format("input index({}) out of range({})", index, checkBoxes_.size());
    return checkBoxes_[index];
}

// Add a tab with given page and label to this tab widget
int CheckableTabWidget::addTab(QWidget* page, const QString& label) { return addTab(page, QIcon(), label); }

// Add a tab with given page, icon and label to this tab widget
int CheckableTabWidget::addTab(QWidget* page, const QIcon& icon, const QString& label) {
    int ret = QTabWidget::addTab(page, label);
    auto checkBox = new QCheckBox;
    checkBox->setChecked(true);
    checkBoxes_.append(checkBox);
    tabBar()->setTabButton(tabBar()->count() - 1, QTabBar::LeftSide, checkBox);
    connect(checkBox, &QCheckBox::stateChanged, this, [&](int state) {
        // NOTE the method to retrieve the sender object, and the `this` in connect() function to ensure the connection
        // to be `QueuedConnection` instead of `DirectConnection`
        int tabIndex = checkBoxes_.indexOf(qobject_cast<QCheckBox*>(sender()));
        if (state == Qt::CheckState::Checked) {
            QTabWidget::setTabEnabled(tabIndex, state == Qt::CheckState::Checked);
        } else {
            widget(tabIndex)->setEnabled(state == Qt::CheckState::Checked);
        }
        emit stateChanged(tabIndex, state);
    });

    return ret;
}

// Remove all pages
void CheckableTabWidget::clear() {
    checkBoxes_.clear();
    QTabWidget::clear();
}

// Enable given tab pag
void CheckableTabWidget::setTabEnabled(int index, bool enable) {
    checkBoxes_[index]->setEnabled(enable);
    QTabWidget::setTabEnabled(index, enable && checkBox(index)->isChecked());
}