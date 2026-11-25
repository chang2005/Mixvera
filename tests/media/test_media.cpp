#include "mixvera/core/domain_types.hpp"
#include "mixvera/ui/media/chroma_key.hpp"
#include "mixvera/ui/media/compositor.hpp"
#include "mixvera/ui/media/recorder.hpp"
#include "mixvera/ui/media/qr_encoder.hpp"
#include "mixvera/ui/media/source_provider.hpp"
#include "mixvera/ui/media/title_package.hpp"
#include "mixvera/ui/platform/win_capture.hpp"
#include "../support/title_package_fixture.hpp"

#include <QApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
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
    void titlePackageRendersVmixTemplate();
    void titlePackageRendersSinSamTemplate();
    void titlePackageOverridesElements();
    void titlePackageLoadsLooseXmlAndFolder();
    void titlePackageCentersWideVerticalDesign();
    void titlePackageProviderRenders();
    void titlePackageCountdownFormatting();
    void titlePackageClockFormats();
    void qrEncodesRealMatrix();
    void titlePackageProviderMapsScoreAndClock();
    void titlePackagePlaysDataChangeStoryboard();
    void titleProviderUpdateKeepsAnimationClock();
    void countdownAndScoreDetails();
    void subtitleAndTickerFollowPortraitCanvas();
    void chromaKeyRemovesBackdrop();
    void compositorChromaKeyShowsLowerLayers();
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

void MediaTest::titlePackageRendersVmixTemplate()
{
    const QString root = QStringLiteral("C:/Program Files (x86)/vMix/Titles");
    if (!QDir(root).exists()) {
        QSKIP("vMix title templates are not installed");
    }
    QString path = QDir(root).filePath(
        QStringLiteral("GT Title/Title 1- Element Blue.gtzip"));
    if (!QFileInfo::exists(path)) {
        QDirIterator iterator(root, { QStringLiteral("*.gtzip") }, QDir::Files,
            QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            const QString candidate = iterator.next();
            if (candidate.contains(QStringLiteral("GT Title"))) {
                path = candidate;
                break;
            }
        }
    }
    if (!QFileInfo::exists(path)) {
        QSKIP("No GT Title package found");
    }

    const mixvera::ui::TitlePackage package = mixvera::ui::TitlePackage::load(path);
    QVERIFY2(package.isValid(), qPrintable(path));
    QVERIFY(!package.elements().isEmpty());
    QVERIFY(package.designSize().width() > 0);

    QHash<QString, QString> overrides;
    for (const auto& element : package.elements()) {
        if (element.type == QStringLiteral("text")) {
            overrides.insert(element.name, QStringLiteral("MIXVERA PACKAGE"));
            break;
        }
    }
    const QImage early = package.render(QSize(1920, 1080), overrides, {}, {}, 0.0, 0.0);
    const QImage frame = package.render(QSize(1920, 1080), overrides, {}, {}, 10.0, 4.0);
    QCOMPARE(frame.size(), QSize(1920, 1080));
    QVERIFY(visiblePixels(frame) > 20);
    QVERIFY2(visiblePixels(frame) > visiblePixels(early), "Entrance animation did not progress");
    QCOMPARE(qAlpha(frame.pixel(0, 0)), 0);

    const QImage portrait = package.render(QSize(1080, 1920), overrides, {}, {}, 10.0, 4.0);
    QCOMPARE(portrait.size(), QSize(1080, 1920));

    const QString probe = qEnvironmentVariable("MIXVERA_PACKAGE_PROBE");
    if (!probe.isEmpty()) {
        QVERIFY(frame.save(probe));
    }
}

