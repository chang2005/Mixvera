#include "mixvera/core/application.hpp"
#include "mixvera/core/project.hpp"
#include "mixvera/services/switcher_service.hpp"
#include "mixvera/ui/main_window.hpp"
#include "mixvera/ui/media/recorder.hpp"
#include "mixvera/ui/stage_view.hpp"

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
    void recordsThroughTheUi();

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

    auto* list = window.findChild<QListWidget*>("inputList");
    QVERIFY(list != nullptr);
    QCOMPARE(list->count(), 5);
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

QTEST_MAIN(UiSmokeTest)
#include "test_ui_smoke.moc"