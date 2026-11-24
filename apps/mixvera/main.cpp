#include "mixvera/core/application.hpp"
#include "mixvera/engine/media_engine.hpp"
#include "mixvera/ui/main_window.hpp"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QTimer>

#include <iostream>

int main(int argc, char** argv)
{
    QApplication qt(argc, argv);
    QCoreApplication::setApplicationName("Mixvera");
    QCoreApplication::setApplicationVersion("0.1.0");
    QCoreApplication::setOrganizationName("Mixvera");

    QString screenshotPath;
    int screenshotDelayMs = 1200;
    const QStringList arguments = QCoreApplication::arguments();
    for (int index = 1; index < arguments.size(); ++index) {
        if (arguments[index] == "--screenshot" && index + 1 < arguments.size()) {
            screenshotPath = arguments[++index];
        } else if (arguments[index] == "--screenshot-delay" && index + 1 < arguments.size()) {
            screenshotDelayMs = arguments[++index].toInt();
        }
    }

    mixvera::engine::StubMediaEngine engine;
    if (!engine.initialize()) {
        std::cerr << "Failed to initialize media engine\n";
        return 1;
    }

    auto& app = mixvera::core::Application::instance();
    if (app.initialize() != 0) {
        return 1;
    }
    app.run();

    mixvera::ui::MainWindow window;
    window.show();

    if (!screenshotPath.isEmpty()) {
        QTimer::singleShot(screenshotDelayMs, &window, [&window, screenshotPath] {
            const QDir directory = QFileInfo(screenshotPath).absoluteDir();
            if (!directory.exists()) {
                directory.mkpath(".");
            }
            if (!window.grab().save(screenshotPath)) {
                std::cerr << "Failed to save screenshot: "
                          << screenshotPath.toStdString() << "\n";
            } else {
                std::cout << "Screenshot saved: " << screenshotPath.toStdString() << "\n";
            }
            QCoreApplication::quit();
        });
    }

    const int result = QApplication::exec();
    app.shutdown();
    engine.shutdown();
    return result;
}