void MediaTest::titlePackageRendersSinSamTemplate()
{
    const QString path = qEnvironmentVariable("MIXVERA_SINSAM_PACKAGE");
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        QSKIP("Set MIXVERA_SINSAM_PACKAGE to a SinSam .gtzip to run this compatibility check");
    }
    const mixvera::ui::TitlePackage package = mixvera::ui::TitlePackage::load(path);
    QVERIFY2(package.isValid(), qPrintable(path));
    QVERIFY(!package.elements().isEmpty());
    QVERIFY(package.designSize().width() > 0);
    QVERIFY(package.designSize().height() > 0);

    QHash<QString, QString> overrides;
    for (const auto& element : package.elements()) {
        if (element.type == QStringLiteral("text")) {
            overrides.insert(element.name, QStringLiteral("MIXVERA 竖屏标题"));
            break;
        }
    }
    const QImage frame = package.render(package.designSize(), overrides, {}, {}, 10.0, 0.0);
    QCOMPARE(frame.size(), package.designSize());
    QVERIFY(visiblePixels(frame) > 20);

    const QString probe = qEnvironmentVariable("MIXVERA_PACKAGE_PROBE");
    if (!probe.isEmpty()) {
        QVERIFY(frame.save(probe));
    }
}

void MediaTest::titlePackageOverridesElements()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = writeSyntheticPackage(directory);
    QVERIFY2(!path.isEmpty(), "Could not write synthetic title package");

    QString error;
    const mixvera::ui::TitlePackage package = mixvera::ui::TitlePackage::load(path, &error);
    QVERIFY2(package.isValid(), qPrintable(error));
    QCOMPARE(package.name(), QStringLiteral("synthetic"));
    QCOMPARE(package.designSize(), QSize(1280, 720));

    const auto elements = package.elements();
    QCOMPARE(elements.size(), 4);
    bool headlineFound = false;
    for (const auto& element : elements) {
        if (element.name == QStringLiteral("Headline")) {
            headlineFound = true;
            QCOMPARE(element.type, QStringLiteral("text"));
            QCOMPARE(element.value, QStringLiteral("DEFAULT HEADLINE"));
        }
    }
    QVERIFY(headlineFound);

    const QImage plain = package.render(QSize(1280, 720), {}, {}, {}, 10.0, 0.0);
    QCOMPARE(plain.size(), QSize(1280, 720));
    QVERIFY(visiblePixels(plain) > 100);
    QCOMPARE(qAlpha(plain.pixel(5, 5)), 255);

    const QHash<QString, QString> texts{
        { QStringLiteral("Headline"), QStringLiteral("OVERRIDE HEADLINE") },
        { QStringLiteral("Description"), QStringLiteral("Subtitle line") },
        { QStringLiteral("Ticker"), QStringLiteral("ROLLING NOW") },
    };
    const QHash<QString, QColor> colours{
        { QStringLiteral("Headline"), QColor("#ff0000") },
    };
    const QImage changed = package.render(QSize(1280, 720), texts, colours, {}, 10.0, 2.0);
    QCOMPARE(changed.size(), plain.size());
    int different = 0;
    for (int y = 0; y < plain.height(); y += 3) {
        for (int x = 0; x < plain.width(); x += 3) {
            if (plain.pixel(x, y) != changed.pixel(x, y)) {
                ++different;
            }
        }
    }
    QVERIFY2(different > 50, "Element overrides did not change the rendered package");

    const mixvera::ui::TitlePackage missing = mixvera::ui::TitlePackage::load(
        QDir(directory.path()).filePath(QStringLiteral("missing.gtzip")), &error);
    QVERIFY(!missing.isValid());
    QVERIFY(!error.isEmpty());
}

void MediaTest::titlePackageLoadsLooseXmlAndFolder()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = writeLoosePackage(directory);
    QVERIFY(!path.isEmpty());

    const mixvera::ui::TitlePackage loose = mixvera::ui::TitlePackage::load(path);
    QVERIFY(loose.isValid());
    QCOMPARE(loose.designSize(), QSize(1280, 720));
    QCOMPARE(loose.elements().size(), 4);
    QCOMPARE(loose.name(), QStringLiteral("loose"));
    const QImage frame = loose.render(QSize(1280, 720), {}, {}, {}, 10.0, 0.0);
    QVERIFY(visiblePixels(frame) > 100);

    const QString folder = QDir(directory.path()).filePath(QStringLiteral("unzipped"));
    QVERIFY(QDir().mkpath(folder));
    QFile document(QDir(folder).filePath(QStringLiteral("document.xml")));
    QVERIFY(document.open(QIODevice::WriteOnly));
    const QByteArray bytes = syntheticDocument();
    QCOMPARE(document.write(bytes), qint64(bytes.size()));
    document.close();

    const mixvera::ui::TitlePackage unzipped = mixvera::ui::TitlePackage::load(folder);
    QVERIFY(unzipped.isValid());
    QCOMPARE(unzipped.name(), QStringLiteral("unzipped"));
    QVERIFY(visiblePixels(unzipped.render(QSize(1280, 720), {}, {}, {}, 10.0, 0.0)) > 100);
}

