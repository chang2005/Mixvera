#include "mixvera/core/application.hpp"
#include "mixvera/engine/media_engine.hpp"
#include "mixvera/ui/main_window.hpp"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>
#include <QSurfaceFormat>
#include <QTextStream>
#include <QTimer>

#include <iostream>

namespace {

QString widgetText(QWidget* widget)
{
    if (auto* label = qobject_cast<QLabel*>(widget)) {
        return label->text();
    }
    if (auto* button = qobject_cast<QPushButton*>(widget)) {
        return button->text();
    }
    if (auto* list = qobject_cast<QListWidget*>(widget)) {
        return QString("items=%1").arg(list->count());
    }
    if (auto* slider = qobject_cast<QSlider*>(widget)) {
        return QString("value=%1").arg(slider->value());
    }
    return QString();
}

QString dumpWidget(QWidget* widget, int depth)
{
    const QString pad(depth * 2, QLatin1Char(' '));
    const QRect geometry = widget->geometry();
    const QString text = widgetText(widget);
    QString line = QString("%1%2#%3 visible=%4 geom=(%5,%6 %7x%8)")
                       .arg(pad, QString::fromLatin1(widget->metaObject()->className()),
                           widget->objectName())
                       .arg(widget->isVisible() ? 1 : 0)
                       .arg(geometry.x())
                       .arg(geometry.y())
                       .arg(geometry.width())
                       .arg(geometry.height());
    if (!text.isEmpty()) {
        line += QString(" text=\"%1\"").arg(QString(text).replace('\n', ' '));
    }
    line += '\n';

    const auto children = widget->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (auto* child : children) {
        line += dumpWidget(child, depth + 1);
    }
    return line;
}

} // namespace

int main(int argc, char** argv)
{
    QApplication qt(argc, argv);
    QCoreApplication::setApplicationName("Mixvera");
    QCoreApplication::setApplicationVersion("0.2.0");
    QCoreApplication::setOrganizationName("Mixvera");

    QString screenshotPath;
    QString dumpPath;
    QString projectFilePath;
    int screenshotDelayMs = 1500;
    bool restoreSession = true;
    const QStringList arguments = QCoreApplication::arguments();
    for (int index = 1; index < arguments.size(); ++index) {
        if (arguments[index] == "--screenshot" && index + 1 < arguments.size()) {
            screenshotPath = arguments[++index];
        } else if (arguments[index] == "--dump-ui" && index + 1 < arguments.size()) {
            dumpPath = arguments[++index];
        } else if (arguments[index] == "--project" && index + 1 < arguments.size()) {
            projectFilePath = arguments[++index];
        } else if (arguments[index] == "--screenshot-delay" && index + 1 < arguments.size()) {
            screenshotDelayMs = arguments[++index].toInt();
        } else if (arguments[index] == "--no-restore") {
            restoreSession = false;
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

    mixvera::ui::MainWindow window(restoreSession);
    if (!projectFilePath.isEmpty() && !window.loadProject(projectFilePath)) {
        std::cerr << "Failed to load project: " << projectFilePath.toStdString() << "\n";
    }
    window.show();

    if (!screenshotPath.isEmpty() || !dumpPath.isEmpty()) {
        QTimer::singleShot(screenshotDelayMs, &window, [&window, screenshotPath, dumpPath] {
            if (!dumpPath.isEmpty()) {
                const QDir directory = QFileInfo(dumpPath).absoluteDir();
                if (!directory.exists()) {
                    directory.mkpath(".");
                }
                QFile file(dumpPath);
                if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream stream(&file);
                    stream << dumpWidget(&window, 0);
                    std::cout << "UI tree written: " << dumpPath.toStdString() << "\n";
                }
            }
            if (screenshotPath.isEmpty()) {
                QCoreApplication::quit();
                return;
            }
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