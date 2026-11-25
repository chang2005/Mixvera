#include "mixvera/core/application.hpp"
#include "mixvera/core/project.hpp"
#include "mixvera/services/switcher_service.hpp"
#include "mixvera/ui/input_panel.hpp"
#include "mixvera/ui/main_window.hpp"
#include "mixvera/ui/media/recorder.hpp"
#include "mixvera/ui/stage_view.hpp"
#include "mixvera/ui/theme.hpp"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>
#include <QTemporaryDir>
#include <QtTest>

class UiSmokeTest final : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void defaultProjectIsLoaded();
    void cutAndAuto();
    void tbarCompletesTransition();
    void monitorsReceiveFrames();
    void inputListShowsEverySource();
    void selectingASceneDoesNotLoop();
    void overlayButtonsToggle();
    void snapshotWritesPng();
    void recordsThroughTheUi();
    void themeStylesheetsAreComplete();
    void themeIsAppliedToMainWindow();

private:
    mixvera::services::SwitcherService& switcher()
    {
        return mixvera::core::Application::instance().services()
            .get<mixvera::services::SwitcherService>();
    }
};

void UiSmokeTest::initTestCase()
{
    QCOMPARE(mixvera::core::Application::instance().initialize(), 0);
}

void UiSmokeTest::defaultProjectIsLoaded()
{
    mixvera::ui::MainWindow window(false);
    window.show();
    QTest::qWait(150);

    QCOMPARE(QString::fromStdString(switcher().state().programInputId), QString("scene-1"));
    QCOMPARE(QString::fromStdString(switcher().state().previewInputId), QString("scene-2"));
}

void UiSmokeTest::cutAndAuto()
{
    mixvera::ui::MainWindow window(false);
    window.show();
    QTest::qWait(150);

    auto* cut = window.findChild<QPushButton*>("cutButton");
    QVERIFY(cut != nullptr);
    cut->click();
    QCOMPARE(QString::fromStdString(switcher().state().programInputId), QString("scene-2"));

    auto* autoButton = window.findChild<QPushButton*>("autoButton");
    QVERIFY(autoButton != nullptr);
    autoButton->click();
    QVERIFY(!autoButton->isEnabled());
    QTest::qWait(900);
    QVERIFY(autoButton->isEnabled());
    QCOMPARE(QString::fromStdString(switcher().state().programInputId), QString("scene-1"));
    QCOMPARE(QString::fromStdString(switcher().state().previewInputId), QString("scene-2"));
}

void UiSmokeTest::tbarCompletesTransition()
{
    mixvera::ui::MainWindow window(false);
    window.show();
    QTest::qWait(150);

    auto* tbar = window.findChild<QSlider*>("tbar");
    QVERIFY(tbar != nullptr);
    tbar->setValue(1023);
    QVERIFY(QMetaObject::invokeMethod(tbar, "sliderReleased"));

    QCOMPARE(QString::fromStdString(switcher().state().programInputId), QString("scene-2"));
    QCOMPARE(tbar->value(), 0);
}

void UiSmokeTest::monitorsReceiveFrames()
{
    mixvera::ui::MainWindow window(false);
    window.show();
    QTest::qWait(400);

    const auto monitors = window.findChildren<mixvera::ui::StageView*>();
    QCOMPARE(monitors.size(), 2);
    for (const auto* monitor : monitors) {
        QVERIFY(!monitor->frame().isNull());
        QVERIFY(monitor->frame().width() >= 640);
    }
}

void UiSmokeTest::inputListShowsEverySource()
{
    mixvera::ui::MainWindow window(false);
    window.show();
    QTest::qWait(150);

    auto* panel = window.findChild<mixvera::ui::InputPanel*>("inputBar");
    QVERIFY(panel != nullptr);
    QCOMPARE(panel->count(), 5);
}

void UiSmokeTest::selectingASceneDoesNotLoop()
{
    mixvera::ui::MainWindow window(false);
    window.show();
    QTest::qWait(150);

    auto* scenes = window.findChild<QListWidget*>("sceneList");
    QVERIFY(scenes != nullptr);
    QCOMPARE(scenes->count(), 2);

    // The program scene cannot become the preview; selecting it must neither
    // recurse through the panel rebuild nor leave an invalid preview.
    scenes->setCurrentRow(0);
    QTest::qWait(50);
    scenes->setCurrentRow(1);
    QTest::qWait(50);
    QCOMPARE(QString::fromStdString(switcher().state().previewInputId), QString("scene-2"));
    QCOMPARE(QString::fromStdString(switcher().state().programInputId), QString("scene-1"));
}