void MediaTest::titlePackageCentersWideVerticalDesign()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = writeVerticalPackage(directory);
    QVERIFY(!path.isEmpty());
    const mixvera::ui::TitlePackage package = mixvera::ui::TitlePackage::load(path);
    QVERIFY(package.isValid());
    QCOMPARE(package.designSize(), QSize(3412, 1920));

    const QImage portrait = package.render(QSize(1080, 1920), {}, {}, {}, 10.0, 0.0);
    QCOMPARE(portrait.size(), QSize(1080, 1920));
    const QRgb topLeft = portrait.pixel(0, 0);
    QVERIFY2(qRed(topLeft) > 200 && qGreen(topLeft) < 40,
        "The central 1080-wide frame was not cropped and scaled 1:1");
    const QRgb bottomRight = portrait.pixel(1079, 1919);
    QVERIFY(qRed(bottomRight) > 200 && qGreen(bottomRight) < 40);
    const QRgb marker = portrait.pixel(590, 960);
    QVERIFY(qRed(marker) > 200 && qGreen(marker) > 200 && qBlue(marker) < 40);
    bool greenVisible = false;
    for (int y = 0; y < portrait.height(); y += 8) {
        for (int x = 0; x < portrait.width(); x += 8) {
            const QRgb pixel = portrait.pixel(x, y);
            if (qGreen(pixel) > 200 && qRed(pixel) < 40) {
                greenVisible = true;
                break;
            }
        }
        if (greenVisible) {
            break;
        }
    }
    QVERIFY2(!greenVisible, "Artboard content outside the portrait frame leaked into the render");

    const QImage landscape = package.render(QSize(1920, 1080), {}, {}, {}, 10.0, 0.0);
    const QRgb outside = landscape.pixel(10, 10);
    QVERIFY(qGreen(outside) > 200 && qRed(outside) < 40);
    const QRgb center = landscape.pixel(700, 500);
    QVERIFY(qRed(center) > 200 && qGreen(center) < 40);
}

void MediaTest::titlePackageProviderRenders()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = writeSyntheticPackage(directory);
    QVERIFY(!path.isEmpty());

    auto input = titleInput("package");
    input.params["package"] = path.toStdString();
    input.params["text"] = "PROVIDER HEADLINE";
    input.params["subtitle"] = "Provider subtitle";
    auto provider = mixvera::ui::createSourceProvider(input);
    QVERIFY(provider != nullptr);
    QVERIFY(provider->ready());
    QTest::qWait(80);
    const QImage frame = provider->frame();
    QCOMPARE(frame.size(), QSize(1280, 720));
    QVERIFY(visiblePixels(frame) > 100);
    QVERIFY2(provider->detail().startsWith("Title package:"),
        qPrintable(provider->detail()));

    provider->setCanvasSize(QSize(1080, 1920));
    const QImage portrait = provider->frame();
    QCOMPARE(portrait.size(), QSize(1080, 1920));
    QVERIFY(visiblePixels(portrait) > 100);
}

