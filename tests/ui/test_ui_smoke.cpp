#include "mixvera/core/application.hpp"
#include "mixvera/core/project.hpp"
#include "mixvera/services/input_service.hpp"
#include "mixvera/services/switcher_service.hpp"
#include "mixvera/ui/add_input_dialog.hpp"
#include "mixvera/ui/input_panel.hpp"
#include "mixvera/ui/main_window.hpp"
#include "mixvera/ui/media/recorder.hpp"
#include "mixvera/ui/settings_dialog.hpp"
#include "mixvera/ui/stage_view.hpp"
#include "mixvera/ui/title_element_dialog.hpp"

#include "../support/title_package_fixture.hpp"
#include "mixvera/ui/theme.hpp"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
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
    void portraitCanvasSwitches();
    void settingsExposeDisplayModes();
    void titlePackageControlsRoundTrip();
    void titleElementDialogEditsText();
    void titlePresetsApplySnapshots();
    void subtitleActionsStepPresets();
    void chromaKeyRoundTripsThroughDialog();
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

void UiSmokeTest::portraitCanvasSwitches()
{
    mixvera::ui::MainWindow window(false);
    window.show();
    QTest::qWait(150);

    auto* action = window.findChild<QAction*>("portraitCanvasAction");
    QVERIFY(action != nullptr);
    QCOMPARE(window.canvasSize(), QSize(1920, 1080));
    QVERIFY(!action->isChecked());

    window.setCanvasSize(1080, 1920, false);
    QCOMPARE(window.canvasSize(), QSize(1080, 1920));
    QVERIFY(action->isChecked());

    window.setCanvasSize(1920, 1080, false);
    QCOMPARE(window.canvasSize(), QSize(1920, 1080));
    QVERIFY(!action->isChecked());
}

void UiSmokeTest::settingsExposeDisplayModes()
{
    mixvera::ui::SettingsDialog dialog;
    auto* mode = dialog.findChild<QComboBox*>("canvasMode");
    QVERIFY(mode != nullptr);
    QCOMPARE(mode->count(), 2);
    QCOMPARE(mode->itemData(0).toString(), QStringLiteral("landscape"));
    QCOMPARE(mode->itemData(1).toString(), QStringLiteral("portrait"));
    QVERIFY(mode->itemText(0).contains(QStringLiteral("Landscape")));
    QVERIFY(mode->itemText(1).contains(QStringLiteral("Portrait")));

    dialog.setValues(1920, 1080, 60, {}, {}, {});
    QCOMPARE(mode->currentData().toString(), QStringLiteral("landscape"));
    dialog.setValues(1080, 1920, 60, {}, {}, {});
    QCOMPARE(mode->currentData().toString(), QStringLiteral("portrait"));
    QCOMPARE(dialog.canvasWidth(), 1080);
    QCOMPARE(dialog.canvasHeight(), 1920);
    dialog.setValues(1280, 720, 60, {}, {}, {});
    QCOMPARE(mode->currentData().toString(), QStringLiteral("landscape"));
}

void UiSmokeTest::titlePackageControlsRoundTrip()
{
    mixvera::ui::AddInputDialog dialog;
    mixvera::core::InputDescriptor title;
    title.id = "package-title";
    title.name = "Package";
    title.kind = mixvera::core::InputKind::Title;
    title.params["style"] = "package";
    title.params["package"] = "C:/Titles/example.gtzip";
    title.params["text"] = "HELLO";
    title.params["subtitle"] = "Second line";
    title.params["captions"] = "ONE\nTWO";
    title.params["titleText:Headline"] = "HELLO";
    title.params["titleColor:Headline"] = "#ff0000";
    title.params["titleImage:Logo"] = "C:/logo.png";
    dialog.loadFrom(title);

    auto* style = dialog.findChild<QComboBox*>("titleStyle");
    auto* row = dialog.findChild<QWidget*>("titlePackageRow");
    auto* path = dialog.findChild<QLineEdit*>("titlePackagePath");
    auto* elements = dialog.findChild<QPushButton*>("titleElementsButton");
    auto* info = dialog.findChild<QLabel*>("packageInfo");
    QVERIFY(style != nullptr);
    QVERIFY(row != nullptr);
    QVERIFY(path != nullptr);
    QVERIFY(elements != nullptr);
    QVERIFY(info != nullptr);

    QCOMPARE(style->currentData().toString(), QStringLiteral("package"));
    QVERIFY(!row->isHidden());
    QCOMPARE(path->text(), QStringLiteral("C:/Titles/example.gtzip"));
    QVERIFY(!elements->isEnabled());
    QVERIFY(info->text().contains(QStringLiteral("Could not read")));
    QCOMPARE(style->itemData(style->findData(QStringLiteral("package"))).toString(),
        QStringLiteral("package"));

    const auto descriptor = dialog.descriptor();
    QCOMPARE(descriptor.kind, mixvera::core::InputKind::Title);
    const auto param = [&descriptor](const char* key) {
        const auto it = descriptor.params.find(key);
        return it == descriptor.params.end() ? QString() : QString::fromStdString(it->second);
    };
    QCOMPARE(param("style"), QStringLiteral("package"));
    QCOMPARE(param("package"), QStringLiteral("C:/Titles/example.gtzip"));
    QCOMPARE(param("text"), QStringLiteral("HELLO"));
    QCOMPARE(param("subtitle"), QStringLiteral("Second line"));
    QCOMPARE(param("captions"), QStringLiteral("ONE\nTWO"));
    QCOMPARE(param("titleText:Headline"), QStringLiteral("HELLO"));
    QCOMPARE(param("titleColor:Headline"), QStringLiteral("#ff0000"));
    QCOMPARE(param("titleImage:Logo"), QStringLiteral("C:/logo.png"));
}

