#include <gflags/gflags.h>
#include <glog/logging.h>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    google::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    FLAGS_alsologtostderr = true;

    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/Icon/SensorRecorder"));
    MainWindow mainWindow;
    mainWindow.showMaximized();

    google::ShutDownCommandLineFlags();
    google::ShutdownGoogleLogging();
    return app.exec();
}