void MediaTest::titlePackageCountdownFormatting()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = writeScoreboardPackage(directory);
    QVERIFY(!path.isEmpty());
    const mixvera::ui::TitlePackage package = mixvera::ui::TitlePackage::load(path);
    QVERIFY(package.isValid());

    const auto compare = [&package](const QHash<QString, qint64>& remaining,
                             const QHash<QString, QString>& expectedTexts) {
        const QImage countdown = package.render(QSize(1280, 720), {}, {}, {}, 10.0, 0.0,
            remaining);
        const QImage expected = package.render(QSize(1280, 720), expectedTexts, {}, {},
            10.0, 0.0);
        return countdown == expected;
    };

    QVERIFY(compare({ { QStringLiteral("Game Clock"), 65000 },
                        { QStringLiteral("Short Clock"), 65000 } },
        { { QStringLiteral("Game Clock"), QStringLiteral("00:01:05") },
            { QStringLiteral("Short Clock"), QStringLiteral("01:05") } }));
    QVERIFY(compare({ { QStringLiteral("Game Clock"), 0 },
                        { QStringLiteral("Short Clock"), 0 } },
        { { QStringLiteral("Game Clock"), QStringLiteral("00:00:00") },
            { QStringLiteral("Short Clock"), QStringLiteral("00:00") } }));
    QVERIFY(compare({ { QStringLiteral("Game Clock"), 3661000 },
                        { QStringLiteral("Short Clock"), 3661000 } },
        { { QStringLiteral("Game Clock"), QStringLiteral("01:01:01") },
            { QStringLiteral("Short Clock"), QStringLiteral("01:01") } }));
}

void MediaTest::titlePackageClockFormats()
{
    const QDateTime now(QDate(2026, 11, 25), QTime(15, 4, 5, 123));
    using mixvera::ui::titlePackageClockText;

    QCOMPARE(titlePackageClockText(QStringLiteral("{0:HH:mm}"), -1, now),
        QStringLiteral("15:04"));
    QCOMPARE(titlePackageClockText(QStringLiteral("{0:hh:mm:ss tt}"), -1, now),
        QStringLiteral("03:04:05 PM"));
    QCOMPARE(titlePackageClockText(QStringLiteral("{0:h:mm:ss}"), -1, now),
        QStringLiteral("3:04:05"));
    QCOMPARE(titlePackageClockText(QStringLiteral("{0:hh:mm tt}"), -1, now),
        QStringLiteral("03:04 PM"));
    QCOMPARE(titlePackageClockText(QStringLiteral("{0:'AT' HH:mm}"), -1, now),
        QStringLiteral("AT 15:04"));
    QCOMPARE(titlePackageClockText(QStringLiteral("{0:4:00PM|mm:ss}"), 65000, now),
        QStringLiteral("01:05"));
    QCOMPARE(titlePackageClockText(QStringLiteral("{0:4:00PM|mm:ss}"), 3661000, now),
        QStringLiteral("01:01"));
    QCOMPARE(titlePackageClockText(QStringLiteral("{0:HH:mm}"), 3661000, now),
        QStringLiteral("01:01"));
    QCOMPARE(titlePackageClockText(QStringLiteral("{0:hh:mm:ss tt}"), 3661000, now),
        QStringLiteral("01:01:01"));
    const QDateTime midnight(QDate(2026, 11, 25), QTime(0, 0, 0));
    QCOMPARE(titlePackageClockText(QStringLiteral("{0:hh:mm tt}"), -1, midnight),
        QStringLiteral("12:00 AM"));
}

void MediaTest::qrEncodesRealMatrix()
{
    const QVector<QVector<bool>> modules = mixvera::ui::qrMatrix(QStringLiteral("vmix.com"));
    QCOMPARE(modules.size(), 21);
    for (int index = 0; index < 7; ++index) {
        // Three finder patterns: top-left, top-right and bottom-left.
        QVERIFY(modules.at(0).at(index));
        QVERIFY(modules.at(index).at(0));
        QVERIFY(modules.at(20).at(index));
        QVERIFY(modules.at(index).at(20));
    }
    QVERIFY(!modules.at(1).at(1));
    QVERIFY(modules.at(3).at(3));
    QCOMPARE(mixvera::ui::qrMatrix(QStringLiteral("vmix.com")), modules);

    const QVector<QVector<bool>> chinese
        = mixvera::ui::qrMatrix(QStringLiteral("芯像字幕包装"));
    QVERIFY(!chinese.isEmpty());
    QVERIFY(chinese.size() > 21);
    QVERIFY(mixvera::ui::qrMatrix(QString(300, QLatin1Char('x'))).isEmpty());
}

