#include "mixvera/core/application.hpp"
#include "mixvera/engine/media_engine.hpp"
#include "mixvera/ui/main_window.hpp"
#include "mixvera/ui/settings_dialog.hpp"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QListWidget>
#include <QLocale>
#include <QPushButton>
#include <QSettings>
#include <QSlider>
#include <QSurfaceFormat>
#include <QTextStream>
#include <QTimer>
#include <QTranslator>

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
    QCoreApplication::setApplicationVersion("0.4.0");
    QCoreApplication::setOrganizationName("Mixvera");

    QString screenshotPath;
    QString settingsScreenshotPath;
    int settingsPage = -1;
    QString dumpPath;
    QString projectFilePath;
    QString languageOverride;
    QString themeOverride;
    int screenshotDelayMs = 1500;
    bool restoreSession = QSettings().value("ui/restoreSession", true).toBool();
    bool showMixer = false;
    const QStringList arguments = QCoreApplication::arguments();
    for (int index = 1; index < arguments.size(); ++index) {
        if (arguments[index] == "--screenshot" && index + 1 < arguments.size()) {
            screenshotPath = arguments[++index];
        } else if (arguments[index] == "--settings-screenshot"
            && index + 1 < arguments.size()) {
            settingsScreenshotPath = arguments[++index];
        } else if (arguments[index] == "--settings-page" && index + 1 < arguments.size()) {
            settingsPage = arguments[++index].toInt();
        } else if (arguments[index] == "--dump-ui" && index + 1 < arguments.size()) {
            dumpPath = arguments[++index];
        } else if (arguments[index] == "--project" && index + 1 < arguments.size()) {
            projectFilePath = arguments[++index];
        } else if (arguments[index] == "--screenshot-delay" && index + 1 < arguments.size()) {
            screenshotDelayMs = arguments[++index].toInt();
        } else if (arguments[index] == "--lang" && index + 1 < arguments.size()) {
            languageOverride = arguments[++index];
        } else if (arguments[index] == "--theme" && index + 1 < arguments.size()) {
            themeOverride = arguments[++index];
        } else if (arguments[index] == "--show-mixer") {
            showMixer = true;
        } else if (arguments[index] == "--no-restore") {
            restoreSession = false;
        }
    }

    QString language = languageOverride;
    if (language.isEmpty()) {
        language = qEnvironmentVariable("MIXVERA_LANG");
    }
    if (language.isEmpty()) {
        QSettings settings;
        language = settings.value("ui/language").toString();
    }
    if (language.isEmpty()) {
        language = QLocale::system().language() == QLocale::Chinese ? "zh_CN" : "en";
    }

    if (themeOverride.isEmpty()) {
        themeOverride = qEnvironmentVariable("MIXVERA_THEME");
    }
    qt.setProperty("mixvera/theme", themeOverride);
    qt.setProperty("mixvera/language", language);

    auto* translator = new QTranslator(&qt);
    if (language.startsWith("zh", Qt::CaseInsensitive)) {
        if (translator->load(QStringLiteral(":/i18n/mixvera_zh_CN.qm"))) {
            qt.installTranslator(translator);
        } else {
            std::cerr << "Failed to load the Simplified Chinese translation\n";
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
    if (showMixer) {
        window.setMixerVisible(true);
    }
    window.show();

    if (!settingsScreenshotPath.isEmpty()) {
        auto* dialog = new mixvera::ui::SettingsDialog(&window);
        dialog->setTheme(mixvera::ui::theme::currentTheme());
        dialog->setLanguage(language.startsWith("zh", Qt::CaseInsensitive)
                ? QStringLiteral("zh_CN")
                : QStringLiteral("en"));
        if (auto* nav = dialog->findChild<QListWidget*>("settingsNav")) {
            if (settingsPage >= 0 && settingsPage < nav->count()) {
                nav->setCurrentRow(settingsPage);
            }
        }
        dialog->show();
        QTimer::singleShot(screenshotDelayMs, &window,
            [dialog, settingsScreenshotPath] {
                const QDir directory = QFileInfo(settingsScreenshotPath).absoluteDir();
                if (!directory.exists()) {
                    directory.mkpath(".");
                }
                if (!dialog->grab().save(settingsScreenshotPath)) {
                    std::cerr << "Failed to save settings screenshot: "
                              << settingsScreenshotPath.toStdString() << "\n";
                } else {
                    std::cout << "Settings screenshot saved: "
                              << settingsScreenshotPath.toStdString() << "\n";
                }
                dialog->close();
                QCoreApplication::quit();
            });
    }

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