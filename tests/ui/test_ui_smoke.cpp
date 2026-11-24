#include "mixvera/core/application.hpp"
#include "mixvera/services/output_service.hpp"
#include "mixvera/services/replay_service.hpp"
#include "mixvera/services/switcher_service.hpp"
#include "mixvera/ui/main_window.hpp"

#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QtTest>

class UiSmokeTest final : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void startState();
    void cutAndAuto();
    void tbarCompletesTransition();
    void outputAndReplayControls();

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

void UiSmokeTest::startState()
{
    mixvera::ui::MainWindow window;
    window.show();

    QCOMPARE(QString::fromStdString(switcher().state().programInputId), QString("camera-1"));
    QCOMPARE(QString::fromStdString(switcher().state().previewInputId), QString("camera-2"));
}

void UiSmokeTest::cutAndAuto()
{
    mixvera::ui::MainWindow window;
    window.show();
    QTest::qWait(50);

    auto* cut = window.findChild<QPushButton*>("cutButton");
    QVERIFY(cut != nullptr);
    cut->click();
    QCOMPARE(QString::fromStdString(switcher().state().programInputId), QString("camera-2"));

    auto* autoButton = window.findChild<QPushButton*>("autoButton");
    QVERIFY(autoButton != nullptr);
    autoButton->click();
    QVERIFY(!autoButton->isEnabled());
    QTest::qWait(750);
    QVERIFY(autoButton->isEnabled());
    QCOMPARE(QString::fromStdString(switcher().state().programInputId), QString("camera-1"));
    QCOMPARE(QString::fromStdString(switcher().state().previewInputId), QString("camera-2"));
}

void UiSmokeTest::tbarCompletesTransition()
{
    mixvera::ui::MainWindow window;
    window.show();
    QTest::qWait(50);

    auto* tbar = window.findChild<QSlider*>("tbar");
    QVERIFY(tbar != nullptr);
    tbar->setValue(1023);
    QVERIFY(QMetaObject::invokeMethod(tbar, "sliderReleased"));

    QCOMPARE(QString::fromStdString(switcher().state().programInputId), QString("camera-2"));
    QCOMPARE(tbar->value(), 0);
}

void UiSmokeTest::outputAndReplayControls()
{
    mixvera::ui::MainWindow window;
    window.show();
    QTest::qWait(50);

    auto& app = mixvera::core::Application::instance();
    auto& outputs = app.services().get<mixvera::services::OutputService>();
    auto& replay = app.services().get<mixvera::services::ReplayService>();

    const auto buttons = window.findChildren<QPushButton*>("outputButton");
    QCOMPARE(buttons.size(), 3);
    QVERIFY(!outputs.outputs().front().running);

    buttons.front()->click();
    QVERIFY(outputs.outputs().front().running);
    QCOMPARE(buttons.front()->text(), QString("STOP"));

    buttons.front()->click();
    QVERIFY(!outputs.outputs().front().running);
    QCOMPARE(buttons.front()->text(), QString("START"));

    auto* record = window.findChild<QPushButton*>("recButton");
    QVERIFY(record != nullptr);
    record->click();
    QVERIFY(replay.state().recording);
    QCOMPARE(record->text(), QString("STOP"));
}

QTEST_MAIN(UiSmokeTest)
#include "test_ui_smoke.moc"