void MediaTest::titlePackageProviderMapsScoreAndClock()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = writeScoreboardPackage(directory);
    QVERIFY(!path.isEmpty());

    auto input = titleInput("package");
    input.params["package"] = path.toStdString();
    input.params.erase("text");
    input.params.erase("subtitle");
    input.params["homeName"] = "LIONS";
    input.params["awayName"] = "TIGERS";
    input.params["scoreHome"] = "3";
    input.params["scoreAway"] = "2";
    input.params["countdownRemaining"] = "65000";
    auto mapped = mixvera::ui::createSourceProvider(input);
    QVERIFY(mapped != nullptr);
    QVERIFY(mapped->ready());

    auto explicitInput = input;
    explicitInput.params["titleText:Home Team"] = "LIONS";
    explicitInput.params["titleText:Home Score"] = "3";
    explicitInput.params["titleText:Away Team"] = "TIGERS";
    explicitInput.params["titleText:Away Score"] = "2";
    explicitInput.params["titleText:Game Clock"] = "00:01:05";
    explicitInput.params["titleText:Short Clock"] = "01:05";
    auto explicitProvider = mixvera::ui::createSourceProvider(explicitInput);
    QVERIFY(explicitProvider != nullptr);
    QVERIFY(explicitProvider->ready());

    const QImage mappedFrame = mapped->frame();
    const QImage explicitFrame = explicitProvider->frame();
    QCOMPARE(mappedFrame.size(), QSize(1280, 720));
    QVERIFY(visiblePixels(mappedFrame) > 100);
    QCOMPARE(mappedFrame, explicitFrame);
}

void MediaTest::titlePackagePlaysDataChangeStoryboard()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = writeSyntheticPackage(directory);
    QVERIFY(!path.isEmpty());
    const mixvera::ui::TitlePackage package = mixvera::ui::TitlePackage::load(path);
    QVERIFY(package.isValid());

    const QHash<QString, QString> texts{ { QStringLiteral("Headline"),
        QStringLiteral("CHANGED HEADLINE") } };
    const QRect headline(140, 120, 1000, 160);
    const auto bright = [&headline](const QImage& image) {
        int count = 0;
        for (int y = headline.top(); y < headline.bottom(); y += 2) {
            for (int x = headline.left(); x < headline.right(); x += 2) {
                const QColor colour(image.pixel(x, y));
                if (colour.red() > 200 && colour.green() > 200 && colour.blue() > 200) {
                    ++count;
                }
            }
        }
        return count;
    };
    const QImage atStart = package.render(QSize(1280, 720), texts, {}, {}, 10.0, 0.0, {},
        0.0);
    const QImage after = package.render(QSize(1280, 720), texts, {}, {}, 10.0, 0.0, {},
        5.0);
    QCOMPARE(bright(atStart), 0);
    QVERIFY(bright(after) > 100);
}

void MediaTest::titleProviderUpdateKeepsAnimationClock()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = writeSyntheticPackage(directory);
    QVERIFY(!path.isEmpty());

    auto input = titleInput("package");
    input.params["package"] = path.toStdString();
    input.params.erase("text");
    input.params.erase("subtitle");

    auto provider = mixvera::ui::createSourceProvider(input);
    QVERIFY(provider != nullptr);
    QCOMPARE(visiblePixels(provider->frame()), 0);

    QTest::qWait(700);
    input.params["titleText:Headline"] = "UPDATED HEADLINE";
    QVERIFY(provider->update(input));
    const QImage frame = provider->frame();
    // An in-place update keeps the entrance animation clock past its 0.4 s
    // fade, so the new headline is visible immediately. Recreating the
    // provider would restart the fade from fully transparent.
    QVERIFY(visiblePixels(frame) > 100);
    auto fresh = mixvera::ui::createSourceProvider(input);
    QVERIFY(fresh != nullptr);
    QCOMPARE(visiblePixels(fresh->frame()), 0);
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