void UiSmokeTest::titleElementDialogEditsText()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = writeSyntheticPackage(directory);
    QVERIFY(!path.isEmpty());
    const mixvera::ui::TitlePackage package = mixvera::ui::TitlePackage::load(path);
    QVERIFY(package.isValid());

    mixvera::ui::TitleElementDialog dialog(package, {}, {}, {}, QSize(1280, 720));
    auto* list = dialog.findChild<QListWidget*>("titleElementList");
    auto* text = dialog.findChild<QPlainTextEdit*>("titleElementText");
    QVERIFY(list != nullptr);
    QVERIFY(text != nullptr);
    QCOMPARE(list->count(), 4);

    int headlineRow = -1;
    for (int row = 0; row < list->count(); ++row) {
        if (list->item(row)->text().startsWith(QStringLiteral("Headline"))) {
            headlineRow = row;
            break;
        }
    }
    QVERIFY(headlineRow >= 0);
    list->setCurrentRow(headlineRow);
    QCOMPARE(text->toPlainText(), QStringLiteral("DEFAULT HEADLINE"));

    text->setPlainText(QStringLiteral("LIVE HEADLINE"));
    QCOMPARE(dialog.textValues().value(QStringLiteral("Headline")),
        QStringLiteral("LIVE HEADLINE"));
    text->clear();
    QVERIFY(!dialog.textValues().contains(QStringLiteral("Headline")));
}

void UiSmokeTest::titlePresetsApplySnapshots()
{
    mixvera::ui::MainWindow window(false);
    window.show();
    QTest::qWait(150);

    auto& inputs = mixvera::core::Application::instance().services()
                       .get<mixvera::services::InputService>();
    std::string titleId;
    for (const auto& input : inputs.inputs()) {
        if (input.kind == mixvera::core::InputKind::Title) {
            titleId = input.id;
            break;
        }
    }
    QVERIFY(!titleId.empty());

    auto title = *inputs.find(titleId);
    title.params["titlePreset:0:label"] = "ONE";
    title.params["titlePreset:0:titleText:Headline"] = "FIRST";
    title.params["titlePreset:1:label"] = "TWO";
    title.params["titlePreset:1:titleText:Headline"] = "SECOND";
    title.params["titlePresetActive"] = "0";
    title.params.erase("titleText:Headline");
    QVERIFY(inputs.update(title));

    const QString id = QString::fromStdString(titleId);
    QVERIFY(QMetaObject::invokeMethod(&window, "onTitleAction", Qt::DirectConnection,
        Q_ARG(QString, id), Q_ARG(QString, QStringLiteral("presetNext"))));
    const auto& applied = *inputs.find(titleId);
    QCOMPARE(QString::fromStdString(applied.params.at("titlePresetActive")), QString("1"));
    QCOMPARE(QString::fromStdString(applied.params.at("titleText:Headline")),
        QString("SECOND"));
    QVERIFY(applied.params.count("titlePresetChanged") == 1);

    QVERIFY(QMetaObject::invokeMethod(&window, "onTitleAction", Qt::DirectConnection,
        Q_ARG(QString, id), Q_ARG(QString, QStringLiteral("presetPrev"))));
    QCOMPARE(QString::fromStdString(inputs.find(titleId)->params.at("titleText:Headline")),
        QString("FIRST"));

    QVERIFY(QMetaObject::invokeMethod(&window, "onTitleAction", Qt::DirectConnection,
        Q_ARG(QString, id), Q_ARG(QString, QStringLiteral("presetClear"))));
    for (const auto& entry : inputs.find(titleId)->params) {
        QVERIFY(QString::fromStdString(entry.first)
                    .startsWith(QStringLiteral("titlePreset:"))
            == false);
    }
}

