#include "mixvera/core/domain_types.hpp"
#include "mixvera/ui/media/compositor.hpp"
#include "mixvera/ui/media/recorder.hpp"
#include "mixvera/ui/media/source_provider.hpp"
#include "mixvera/ui/platform/win_capture.hpp"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QTemporaryDir>
#include <QtTest>

class MediaTest final : public QObject {
    Q_OBJECT

private slots:
    void screenCaptureProducesFrames();
    void windowEnumerationFindsWindows();
    void compositorRendersCapturedSource();
    void titleStylesRender();
    void countdownAndScoreDetails();
    void overlaysCompositeAboveBase();
    void flyAndCubeTransitionsRender();
    void recorderWritesPlayableFile();
};

namespace {

int visiblePixels(const QImage& image)
{
    int count = 0;
    for (int y = 0; y < image.height(); y += 4) {
        for (int x = 0; x < image.width(); x += 4) {
            if (qAlpha(image.pixel(x, y)) > 8) {
                ++count;
            }
        }
    }
    return count;
}

mixvera::core::InputDescriptor titleInput(const QString& style)
{
    mixvera::core::InputDescriptor input;
    input.id = "title-test";
    input.name = "Title";
    input.kind = mixvera::core::InputKind::Title;
    input.params["style"] = style.toStdString();
    input.params["text"] = "MIXVERA LIVE";
    input.params["subtitle"] = "Round 1";
    input.params["color"] = "#ff8a3d";
    input.params["size"] = "56";
    input.params["align"] = "center";
    input.params["countdownSeconds"] = "65";
    input.params["countdownRemaining"] = "65000";
    input.params["competition"] = "MIXVERA CUP";
    input.params["homeName"] = "HOME";
    input.params["awayName"] = "AWAY";
    input.params["scoreHome"] = "2";
    input.params["scoreAway"] = "3";
    return input;
}

} // namespace

void MediaTest::screenCaptureProducesFrames()
{
    if (QGuiApplication::screens().isEmpty()) {
        QSKIP("No screens available");
    }

    mixvera::core::InputDescriptor input;
    input.id = "screen-test";
    input.name = "Screen";
    input.kind = mixvera::core::InputKind::ScreenCapture;
    input.params["monitor"] = "0";

    auto provider = mixvera::ui::createSourceProvider(input);
    QVERIFY(provider != nullptr);
    QTest::qWait(150);
    const QImage frame = provider->frame();
    QVERIFY(!frame.isNull());
    QVERIFY(frame.width() > 100);
    QVERIFY(frame.height() > 100);
}

void MediaTest::windowEnumerationFindsWindows()
{
    const auto windows = mixvera::ui::listCaptureWindows();
    QVERIFY2(!windows.isEmpty(), "Expected at least one capturable top level window");
    for (const auto& window : windows) {
        QVERIFY(!window.title.isEmpty());
        QVERIFY(window.handle != 0);
    }
}

void MediaTest::compositorRendersCapturedSource()
{
    if (QGuiApplication::screens().isEmpty()) {
        QSKIP("No screens available");
    }

    mixvera::core::InputDescriptor screen;
    screen.id = "screen-test";
    screen.name = "Screen";
    screen.kind = mixvera::core::InputKind::ScreenCapture;
    screen.params["monitor"] = "0";

    mixvera::ui::Compositor compositor;
    compositor.setCanvasSize(QSize(640, 360));
    const std::vector<mixvera::core::InputDescriptor> inputs{screen};
    compositor.refresh(inputs);

    const QImage canvas = compositor.renderInput(inputs, "screen-test");
    QCOMPARE(canvas.size(), QSize(640, 360));

    // A rendered desktop must differ from the black fallback canvas.
    qint64 brightness = 0;
    for (int y = 0; y < canvas.height(); y += 8) {
        for (int x = 0; x < canvas.width(); x += 8) {
            brightness += qGray(canvas.pixel(x, y));
        }
    }
    QVERIFY2(brightness > 0, "Captured desktop rendered completely black");
}

void MediaTest::titleStylesRender()
{
    for (const QString& style : {QStringLiteral("full"), QStringLiteral("lower"),
             QStringLiteral("countdown"), QStringLiteral("scoreboard")}) {
        auto provider = mixvera::ui::createSourceProvider(titleInput(style));
        QVERIFY(provider != nullptr);
        const QImage frame = provider->frame();
        QVERIFY2(!frame.isNull(), qPrintable(style));
        QCOMPARE(frame.size(), QSize(1280, 720));
        QVERIFY2(visiblePixels(frame) > 10, qPrintable(style));
    }
}