void MediaTest::subtitleAndTickerFollowPortraitCanvas()
{
    auto captionInput = titleInput("caption");
    captionInput.params["text"] = "MIXVERA SUBTITLE";
    auto caption = mixvera::ui::createSourceProvider(captionInput);
    QVERIFY(caption != nullptr);
    caption->setCanvasSize(QSize(1080, 1920));
    const QImage captionFrame = caption->frame();
    QCOMPARE(captionFrame.size(), QSize(1080, 1920));
    QVERIFY(visiblePixels(captionFrame) > 10);
    QVERIFY2(caption->detail().startsWith("Subtitle:"), qPrintable(caption->detail()));

    auto tickerInput = titleInput("ticker");
    tickerInput.params["text"] = "BREAKING NEWS · MIXVERA LIVE";
    tickerInput.params["tickerSpeed"] = "800";
    auto ticker = mixvera::ui::createSourceProvider(tickerInput);
    QVERIFY(ticker != nullptr);
    ticker->setCanvasSize(QSize(1080, 1920));
    QTest::qWait(350);
    const QImage tickerFrame = ticker->frame();
    QCOMPARE(tickerFrame.size(), QSize(1080, 1920));
    QVERIFY(visiblePixels(tickerFrame) > 10);
    QVERIFY2(ticker->detail().startsWith("Ticker:"), qPrintable(ticker->detail()));
}

void MediaTest::chromaKeyRemovesBackdrop()
{
    QImage frame(320, 180, QImage::Format_ARGB32);
    frame.fill(QColor("#00ff00"));
    {
        QPainter painter(&frame);
        painter.fillRect(QRect(80, 40, 160, 100), QColor("#ff0000"));
    }

    mixvera::ui::ChromaKeySettings settings;
    settings.enabled = true;
    settings.color = QColor("#00ff00");
    const QImage keyed = mixvera::ui::applyChromaKey(frame, settings);
    QCOMPARE(keyed.size(), frame.size());
    QCOMPARE(qAlpha(keyed.pixel(10, 10)), 0);
    QCOMPARE(qAlpha(keyed.pixel(160, 90)), 255);
    QVERIFY(qRed(keyed.pixel(160, 90)) > 200);

    frame.fill(QColor("#0000ff"));
    {
        QPainter painter(&frame);
        painter.fillRect(QRect(120, 50, 80, 80), QColor("#ffffff"));
    }
    settings.color = QColor("#0000ff");
    const QImage blueKeyed = mixvera::ui::applyChromaKey(frame, settings);
    QCOMPARE(qAlpha(blueKeyed.pixel(10, 10)), 0);
    QCOMPARE(qAlpha(blueKeyed.pixel(160, 90)), 255);
}

void MediaTest::compositorChromaKeyShowsLowerLayers()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = QDir(directory.path()).filePath("green-screen.png");
    {
        QImage image(320, 180, QImage::Format_ARGB32);
        image.fill(QColor("#00ff00"));
        QPainter painter(&image);
        painter.fillRect(QRect(80, 40, 160, 100), QColor("#e03030"));
        painter.end();
        QVERIFY(image.save(path));
    }

    mixvera::core::InputDescriptor background;
    background.id = "background";
    background.name = "Background";
    background.kind = mixvera::core::InputKind::Color;
    background.params["color"] = "#0000ff";

    mixvera::core::InputDescriptor camera;
    camera.id = "camera";
    camera.name = "Camera";
    camera.kind = mixvera::core::InputKind::Image;
    camera.params["file"] = path.toStdString();
    camera.params["chromaEnabled"] = "1";
    camera.params["chromaColor"] = "#00ff00";
    camera.params["chromaSimilarity"] = "35";
    camera.params["chromaSmoothness"] = "18";
    camera.params["chromaSpill"] = "60";

    mixvera::core::InputDescriptor scene;
    scene.id = "scene";
    scene.name = "Scene";
    scene.kind = mixvera::core::InputKind::Scene;
    mixvera::core::SceneItem backgroundItem;
    backgroundItem.inputId = "background";
    mixvera::core::SceneItem cameraItem;
    cameraItem.inputId = "camera";
    scene.items = {backgroundItem, cameraItem};

    mixvera::ui::Compositor compositor;
    compositor.setCanvasSize(QSize(320, 180));
    const std::vector<mixvera::core::InputDescriptor> inputs{background, camera, scene};
    compositor.refresh(inputs);
    const QImage rendered = compositor.renderInput(inputs, "scene");
    QCOMPARE(QColor(rendered.pixel(10, 10)).rgb(), QColor("#0000ff").rgb());
    const QColor subject(rendered.pixel(160, 90));
    QVERIFY2(subject.red() > 160, qPrintable(subject.name()));
    QVERIFY(subject.blue() < 100);
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