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
    void recorderWritesPlayableFile();
};

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