void UiSmokeTest::subtitleActionsStepPresets()
{
    mixvera::ui::MainWindow window(false);
    window.show();
    QTest::qWait(150);

    auto& inputs = mixvera::core::Application::instance().services()
                       .get<mixvera::services::InputService>();
    std::string titleId;
    for (const auto& input : inputs.inputs()) {
        if (input.kind == mixvera::core::InputKind::Title) {
            titleId = input.id;
            break;
        }
    }
    QVERIFY(!titleId.empty());

    auto title = *inputs.find(titleId);
    title.params["style"] = "caption";
    title.params["captions"] = "ONE\nTWO\nTHREE";
    title.params["text"] = "ONE";
    title.params["captionIndex"] = "0";
    QVERIFY(inputs.update(title));

    const QString id = QString::fromStdString(titleId);
    QVERIFY(QMetaObject::invokeMethod(&window, "onTitleAction", Qt::DirectConnection,
        Q_ARG(QString, id), Q_ARG(QString, QStringLiteral("captionNext"))));
    QCOMPARE(QString::fromStdString(inputs.find(titleId)->params.at("text")), QString("TWO"));

    QVERIFY(QMetaObject::invokeMethod(&window, "onTitleAction", Qt::DirectConnection,
        Q_ARG(QString, id), Q_ARG(QString, QStringLiteral("captionPrev"))));
    QCOMPARE(QString::fromStdString(inputs.find(titleId)->params.at("text")), QString("ONE"));

    QVERIFY(QMetaObject::invokeMethod(&window, "onTitleAction", Qt::DirectConnection,
        Q_ARG(QString, id), Q_ARG(QString, QStringLiteral("captionClear"))));
    QCOMPARE(QString::fromStdString(inputs.find(titleId)->params.at("text")), QString());
}

void UiSmokeTest::chromaKeyRoundTripsThroughDialog()
{
    mixvera::ui::AddInputDialog dialog;

    mixvera::core::InputDescriptor camera;
    camera.id = "camera";
    camera.name = "Camera";
    camera.kind = mixvera::core::InputKind::Camera;
    camera.params["device"] = "";
    camera.params["chromaEnabled"] = "1";
    camera.params["chromaPreset"] = "green";
    camera.params["chromaColor"] = "#00ff00";
    camera.params["chromaSimilarity"] = "40";
    camera.params["chromaSmoothness"] = "20";
    camera.params["chromaSpill"] = "70";
    dialog.loadFrom(camera);

    auto* enabled = dialog.findChild<QCheckBox*>("chromaEnabled");
    QVERIFY(enabled != nullptr);
    QVERIFY(enabled->isChecked());
    auto* preset = dialog.findChild<QComboBox*>("chromaPreset");
    QVERIFY(preset != nullptr);
    QCOMPARE(preset->currentData().toString(), QString("green"));

    preset->setCurrentIndex(preset->findData("blue"));
    const auto output = dialog.descriptor();
    QCOMPARE(output.kind, mixvera::core::InputKind::Camera);
    QCOMPARE(QString::fromStdString(output.params.at("chromaEnabled")), QString("1"));
    QCOMPARE(QString::fromStdString(output.params.at("chromaPreset")), QString("blue"));
    QCOMPARE(QString::fromStdString(output.params.at("chromaColor")), QString("#0000ff"));
    QCOMPARE(QString::fromStdString(output.params.at("chromaSimilarity")), QString("40"));

    mixvera::core::InputDescriptor title;
    title.id = "title";
    title.name = "Title";
    title.kind = mixvera::core::InputKind::Title;
    title.params["style"] = "caption";
    title.params["text"] = "ONE";
    title.params["captions"] = "ONE\nTWO";
    title.params["tickerSpeed"] = "200";
    dialog.loadFrom(title);
    const auto titleOutput = dialog.descriptor();
    QCOMPARE(QString::fromStdString(titleOutput.params.at("captions")), QString("ONE\nTWO"));
    QCOMPARE(QString::fromStdString(titleOutput.params.at("tickerSpeed")), QString("200"));
    QVERIFY(titleOutput.params.find("chromaEnabled") == titleOutput.params.end());
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