void MediaTest::countdownAndScoreDetails()
{
    auto countdown = mixvera::ui::createSourceProvider(titleInput("countdown"));
    QVERIFY(countdown != nullptr);
    QVERIFY2(countdown->detail().contains("01:05"), qPrintable(countdown->detail()));

    auto scoreboard = mixvera::ui::createSourceProvider(titleInput("scoreboard"));
    QVERIFY(scoreboard != nullptr);
    QCOMPARE(scoreboard->detail(), QStringLiteral("Score: HOME 2 : 3 AWAY"));

    auto lower = mixvera::ui::createSourceProvider(titleInput("lower"));
    QVERIFY(lower != nullptr);
    QVERIFY(lower->detail().startsWith("Lower third:"));
}

void MediaTest::overlaysCompositeAboveBase()
{
    mixvera::core::InputDescriptor base;
    base.id = "base";
    base.name = "Base";
    base.kind = mixvera::core::InputKind::Color;
    base.params["color"] = "#0000ff";

    mixvera::core::InputDescriptor overlay;
    overlay.id = "overlay";
    overlay.name = "Overlay";
    overlay.kind = mixvera::core::InputKind::Color;
    overlay.params["color"] = "#ff0000";

    mixvera::ui::Compositor compositor;
    compositor.setCanvasSize(QSize(320, 180));
    const std::vector<mixvera::core::InputDescriptor> inputs{base, overlay};
    compositor.refresh(inputs);

    const QImage frame = compositor.renderInput(inputs, "base");
    QCOMPARE(QColor(frame.pixel(160, 90)).rgb(), QColor("#0000ff").rgb());

    mixvera::core::OverlayLayer layer;
    layer.layer = 1;
    layer.inputId = "overlay";
    layer.enabled = true;
    const QImage composed = compositor.applyOverlays(frame, inputs, {layer});
    QCOMPARE(QColor(composed.pixel(160, 90)).rgb(), QColor("#ff0000").rgb());

    layer.enabled = false;
    const QImage clean = compositor.applyOverlays(frame, inputs, {layer});
    QCOMPARE(QColor(clean.pixel(160, 90)).rgb(), QColor("#0000ff").rgb());
}

void MediaTest::flyAndCubeTransitionsRender()
{
    mixvera::ui::Compositor compositor;
    compositor.setCanvasSize(QSize(320, 180));
    QImage from(320, 180, QImage::Format_ARGB32_Premultiplied);
    QImage to(320, 180, QImage::Format_ARGB32_Premultiplied);
    from.fill(QColor("#ff0000"));
    to.fill(QColor("#0000ff"));

    for (auto kind : {mixvera::core::TransitionKind::Fly,
             mixvera::core::TransitionKind::Cube}) {
        for (float position : {0.35F, 0.65F}) {
            const QImage mixed = compositor.mix(from, to, kind, position);
            QCOMPARE(mixed.size(), QSize(320, 180));
            QVERIFY(mixed != from);
            QVERIFY(mixed != to);
        }
    }
}

void MediaTest::recorderWritesPlayableFile()
{
    if (!mixvera::ui::Recorder::available()) {
        QSKIP("ffmpeg is not available on PATH");
    }

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = QDir(directory.path()).filePath("capture.mp4");

    mixvera::ui::Recorder recorder;
    QString error;
    QVERIFY2(recorder.startRecording(path, QSize(320, 180), 30, &error), qPrintable(error));
    QVERIFY(recorder.recording());

    QImage frame(320, 180, QImage::Format_ARGB32);
    for (int index = 0; index < 30; ++index) {
        frame.fill(QColor(20 + index * 4, 40, 120));
        recorder.submit(frame);
        QTest::qWait(25);
    }
    recorder.stopRecording();
    QVERIFY(!recorder.recording());

    const QFileInfo info(path);
    QVERIFY2(info.exists(), "Recording file was not created");
    QVERIFY2(info.size() > 1024, "Recording file is suspiciously small");
}

QTEST_MAIN(MediaTest)
#include "test_media.moc"