void UiSmokeTest::overlayButtonsToggle()
{
    mixvera::ui::MainWindow window(false);
    window.show();
    QTest::qWait(150);

    const auto buttons = window.findChildren<QPushButton*>("overlayButton");
    QCOMPARE(buttons.size(), 4);
    QPushButton* layerOne = nullptr;
    for (auto* button : buttons) {
        if (button->property("overlayLayer").toInt() == 1) {
            layerOne = button;
        }
    }
    QVERIFY(layerOne != nullptr);
    QVERIFY(!layerOne->isChecked());

    auto* panel = window.findChild<mixvera::ui::InputPanel*>("inputBar");
    QVERIFY(panel != nullptr);
    panel->selectInput("title-1");
    layerOne->click();
    QVERIFY(layerOne->isChecked());
    QVERIFY(layerOne->toolTip().contains("Mixvera Title"));
    QTest::qWait(120);

    layerOne->click();
    QVERIFY(!layerOne->isChecked());
}

void UiSmokeTest::snapshotWritesPng()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    mixvera::core::Project project = mixvera::core::makeDefaultProject();
    project.outputs.recordingDirectory = directory.path().toStdString();
    const QString projectPath = QDir(directory.path()).filePath("snapshot.json");
    QVERIFY(mixvera::core::saveProjectFile(project, projectPath.toStdString()));

    mixvera::ui::MainWindow window(false);
    window.show();
    QVERIFY(window.loadProject(projectPath));
    QTest::qWait(350);

    auto* snapshot = window.findChild<QPushButton*>("snapshotButton");
    QVERIFY(snapshot != nullptr);
    snapshot->click();
    QTest::qWait(250);

    const QStringList files = QDir(directory.path()).entryList({"*.png"}, QDir::Files);
    QVERIFY2(!files.isEmpty(), "Snapshot button did not write a PNG");
    const QFileInfo info(QDir(directory.path()).filePath(files.front()));
    QVERIFY(info.size() > 1024);
}

void UiSmokeTest::recordsThroughTheUi()
{
    if (!mixvera::ui::Recorder::available()) {
        QSKIP("ffmpeg is not available on PATH");
    }

    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    mixvera::core::Project project = mixvera::core::makeDefaultProject();
    project.outputs.recordingDirectory = directory.path().toStdString();
    const QString projectPath = QDir(directory.path()).filePath("record.json");
    QVERIFY(mixvera::core::saveProjectFile(project, projectPath.toStdString()));

    mixvera::ui::MainWindow window(false);
    window.show();
    QVERIFY(window.loadProject(projectPath));
    QTest::qWait(200);

    auto* record = window.findChild<QPushButton*>("recordButton");
    QVERIFY(record != nullptr);
    record->click();
    QVERIFY(record->isChecked());
    QTest::qWait(1200);
    record->click();
    QVERIFY(!record->isChecked());
    QTest::qWait(400);

    const QStringList files = QDir(directory.path()).entryList({"*.mp4"}, QDir::Files);
    QVERIFY2(!files.isEmpty(), "No recording file was written by the UI");
    QVERIFY(QFileInfo(QDir(directory.path()).filePath(files.front())).size() > 1024);
}

void UiSmokeTest::themeStylesheetsAreComplete()
{
    using namespace mixvera::ui::theme;
    const ThemeId ids[] = {ThemeId::Charcoal, ThemeId::Yami, ThemeId::Daylight};
    QStringList sheets;
    for (ThemeId id : ids) {
        const QString qss = styleSheet(id);
        QVERIFY(!qss.isEmpty());
        QVERIFY2(!qss.contains("{{"),
            qPrintable(QStringLiteral("Unresolved token in %1").arg(themeKey(id))));
        QVERIFY(qss.contains(palette(id).window));
        QVERIFY(qss.contains(palette(id).program));
        sheets << qss;
    }
    QVERIFY(sheets.at(0) != sheets.at(1));
    QVERIFY(sheets.at(1) != sheets.at(2));
    QCOMPARE(themeFromKey("yami"), ThemeId::Yami);
    QCOMPARE(themeFromKey("obs"), ThemeId::Yami);
    QCOMPARE(themeFromKey("light"), ThemeId::Daylight);
    QCOMPARE(themeKey(ThemeId::Daylight), QStringLiteral("daylight"));
    QVERIFY(!themeName(ThemeId::Charcoal).isEmpty());
    QVERIFY(!themeDescription(ThemeId::Daylight).isEmpty());
}

void UiSmokeTest::themeIsAppliedToMainWindow()
{
    qApp->setProperty("mixvera/theme", QStringLiteral("daylight"));
    {
        mixvera::ui::MainWindow window(false);
        QVERIFY(window.styleSheet().contains(mixvera::ui::theme::palette(
            mixvera::ui::theme::ThemeId::Daylight).window));
    }
    qApp->setProperty("mixvera/theme", QString());
    QCOMPARE(mixvera::ui::theme::currentTheme(), mixvera::ui::theme::ThemeId::Daylight);
    mixvera::ui::theme::setTheme(mixvera::ui::theme::ThemeId::Charcoal);
}

QTEST_MAIN(UiSmokeTest)
#include "test_ui_smoke.moc"