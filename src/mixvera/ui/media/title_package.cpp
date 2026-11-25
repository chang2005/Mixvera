#include "mixvera/ui/media/title_package.hpp"

#include "mixvera/ui/media/qr_encoder.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontMetricsF>
#include <QLocale>
#include <QPainter>
#include <QPainterPath>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>
#include <QTime>
#include <QXmlStreamReader>

#include <zlib.h>

#include <algorithm>
#include <cmath>
#include <functional>

namespace mixvera::ui {

namespace {

constexpr qint64 kMaxPackageBytes = 64LL * 1024 * 1024;
constexpr quint32 kMaxEntryBytes = 32U * 1024 * 1024;
constexpr quint32 kMaxEntries = 2048;
constexpr qreal kPi = 3.14159265358979323846;

quint16 le16(const QByteArray& data, int offset)
{
    if (offset < 0 || offset + 2 > data.size()) {
        return 0;
    }
    return static_cast<quint16>(static_cast<unsigned char>(data.at(offset)))
        | (static_cast<quint16>(static_cast<unsigned char>(data.at(offset + 1))) << 8);
}

quint32 le32(const QByteArray& data, int offset)
{
    if (offset < 0 || offset + 4 > data.size()) {
        return 0;
    }
    return static_cast<quint32>(static_cast<unsigned char>(data.at(offset)))
        | (static_cast<quint32>(static_cast<unsigned char>(data.at(offset + 1))) << 8)
        | (static_cast<quint32>(static_cast<unsigned char>(data.at(offset + 2))) << 16)
        | (static_cast<quint32>(static_cast<unsigned char>(data.at(offset + 3))) << 24);
}

QString normalizedArchiveName(QString name)
{
    name.replace(QLatin1Char('\\'), QLatin1Char('/'));
    while (name.startsWith(QStringLiteral("./"))) {
        name.remove(0, 2);
    }
    return name;
}

/// Minimal read-only ZIP reader for GT title packages. It supports stored and
/// deflated entries and refuses encrypted, Zip64 or oversized archives.
class ZipArchive {
public:
    bool open(const QString& path, QString* error)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            if (error) {
                *error = QStringLiteral("Cannot open %1").arg(path);
            }
            return false;
        }
        if (file.size() <= 0 || file.size() > kMaxPackageBytes) {
            if (error) {
                *error = QStringLiteral("Title package is empty or too large");
            }
            return false;
        }
        data_ = file.readAll();
        const int archiveSize = static_cast<int>(data_.size());
        int eocd = -1;
        const int lowest = std::max(0, archiveSize - 22 - 0xFFFF);
        for (int offset = archiveSize - 22; offset >= lowest; --offset) {
            if (le32(data_, offset) == 0x06054b50U) {
                eocd = offset;
                break;
            }
        }
        if (eocd < 0) {
            if (error) {
                *error = QStringLiteral("Not a ZIP archive");
            }
            return false;
        }
        const quint32 count = le16(data_, eocd + 10);
        const quint32 directoryOffset = le32(data_, eocd + 16);
        if (count == 0 || count > kMaxEntries || directoryOffset >= data_.size()) {
            if (error) {
                *error = QStringLiteral("Unsupported ZIP directory");
            }
            return false;
        }
        int cursor = static_cast<int>(directoryOffset);
        for (quint32 index = 0; index < count; ++index) {
            if (cursor + 46 > data_.size() || le32(data_, cursor) != 0x02014b50U) {
                if (error) {
                    *error = QStringLiteral("Damaged ZIP directory");
                }
                return false;
            }
            const quint16 flags = le16(data_, cursor + 8);
            const quint16 method = le16(data_, cursor + 10);
            const quint32 compressed = le32(data_, cursor + 20);
            const quint32 uncompressed = le32(data_, cursor + 24);
            const quint16 nameLength = le16(data_, cursor + 28);
            const quint16 extraLength = le16(data_, cursor + 30);
            const quint16 commentLength = le16(data_, cursor + 32);
            const quint32 localOffset = le32(data_, cursor + 42);
            if ((flags & 1U) != 0 || compressed == 0xFFFFFFFFU
                || uncompressed == 0xFFFFFFFFU || localOffset == 0xFFFFFFFFU) {
                if (error) {
                    *error = QStringLiteral("Encrypted or Zip64 packages are not supported");
                }
                return false;
            }
            if (cursor + 46 + nameLength > data_.size()) {
                if (error) {
                    *error = QStringLiteral("Damaged ZIP entry name");
                }
                return false;
            }
            const QString name = normalizedArchiveName(
                QString::fromUtf8(data_.mid(cursor + 46, nameLength)));
            if (!name.isEmpty()) {
                Entry entry;
                entry.method = method;
                entry.compressed = compressed;
                entry.uncompressed = uncompressed;
                entry.offset = localOffset;
                entries_.insert(name, entry);
                lowerNames_.insert(name.toLower(), name);
            }
            cursor += 46 + nameLength + extraLength + commentLength;
        }
        return true;
    }

    bool contains(const QString& name) const
    {
        const QString key = normalizedArchiveName(name);
        return entries_.contains(key) || lowerNames_.contains(key.toLower());
    }

    QByteArray read(const QString& name) const
    {
        QString key = normalizedArchiveName(name);
        auto it = entries_.constFind(key);
        if (it == entries_.constEnd()) {
            const QString lower = lowerNames_.value(key.toLower());
            if (lower.isEmpty()) {
                return {};
            }
            key = lower;
            it = entries_.constFind(key);
            if (it == entries_.constEnd()) {
                return {};
            }
        }
        const Entry& entry = it.value();
        const int local = static_cast<int>(entry.offset);
        if (local + 30 > data_.size() || le32(data_, local) != 0x04034b50U) {
            return {};
        }
        const quint16 nameLength = le16(data_, local + 26);
        const quint16 extraLength = le16(data_, local + 28);
        const int start = local + 30 + nameLength + extraLength;
        if (start < 0 || entry.compressed > kMaxEntryBytes
            || start + static_cast<int>(entry.compressed) > data_.size()) {
            return {};
        }
        const QByteArray compressed = data_.mid(start, static_cast<int>(entry.compressed));
        if (entry.method == 0) {
            return entry.uncompressed == static_cast<quint32>(compressed.size()) ? compressed
                                                                                  : QByteArray();
        }
        if (entry.method != 8 || entry.uncompressed > kMaxEntryBytes) {
            return {};
        }
        z_stream stream{};
        if (inflateInit2(&stream, -MAX_WBITS) != Z_OK) {
            return {};
        }
        QByteArray output(static_cast<int>(entry.uncompressed), '\0');
        stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(compressed.constData()));
        stream.avail_in = static_cast<uInt>(compressed.size());
        stream.next_out = reinterpret_cast<Bytef*>(output.data());
        stream.avail_out = static_cast<uInt>(output.size());
        const int result = inflate(&stream, Z_FINISH);
        const qint64 produced = stream.total_out;
        inflateEnd(&stream);
        if (result != Z_STREAM_END) {
            return {};
        }
        output.resize(static_cast<int>(produced));
        return output;
    }

    QStringList names() const { return entries_.keys(); }

private:
    struct Entry {
        quint16 method = 0;
        quint32 compressed = 0;
        quint32 uncompressed = 0;
        quint32 offset = 0;
    };
    QByteArray data_;
    QHash<QString, Entry> entries_;
    QHash<QString, QString> lowerNames_;
};

struct BrushData {
    enum Kind { Solid, LinearGradient, RadialGradient, Transparent, Bitmap };
    Kind kind = Solid;
    QColor color = Qt::white;
    QVector<QPair<qreal, QColor>> stops;
    QString bitmapSource;
    QPointF start;
    QPointF end;
    bool hasCoordinates = false;
};

struct AnimationData {
    QString type;
    QString object;
    QString direction;
    QString interpolation;
    qreal delay = 0.0;
    qreal duration = 0.5;
    bool hasDuration = false;
    bool reverse = false;
};

struct NodeData {
    QString tag;
    QString name;
    QSizeF size;
    QPointF location;
    qreal opacity = 1.0;
    qreal rotation = 0.0;
    qreal radius = 0.0;
    bool visible = true;

    QString text;
    QString fontFamily;
    qreal fontSize = 32.0;
    bool bold = false;
    bool italic = false;
    bool wrap = true;
    bool uppercase = false;
    QString autoSize;
    QString textAlign;
    QString verticalAlign;
    qreal strokeThickness = 0.0;

    BrushData fill;
    BrushData stroke;
    QVector<NodeData> children;

    QString imageSource;
    QString imageStretch;
    QString tickerText;
    QString tickerDirection;
    qreal tickerSpeed = 4.0;
    QString qrText;
};

struct StoryboardData {
    QVector<AnimationData> enter;
    QVector<AnimationData> exit;
    QVector<AnimationData> dataChangeIn;
    QVector<AnimationData> dataChangeOut;
};

struct PackageModel {
    QSize designSize = QSize(1920, 1080);
    NodeData root;
    StoryboardData storyboard;
    QHash<QString, QString> resources;
    QHash<QString, QImage> images;
};

qreal number(const QString& value, qreal fallback = 0.0)
{
    bool ok = false;
    const qreal parsed = value.toDouble(&ok);
    return ok ? parsed : fallback;
}

qreal fraction(const QString& value)
{
    QString text = value.trimmed();
    bool percent = false;
    if (text.endsWith(QLatin1Char('%'))) {
        percent = true;
        text.chop(1);
    }
    qreal result = number(text, 0.0);
    if (percent) {
        result /= 100.0;
    }
    return result;
}

QPointF pointValue(const QString& value)
{
    const QStringList parts = value.split(QLatin1Char(','), Qt::SkipEmptyParts);
    if (parts.size() < 2) {
        return {};
    }
    return QPointF(number(parts[0]), number(parts[1]));
}

QSizeF sizeValue(const QString& value)
{
    const QStringList parts = value.split(QLatin1Char(','), Qt::SkipEmptyParts);
    if (parts.size() < 2) {
        return {};
    }
    return QSizeF(number(parts[0]), number(parts[1]));
}

QColor colorValue(const QString& value, const QColor& fallback = Qt::white)
{
    const QColor color(value.trimmed());
    return color.isValid() ? color : fallback;
}

QByteArray normalizedXml(const QByteArray& bytes)
{
    QByteArray xml = bytes;
    if (xml.startsWith("\xEF\xBB\xBF")) {
        xml.remove(0, 3);
    }
    if (xml.startsWith("<?xml")) {
        const int end = xml.indexOf("?>");
        if (end > 0) {
            xml.remove(0, end + 2);
        }
    }
    return xml;
}

bool truthy(const QString& value, bool fallback = true)
{
    if (value.isEmpty()) {
        return fallback;
    }
    return !(value.compare(QLatin1String("False"), Qt::CaseInsensitive) == 0
        || value.compare(QLatin1String("0"), Qt::CaseInsensitive) == 0
        || value.compare(QLatin1String("No"), Qt::CaseInsensitive) == 0);
}

qreal durationSeconds(const QString& value)
{
    const qreal seconds = number(value, 0.0);
    if (seconds > 50.0) {
        return seconds / 1000.0;
    }
    return seconds;
}

/// Small namespace-free XML tree. The stream API's readNextStartElement()
/// behaviour makes hand-written container parsers fragile, while GT documents
/// are tiny enough that building a complete tree is simpler and safer.
struct XmlElement {
    QString name;
    QHash<QString, QString> attributes;
    QString text;
    QVector<std::shared_ptr<XmlElement>> children;

    QString attribute(const char* key) const
    {
        return attributes.value(QLatin1String(key));
    }
};

using XmlElementPtr = std::shared_ptr<XmlElement>;

XmlElementPtr parseXmlTree(const QByteArray& bytes)
{
    QXmlStreamReader reader(normalizedXml(bytes));
    XmlElementPtr root;
    QVector<XmlElementPtr> stack;
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            auto element = std::make_shared<XmlElement>();
            element->name = reader.name().toString();
            for (const QXmlStreamAttribute& attribute : reader.attributes()) {
                element->attributes.insert(attribute.name().toString(),
                    attribute.value().toString());
            }
            if (stack.isEmpty()) {
                if (!root) {
                    root = element;
                }
            } else {
                stack.last()->children.append(element);
            }
            stack.append(element);
        } else if (reader.isEndElement()) {
            if (!stack.isEmpty()) {
                stack.removeLast();
            }
        } else if (reader.isCharacters() && !reader.isWhitespace() && !stack.isEmpty()) {
            stack.last()->text += reader.text().toString();
        }
    }
    return root;
}

const XmlElement* childNamed(const XmlElement& parent, const QString& name)
{
    for (const auto& child : parent.children) {
        if (child->name == name) {
            return child.get();
        }
    }
    return nullptr;
}

const XmlElement* childEndingWith(const XmlElement& parent, const QString& suffix)
{
    for (const auto& child : parent.children) {
        if (child->name.endsWith(suffix)) {
            return child.get();
        }
    }
    return nullptr;
}

/// GT ticker templates often wrap their headline in a Layer/Composition with
/// several text and image cells. The scrolling headline is the widest text
/// block; simple templates have just one TextBlock.
QString tickerTemplateText(const XmlElement& element)
{
    const XmlElement* best = nullptr;
    qreal bestWidth = -1.0;
    std::function<void(const XmlElement&)> visit = [&](const XmlElement& node) {
        if (node.name == QLatin1String("TextBlock")) {
            const qreal width = sizeValue(node.attribute("Dimensions")).width();
            if (!best || width > bestWidth) {
                best = &node;
                bestWidth = width;
            }
        }
        for (const auto& child : node.children) {
            visit(*child);
        }
    };
    visit(element);
    if (!best) {
        return {};
    }
    QString text = best->attribute("Text");
    for (const auto& child : best->children) {
        if (child->name == QLatin1String("TextBlock.Text")
            && !child->text.trimmed().isEmpty()) {
            text = child->text.trimmed();
        }
    }
    return text;
}

BrushData brushFrom(const XmlElement* element)
{
    BrushData brush;
    if (!element) {
        brush.kind = BrushData::Transparent;
        return brush;
    }
    const QString type = element->attribute("Type");
    const QString colour = element->attribute("Color");
    // WPF leaves a brush without a colour transparent; GT omits Color on
    // gradient-only and bitmap-only brushes, where the stops/image decide.
    brush.color = colour.isEmpty() ? QColor(0, 0, 0, 0) : colorValue(colour);
    const QString start = element->attribute("StartPoint");
    const QString end = element->attribute("EndPoint");
    brush.hasCoordinates = !start.isEmpty() || !end.isEmpty();
    brush.start = pointValue(start);
    brush.end = pointValue(end);
    if (type.compare(QLatin1String("Transparent"), Qt::CaseInsensitive) == 0) {
        brush.kind = BrushData::Transparent;
    } else if (type.compare(QLatin1String("Bitmap"), Qt::CaseInsensitive) == 0) {
        brush.kind = BrushData::Bitmap;
    } else if (type.compare(QLatin1String("LinearGradient"), Qt::CaseInsensitive) == 0) {
        brush.kind = BrushData::LinearGradient;
    } else if (type.compare(QLatin1String("RadialGradient"), Qt::CaseInsensitive) == 0) {
        brush.kind = BrushData::RadialGradient;
    }
    if (const XmlElement* stops = childNamed(*element, QStringLiteral("Brush.Stops"))) {
        for (const auto& stop : stops->children) {
            if (stop->name == QLatin1String("GradientStop")) {
                const QString stopColour = stop->attribute("Color");
                brush.stops.append({ fraction(stop->attribute("Position")),
                    stopColour.isEmpty() ? QColor(0, 0, 0, 0) : colorValue(stopColour) });
            }
        }
        std::stable_sort(brush.stops.begin(), brush.stops.end(),
            [](const auto& left, const auto& right) { return left.first < right.first; });
    }
    if (const XmlElement* bitmap = childNamed(*element, QStringLiteral("Brush.Bitmap"))) {
        if (const XmlElement* source = childNamed(*bitmap, QStringLiteral("Bitmap"))) {
            brush.bitmapSource = source->attribute("Source");
        }
    }
    if (brush.kind == BrushData::Solid) {
        // GT omits Type on most brushes. Stops or gradient coordinates imply a
        // linear gradient; a bitmap child implies a bitmap brush.
        if (!brush.stops.isEmpty() || brush.hasCoordinates) {
            brush.kind = BrushData::LinearGradient;
        } else if (!brush.bitmapSource.isEmpty()) {
            brush.kind = BrushData::Bitmap;
        }
    }
    if (brush.stops.isEmpty()) {
        brush.stops.append({ 0.0, brush.color });
    }
    return brush;
}

NodeData nodeFrom(const XmlElement& element)
{
    NodeData node;
    node.tag = element.name;
    node.name = element.attribute("Name");
    node.size = sizeValue(element.attribute("Dimensions"));
    node.location = pointValue(element.attribute("Location"));
    node.opacity = number(element.attribute("Opacity"), 1.0);
    if (node.opacity > 1.0) {
        node.opacity /= 100.0;
    }
    node.opacity = std::clamp(node.opacity, 0.0, 1.0);
    node.visible = truthy(element.attribute("Visible"), true);
    node.text = element.attribute("Text");
    node.fontFamily = element.attribute("FontFamily");
    node.fontSize = number(element.attribute("FontSize"), 32.0);
    const QString weight = element.attribute("FontWeight");
    node.bold = weight.compare(QLatin1String("Bold"), Qt::CaseInsensitive) == 0
        || weight.compare(QLatin1String("SemiBold"), Qt::CaseInsensitive) == 0
        || weight.toInt() >= 600;
    node.italic = element.attribute("FontStyle").compare(QLatin1String("Italic"),
        Qt::CaseInsensitive) == 0;
    const QString wrapping = element.attribute("TextWordWrapping");
    node.wrap = wrapping.isEmpty()
        || wrapping.compare(QLatin1String("NoWrap"), Qt::CaseInsensitive) != 0;
    node.uppercase = element.attribute("TextEffect").contains(QLatin1String("Uppercase"),
        Qt::CaseInsensitive);
    node.autoSize = element.attribute("AutoSize");
    node.textAlign = element.attribute("TextAlign");
    node.verticalAlign = element.attribute("VerticalAlign");
    node.strokeThickness = number(element.attribute("StrokeThickness"), 0.0);
    QString radius = element.attribute("Radius");
    if (radius.isEmpty()) {
        radius = element.attribute("RadiusX");
    }
    node.radius = number(radius.section(QLatin1Char(','), 0, 0), 0.0);
    if (element.attribute("Style").compare(QLatin1String("Square"),
            Qt::CaseInsensitive) == 0) {
        node.radius = 0.0;
    }
    node.tickerSpeed = number(element.attribute("Speed"), 4.0);
    node.tickerDirection = element.attribute("Direction");
    node.imageStretch = element.attribute("SizeMode");
    if (node.imageStretch.isEmpty()) {
        node.imageStretch = element.attribute("Stretch");
    }
    if (node.imageStretch.compare(QLatin1String("Stretch"), Qt::CaseInsensitive) == 0) {
        node.imageStretch = QStringLiteral("Fill");
    }
    node.qrText = element.attribute("Text");
    if (element.name == QLatin1String("Composition")) {
        const qreal width = number(element.attribute("Width"), 0.0);
        const qreal height = number(element.attribute("Height"), 0.0);
        if (width > 0 && height > 0) {
            node.size = QSizeF(width, height);
        }
    }
    for (const auto& childPtr : element.children) {
        const XmlElement& child = *childPtr;
        if (child.name == QLatin1String("Transform")) {
            // GT stores a WPF RotateTransform as "x,y,z" Euler angles in
            // radians; only the Z angle matters to this 2D renderer.
            const QStringList angles = child.attribute("Rotate")
                                           .split(QLatin1Char(','), Qt::SkipEmptyParts);
            const QString angle = angles.isEmpty() ? QString() : angles.last().trimmed();
            qreal radians = number(angle, 0.0);
            if (std::abs(radians) > 6.5) {
                // A few third-party templates write degrees despite the GT
                // schema; accept them rather than spinning the artwork.
                node.rotation = radians;
            } else {
                node.rotation = radians * 180.0 / kPi;
            }
        } else if (child.name == QLatin1String("TextBlock.Text")) {
            const QString value = child.text.trimmed();
            if (!value.isEmpty()) {
                node.text = value;
            }
        } else if (child.name.endsWith(QLatin1String(".Fill"))) {
            if (const XmlElement* brush = childNamed(child, QStringLiteral("Brush"))) {
                node.fill = brushFrom(brush);
            }
        } else if (child.name.endsWith(QLatin1String(".Stroke"))) {
            const qreal thickness = number(child.attribute("Thickness"), 0.0);
            if (thickness > 0) {
                node.strokeThickness = thickness;
            }
            if (const XmlElement* brush = childNamed(child, QStringLiteral("Brush"))) {
                node.stroke = brushFrom(brush);
            }
        } else if (child.name == QLatin1String("Image.Bitmap")) {
            if (const XmlElement* bitmap = childNamed(child, QStringLiteral("Bitmap"))) {
                node.imageSource = bitmap->attribute("Source");
            }
        } else if (child.name == QLatin1String("Ticker.Template")) {
            node.tickerText = tickerTemplateText(child);
        }
    }
    return node;
}

void storyboardFrom(const XmlElement& element, PackageModel& model)
{
    const QString type = element.attribute("Type").trimmed();
    QVector<AnimationData>* animations = nullptr;
    if (type.isEmpty()) {
        animations = &model.storyboard.enter;
    } else if (type.compare(QLatin1String("TransitionOut"), Qt::CaseInsensitive) == 0) {
        animations = &model.storyboard.exit;
    } else if (type.compare(QLatin1String("DataChangeIn"), Qt::CaseInsensitive) == 0) {
        animations = &model.storyboard.dataChangeIn;
    } else if (type.compare(QLatin1String("DataChangeOut"), Qt::CaseInsensitive) == 0) {
        animations = &model.storyboard.dataChangeOut;
    }
    // Continuous, Page1 and other storyboard kinds are authoring metadata;
    // they must not be mistaken for entrance animations.
    if (!animations) {
        return;
    }
    const XmlElement* list = childNamed(element, QStringLiteral("Storyboard.Animations"));
    if (!list) {
        return;
    }
    for (const auto& childPtr : list->children) {
        const XmlElement& child = *childPtr;
        AnimationData animation;
        animation.type = child.name;
        animation.object = child.attribute("Object");
        animation.direction = child.attribute("Direction");
        animation.interpolation = child.attribute("Interpolation");
        animation.delay = durationSeconds(child.attribute("Delay"));
        const QString duration = child.attribute("Duration");
        animation.hasDuration = !duration.isEmpty();
        animation.duration = animation.hasDuration ? durationSeconds(duration) : 0.5;
        animation.reverse = truthy(child.attribute("Reverse"), false);
        animations->append(animation);
    }
}

QVector<NodeData> visualChildren(const XmlElement& container, PackageModel& model);
NodeData layerFrom(const XmlElement& element, PackageModel& model);

NodeData layerFrom(const XmlElement& element, PackageModel& model)
{
    NodeData layer;
    layer.tag = QStringLiteral("Layer");
    layer.name = element.attribute("Name");
    layer.location = pointValue(element.attribute("Location"));
    layer.opacity = number(element.attribute("Opacity"), 1.0);
    if (layer.opacity > 1.0) {
        layer.opacity /= 100.0;
    }
    layer.opacity = std::clamp(layer.opacity, 0.0, 1.0);
    if (const XmlElement* composition = childNamed(element,
            QStringLiteral("Layer.Composition"))) {
        if (const XmlElement* root = childNamed(*composition, QStringLiteral("Composition"))) {
            const qreal width = number(root->attribute("Width"), 0.0);
            const qreal height = number(root->attribute("Height"), 0.0);
            if (width > 0 && height > 0) {
                layer.size = QSizeF(width, height);
            }
            layer.children = visualChildren(*root, model);
        }
    }
    return layer;
}

QVector<NodeData> visualChildren(const XmlElement& container, PackageModel& model)
{
    QVector<NodeData> result;
    for (const auto& childPtr : container.children) {
        const XmlElement& child = *childPtr;
        if (child.name == QLatin1String("Layer")) {
            result.append(layerFrom(child, model));
        } else if (child.name == QLatin1String("Storyboard")) {
            storyboardFrom(child, model);
        } else if (child.name == QLatin1String("Composition")) {
            NodeData node = nodeFrom(child);
            node.tag = QStringLiteral("Composition");
            node.children = visualChildren(child, model);
            result.append(node);
        } else if (!child.name.endsWith(QLatin1String(".Fill"))
            && !child.name.endsWith(QLatin1String(".Stroke"))
            && !child.name.endsWith(QLatin1String(".Effects"))
            && !child.name.endsWith(QLatin1String(".Mask"))
            && child.name != QLatin1String("Layer.Composition")
            && child.name != QLatin1String("Storyboard.Animations")) {
            result.append(nodeFrom(child));
        }
    }
    return result;
}

bool parseDocument(const QByteArray& bytes, PackageModel& model)
{
    const XmlElementPtr root = parseXmlTree(bytes);
    if (!root || root->name != QLatin1String("Composition")) {
        return false;
    }
    const int width = qRound(number(root->attribute("Width"), 1920.0));
    const int height = qRound(number(root->attribute("Height"), 1080.0));
    model.designSize = QSize(width > 0 ? width : 1920, height > 0 ? height : 1080);
    model.root = NodeData();
    model.root.tag = QStringLiteral("Composition");
    model.root.size = QSizeF(model.designSize);
    model.root.children = visualChildren(*root, model);
    return true;
}

void collectResources(const XmlElement& element, QHash<QString, QString>& resources)
{
    if (element.name == QLatin1String("resource")) {
        const QString filename = normalizedArchiveName(element.attribute("filename"));
        const XmlElement* source = childNamed(element, QStringLiteral("source"));
        const QString entry = source
            ? normalizedArchiveName(source->attribute("guid"))
            : QString();
        if (!filename.isEmpty() && !entry.isEmpty()) {
            resources.insert(filename, entry);
        }
    }
    for (const auto& child : element.children) {
        collectResources(*child, resources);
    }
}

void parseResources(const QByteArray& bytes, QHash<QString, QString>& resources)
{
    const XmlElementPtr root = parseXmlTree(bytes);
    if (root) {
        collectResources(*root, resources);
    }
}void collectImageSources(const NodeData& node, QSet<QString>& sources)
{
    if (node.fill.kind == BrushData::Bitmap && !node.fill.bitmapSource.isEmpty()) {
        sources.insert(node.fill.bitmapSource);
    }
    if (node.stroke.kind == BrushData::Bitmap && !node.stroke.bitmapSource.isEmpty()) {
        sources.insert(node.stroke.bitmapSource);
    }
    if (node.tag == QLatin1String("Image") && !node.imageSource.isEmpty()) {
        sources.insert(node.imageSource);
    }
    for (const NodeData& child : node.children) {
        collectImageSources(child, sources);
    }
}

QString resolveImageEntry(const QString& source, const PackageModel& model,
    const ZipArchive& archive)
{
    const QString key = normalizedArchiveName(source);
    const QString mapped = model.resources.value(key);
    if (!mapped.isEmpty() && archive.contains(mapped)) {
        return mapped;
    }
    if (archive.contains(key)) {
        return key;
    }
    for (auto it = model.resources.constBegin(); it != model.resources.constEnd(); ++it) {
        if (it.key().endsWith(key, Qt::CaseInsensitive)
            || key.endsWith(it.key(), Qt::CaseInsensitive)) {
            return it.value();
        }
    }
    const QStringList names = archive.names();
    for (const QString& name : names) {
        if (name.endsWith(key, Qt::CaseInsensitive) || key.endsWith(name, Qt::CaseInsensitive)) {
            return name;
        }
    }
    return {};
}

QColor solidColor(const BrushData& brush, const QColor& fallback = Qt::white)
{
    if (brush.kind == BrushData::Transparent) {
        return Qt::transparent;
    }
    if (!brush.stops.isEmpty()) {
        return brush.stops.at(brush.stops.size() / 2).second;
    }
    return brush.color.isValid() ? brush.color : fallback;
}

QColor effectiveColour(const NodeData& node, const BrushData& brush,
    const QHash<QString, QColor>& overrides, const QColor& fallback = Qt::white)
{
    const auto it = overrides.constFind(node.name);
    if (it != overrides.constEnd()) {
        return it.value();
    }
    return solidColor(brush, fallback);
}

QBrush makeBrush(const NodeData& node, const BrushData& brush, const QRectF& rect,
    const QHash<QString, QColor>& colourOverrides, const PackageModel& model)
{
    const auto override = colourOverrides.constFind(node.name);
    if (override != colourOverrides.constEnd()) {
        return QBrush(override.value());
    }
    switch (brush.kind) {
    case BrushData::Transparent:
        return Qt::NoBrush;
    case BrushData::LinearGradient: {
        QPointF start = QPointF(rect.left(), rect.top());
        QPointF end = QPointF(rect.left(), rect.bottom());
        if (brush.hasCoordinates) {
            start = QPointF(rect.left() + brush.start.x() * rect.width(),
                rect.top() + brush.start.y() * rect.height());
            end = QPointF(rect.left() + brush.end.x() * rect.width(),
                rect.top() + brush.end.y() * rect.height());
        }
        QLinearGradient gradient(start, end);
        for (const auto& stop : brush.stops) {
            gradient.setColorAt(std::clamp(stop.first, 0.0, 1.0), stop.second);
        }
        return QBrush(gradient);
    }
    case BrushData::RadialGradient: {
        QRadialGradient gradient(rect.center(), std::max(rect.width(), rect.height()) / 2.0);
        for (const auto& stop : brush.stops) {
            gradient.setColorAt(std::clamp(stop.first, 0.0, 1.0), stop.second);
        }
        return QBrush(gradient);
    }
    case BrushData::Bitmap: {
        const QImage image = model.images.value(brush.bitmapSource);
        return image.isNull() ? QBrush(brush.color) : QBrush(image);
    }
    case BrushData::Solid:
        break;
    }
    return QBrush(brush.color);
}

QFont textFont(const NodeData& node, const QString& text, qreal pixelSize)
{
    QString family = node.fontFamily;
    if (family.isEmpty()) {
        bool wide = false;
        for (const QChar character : text) {
            if (character.unicode() > 0x2E80) {
                wide = true;
                break;
            }
        }
        family = wide ? QStringLiteral("Microsoft YaHei UI") : QStringLiteral("Bahnschrift");
    }
    QStringList families{ family };
    // GT packages reference vMix/Designer fonts that may not be installed.
    // Keep the authored face first, then a metric-sensible substitute and CJK
    // fallbacks so Chinese titles never render as boxes.
    const QString lower = family.toLower();
    if (lower == QLatin1String("bebas neue")) {
        families << QStringLiteral("Bahnschrift SemiCondensed") << QStringLiteral("Impact");
    } else if (lower.contains(QLatin1String("century gothic"))) {
        families << QStringLiteral("Segoe UI") << QStringLiteral("Arial");
    } else if (lower.contains(QLatin1String("gill sans"))) {
        families << QStringLiteral("Segoe UI") << QStringLiteral("Arial");
    } else if (lower.contains(QLatin1String("rounded"))) {
        families << QStringLiteral("Segoe UI") << QStringLiteral("Arial");
    }
    families << QStringLiteral("Segoe UI") << QStringLiteral("Microsoft YaHei UI")
             << QStringLiteral("Noto Sans CJK SC");
    families.removeDuplicates();
    QFont font;
    font.setFamilies(families);
    font.setPixelSize(std::max(1, qRound(pixelSize)));
    font.setBold(node.bold);
    font.setItalic(node.italic);
    return font;
}

Qt::Alignment textAlignment(const NodeData& node)
{
    Qt::Alignment alignment = Qt::AlignLeft;
    const QString horizontal = node.textAlign.toLower();
    if (horizontal.contains(QLatin1String("center"))) {
        alignment |= Qt::AlignHCenter;
    } else if (horizontal.contains(QLatin1String("right"))) {
        alignment |= Qt::AlignRight;
    } else if (horizontal.contains(QLatin1String("justify"))) {
        alignment |= Qt::AlignJustify;
    }
    const QString vertical = node.verticalAlign.toLower();
    if (vertical.contains(QLatin1String("bottom"))) {
        alignment |= Qt::AlignBottom;
    } else {
        alignment |= Qt::AlignTop;
    }
    return alignment;
}

struct ClockParts {
    int hour = 0;      // 0-23, or total hours for a duration
    int minute = 0;
    int second = 0;
    int millisecond = 0;
    bool wallClock = true;
    QDateTime dateTime;
};

int integerRun(const QString& format, int start, QChar token)
{
    int index = start;
    while (index < format.size() && format.at(index) == token) {
        ++index;
    }
    return index - start;
}

QString padded(int value, int width)
{
    return QStringLiteral("%1").arg(value, width, 10, QLatin1Char('0'));
}

/// Evaluates the .NET custom date/time format subset GT titles use, including
/// 12-hour clocks, tt AM/PM designators, quoted/escaped literals and short or
/// long date fields. vMix writes these formats with .NET semantics, so Qt's
/// own format letters cannot be used directly (Qt has no "t" designator).
QString netClockFormat(const QString& format, const ClockParts& parts)
{
    QString output;
    const bool am = parts.hour < 12;
    for (int index = 0; index < format.size();) {
        const QChar token = format.at(index);
        if (token == QLatin1Char('\\') && index + 1 < format.size()) {
            output += format.at(index + 1);
            index += 2;
            continue;
        }
        if (token == QLatin1Char('\'') || token == QLatin1Char('"')) {
            const int end = format.indexOf(token, index + 1);
            if (end < 0) {
                output += format.mid(index + 1);
                break;
            }
            output += format.mid(index + 1, end - index - 1);
            index = end + 1;
            continue;
        }
        int run = 1;
        if (token == QLatin1Char('%')) {
            if (index + 1 >= format.size()) {
                break;
            }
            ++index;
            // A % prefix forces a single-letter custom specifier.
            run = 1;
        } else if (token.isLetter()) {
            run = integerRun(format, index, token);
        }
        const QChar lower = token.toLower();
        if (token == QLatin1Char('H')) {
            output += run >= 2 ? padded(parts.hour, run) : QString::number(parts.hour);
        } else if (lower == QLatin1Char('h')) {
            const int hour12 = ((parts.hour + 11) % 12) + 1;
            output += run >= 2 ? padded(hour12, run) : QString::number(hour12);
        } else if (token == QLatin1Char('M')) {
            const QDate date = parts.dateTime.date();
            if (run <= 2) {
                output += run == 2 ? padded(date.month(), 2) : QString::number(date.month());
            } else if (run == 3) {
                output += QLocale::system().monthName(date.month(), QLocale::ShortFormat);
            } else {
                output += QLocale::system().monthName(date.month(), QLocale::LongFormat);
            }
        } else if (lower == QLatin1Char('m')) {
            output += run >= 2 ? padded(parts.minute, run) : QString::number(parts.minute);
        } else if (lower == QLatin1Char('s')) {
            output += run >= 2 ? padded(parts.second, run) : QString::number(parts.second);
        } else if (lower == QLatin1Char('f')) {
            // fraction of a second: f = tenths, ff = hundredths, fff = ms
            int digits = run;
            int value = parts.millisecond;
            if (digits <= 0) {
                digits = 1;
            }
            if (digits < 3) {
                value /= static_cast<int>(std::pow(10.0, 3 - digits));
            }
            QString fraction = padded(value, digits < 3 ? digits : 3);
            if (digits > 3) {
                fraction += QString(digits - 3, QLatin1Char('0'));
            }
            if (token == QLatin1Char('F')) {
                while (fraction.endsWith(QLatin1Char('0'))) {
                    fraction.chop(1);
                }
            }
            output += fraction;
        } else if (lower == QLatin1Char('t')) {
            if (parts.wallClock) {
                output += run >= 2 ? (am ? QStringLiteral("AM") : QStringLiteral("PM"))
                                   : (am ? QStringLiteral("A") : QStringLiteral("P"));
            } else if (output.endsWith(QLatin1Char(' '))) {
                // A countdown has no AM/PM designator; drop the separator that
                // the authored wall-clock format placed before it.
                output.chop(1);
            }
        } else if (lower == QLatin1Char('d')) {
            const QDate date = parts.dateTime.date();
            if (run <= 2) {
                output += run == 2 ? padded(date.day(), 2) : QString::number(date.day());
            } else if (run == 3) {
                output += QLocale::system().dayName(date.dayOfWeek(), QLocale::ShortFormat);
            } else {
                output += QLocale::system().dayName(date.dayOfWeek(), QLocale::LongFormat);
            }
        } else if (lower == QLatin1Char('y')) {
            const int year = parts.dateTime.date().year();
            if (run == 1) {
                output += QString::number(year);
            } else if (run == 2) {
                output += padded(year % 100, 2);
            } else {
                output += padded(year, run);
            }
        } else {
            output += format.mid(index, run);
        }
        index += run;
    }
    return output;
}

QString timerText(const QString& value, const qint64* countdownMilliseconds,
    const QDateTime& current = QDateTime())
{
    static const QRegularExpression expression(QStringLiteral("\\{0(?::([^}]*))?\\}"));
    auto matches = expression.globalMatch(value);
    if (!matches.hasNext()) {
        return value;
    }
    QString result;
    int cursor = 0;
    const QDateTime now = current.isValid() ? current : QDateTime::currentDateTime();
    const qint64 remaining = countdownMilliseconds
        ? (std::max<qint64>(*countdownMilliseconds, 0) + 999) / 1000
        : 0;
    while (matches.hasNext()) {
        const auto match = matches.next();
        QString format = match.captured(1).trimmed();
        if (format.contains(QLatin1Char('|'))) {
            // vMix lists alternative clock formats separated by |; the final
            // segment is the one used while a countdown is running.
            format = format.section(QLatin1Char('|'), -1).trimmed();
        }
        QString replacement;
        if (countdownMilliseconds) {
            ClockParts parts;
            parts.hour = static_cast<int>(remaining / 3600);
            parts.minute = static_cast<int>((remaining / 60) % 60);
            parts.second = static_cast<int>(remaining % 60);
            parts.wallClock = false;
            parts.dateTime = now;
            replacement = netClockFormat(format.isEmpty()
                    ? QStringLiteral("HH:mm:ss") : format, parts);
        } else {
            ClockParts parts;
            const QTime time = now.time();
            parts.hour = time.hour();
            parts.minute = time.minute();
            parts.second = time.second();
            parts.millisecond = time.msec();
            parts.wallClock = true;
            parts.dateTime = now;
            replacement = netClockFormat(format.isEmpty()
                    ? QStringLiteral("HH:mm:ss") : format, parts);
        }
        result += value.mid(cursor, match.capturedStart() - cursor);
        result += replacement;
        cursor = match.capturedEnd();
    }
    result += value.mid(cursor);
    return result;
}

struct AnimationState {
    qreal opacity = 1.0;
    QPointF offset;
    qreal scale = 1.0;
    qreal rotation = 0.0;
    qreal reveal = 1.0;
    QString revealDirection = QStringLiteral("Left");
    QPointF scaleAnchor = QPointF(0.5, 0.5);
};

qreal easedProgress(qreal progress, const QString& interpolation)
{
    const QString kind = interpolation.toLower();
    if (kind.contains(QLatin1String("linear"))) {
        return progress;
    }
    if (kind.contains(QLatin1String("inout"))) {
        return progress < 0.5 ? 4.0 * progress * progress * progress
                              : 1.0 - std::pow(-2.0 * progress + 2.0, 3.0) / 2.0;
    }
    if (kind.contains(QLatin1String("out"))) {
        return 1.0 - std::pow(1.0 - progress, 3.0);
    }
    if (kind.contains(QLatin1String("in"))) {
        return progress * progress * progress;
    }
    return progress * progress * (3.0 - 2.0 * progress);
}

QPointF scaleAnchorForDirection(const QString& direction)
{
    const QString value = direction.toLower();
    qreal x = 0.5;
    qreal y = 0.5;
    if (value.contains(QLatin1String("left"))) {
        x = 0.0;
    } else if (value.contains(QLatin1String("right"))) {
        x = 1.0;
    }
    if (value.contains(QLatin1String("top"))) {
        y = 0.0;
    } else if (value.contains(QLatin1String("bottom"))) {
        y = 1.0;
    }
    return QPointF(x, y);
}

AnimationState animationFor(const QString& name, const QVector<AnimationData>& animations,
    qreal seconds, const QSizeF& design)
{
    AnimationState state;
    if (name.isEmpty()) {
        return state;
    }
    for (const AnimationData& animation : animations) {
        if (animation.object != name) {
            continue;
        }
        const qreal duration = std::max(animation.duration, 0.001);
        qreal progress = std::clamp((seconds - animation.delay) / duration, 0.0, 1.0);
        if (animation.reverse) {
            progress = 1.0 - progress;
        }
        const qreal eased = easedProgress(progress, animation.interpolation);
        const QString direction = animation.direction.toLower();
        if (animation.type == QLatin1String("Fade")) {
            state.opacity *= eased;
        } else if (animation.type == QLatin1String("Fly")) {
            // Fly moves the element in from outside the design canvas. GT
            // defaults to Bottom when no direction is authored.
            const bool left = direction.contains(QLatin1String("left"));
            const bool right = direction.contains(QLatin1String("right"));
            const bool top = direction.contains(QLatin1String("top"));
            const bool bottom = direction.contains(QLatin1String("bottom"));
            const qreal hidden = 1.0 - eased;
            if (left) {
                state.offset.rx() -= hidden * design.width();
            }
            if (right) {
                state.offset.rx() += hidden * design.width();
            }
            if (top) {
                state.offset.ry() -= hidden * design.height();
            }
            if (bottom || (!top && !bottom)) {
                state.offset.ry() += hidden * design.height();
            }
        } else if (animation.type == QLatin1String("Reveal")) {
            if (eased < state.reveal) {
                state.reveal = eased;
                state.revealDirection = animation.direction.isEmpty()
                    ? QStringLiteral("Left") : animation.direction;
            }
        } else if (animation.type == QLatin1String("Expand")
            || animation.type == QLatin1String("ZoomFade")) {
            state.scale *= std::max(eased, 0.001);
            state.scaleAnchor = scaleAnchorForDirection(animation.direction);
            if (animation.type == QLatin1String("ZoomFade")) {
                state.opacity *= eased;
            }
        } else if (animation.type == QLatin1String("Zoom")) {
            state.scale *= std::max(eased, 0.001);
            state.scaleAnchor = QPointF(0.5, 0.5);
        } else if (animation.type == QLatin1String("Bounce")) {
            const qreal c1 = 1.70158;
            const qreal c3 = c1 + 1.0;
            const qreal t = eased - 1.0;
            state.scale *= std::max(1.0 + c3 * t * t * t + c1 * t * t, 0.001);
            state.scaleAnchor = scaleAnchorForDirection(animation.direction);
        } else if (animation.type == QLatin1String("Rotate")) {
            state.rotation += (1.0 - eased) * 90.0;
        }
    }
    return state;
}

struct RenderContext {
    const PackageModel& model;
    const QHash<QString, QString>& textOverrides;
    const QHash<QString, QColor>& colourOverrides;
    const QHash<QString, QImage>& imageOverrides;
    const QHash<QString, qint64>& countdownMilliseconds;
    const QVector<AnimationData>& animations;
    qreal animationSeconds;
    qreal tickerSeconds;
};

void drawNode(QPainter& painter, const NodeData& node, const RenderContext& context);

void paintImage(QPainter& painter, const QRectF& rect, const QImage& image,
    const QString& stretch)
{
    if (image.isNull() || rect.isEmpty()) {
        return;
    }
    const QString mode = stretch.toLower();
    if (mode == QLatin1String("fill")) {
        painter.drawImage(rect, image);
        return;
    }
    if (mode == QLatin1String("uniform") || mode.isEmpty()) {
        QSizeF scaled = image.size();
        scaled.scale(rect.size(), Qt::KeepAspectRatio);
        const QRectF target(rect.center().x() - scaled.width() / 2.0,
            rect.center().y() - scaled.height() / 2.0, scaled.width(), scaled.height());
        painter.drawImage(target, image);
        return;
    }
    // UniformToFill and unrecognised modes cover the element.
    const qreal scale = std::max(rect.width() / image.width(), rect.height() / image.height());
    const qreal sourceWidth = rect.width() / scale;
    const qreal sourceHeight = rect.height() / scale;
    const QRectF source((image.width() - sourceWidth) / 2.0, (image.height() - sourceHeight) / 2.0,
        sourceWidth, sourceHeight);
    painter.drawImage(rect, image, source);
}

void drawTextNode(QPainter& painter, const NodeData& node, const RenderContext& context)
{
    QString text = context.textOverrides.value(node.name);
    if (text.isEmpty()) {
        text = node.text;
    }
    const auto countdown = context.countdownMilliseconds.constFind(node.name);
    text = timerText(text,
        countdown == context.countdownMilliseconds.constEnd() ? nullptr : &countdown.value());
    if (node.uppercase) {
        text = text.toUpper();
    }
    if (text.isEmpty()) {
        return;
    }
    const QRectF rect(0, 0, node.size.width(), node.size.height());
    if (rect.isEmpty()) {
        return;
    }
    const Qt::Alignment alignment = textAlignment(node);
    int flags = static_cast<int>(alignment);
    if (node.wrap) {
        flags |= Qt::TextWordWrap;
    } else {
        flags |= Qt::TextSingleLine;
    }
    qreal pixelSize = node.fontSize > 0 ? node.fontSize : 32.0;
    QFont font = textFont(node, text, pixelSize);
    if (node.autoSize.contains(QLatin1String("Shrink"), Qt::CaseInsensitive)) {
        for (int iteration = 0; iteration < 60; ++iteration) {
            const QFontMetricsF metrics(font);
            const QRectF bounds = metrics.boundingRect(rect, flags, text);
            if ((bounds.height() <= rect.height() + 0.5
                    && (bounds.width() <= rect.width() + 0.5 || !node.wrap))
                || pixelSize <= 4.0) {
                break;
            }
            pixelSize *= 0.93;
            font.setPixelSize(std::max(1, qRound(pixelSize)));
        }
    }
    painter.setFont(font);
    if (node.strokeThickness > 0 && node.stroke.kind != BrushData::Transparent) {
        const QColor stroke = effectiveColour(node, node.stroke, context.colourOverrides,
            Qt::black);
        if (stroke.alpha() > 0) {
            painter.setPen(stroke);
            const qreal thickness = std::max(0.5, node.strokeThickness);
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    if (dx == 0 && dy == 0) {
                        continue;
                    }
                    painter.drawText(rect.translated(dx * thickness, dy * thickness),
                        flags, text);
                }
            }
        }
    }
    const QColor fill = effectiveColour(node, node.fill, context.colourOverrides, Qt::white);
    if (fill.alpha() == 0) {
        return;
    }
    painter.setPen(fill);
    painter.drawText(rect, flags, text);
}

void drawTickerNode(QPainter& painter, const NodeData& node, const RenderContext& context)
{
    QString text = context.textOverrides.value(node.name);
    if (text.isEmpty()) {
        text = node.tickerText;
    }
    if (text.isEmpty()) {
        text = node.text;
    }
    const auto countdown = context.countdownMilliseconds.constFind(node.name);
    text = timerText(text,
        countdown == context.countdownMilliseconds.constEnd() ? nullptr : &countdown.value());
    if (text.isEmpty()) {
        return;
    }
    text.replace(QLatin1Char('\n'), QStringLiteral("  •  "));
    const QRectF rect(0, 0, node.size.width(), node.size.height());
    if (rect.isEmpty()) {
        return;
    }
    const QFont font = textFont(node, text, node.fontSize > 0 ? node.fontSize : 40.0);
    painter.setFont(font);
    const QFontMetricsF metrics(font);
    const qreal advance = metrics.horizontalAdvance(text);
    const QString direction = node.tickerDirection.toLower();
    const bool vertical = direction.contains(QLatin1String("top"))
        || direction.contains(QLatin1String("bottom"))
        || direction.contains(QLatin1String("up"))
        || direction.contains(QLatin1String("down"))
        || direction.contains(QLatin1String("vertical"));
    const qreal length = vertical ? rect.height() : rect.width();
    const qreal gap = std::max(24.0, length * 0.04);
    const qreal travel = advance + gap;
    qreal speed = node.tickerSpeed > 0 ? node.tickerSpeed : 4.0;
    speed = std::clamp(speed * 30.0, 20.0, 1200.0);
    const qreal offset = std::fmod(std::max(0.0, context.tickerSeconds) * speed, travel);
    painter.save();
    painter.setClipRect(rect);
    const QColor fill = effectiveColour(node, node.fill, context.colourOverrides, Qt::white);
    painter.setPen(fill);
    if (vertical) {
        // Vertical strips (Direction="Top") scroll the headline from the
        // bottom edge upward, with the glyphs rotated to read along the strip.
        painter.translate(rect.center());
        painter.rotate(-90.0);
        painter.translate(-rect.center());
        const QRectF local(0, 0, rect.height(), rect.width());
        const bool towardsTop = !direction.contains(QLatin1String("bottom"))
            && !direction.contains(QLatin1String("down"));
        const qreal x = towardsTop ? local.width() - offset : offset - advance;
        const auto paintAt = [&](qreal position) {
            painter.drawText(QRectF(position, 0, advance + 4, local.height()),
                Qt::AlignLeft | Qt::AlignVCenter, text);
        };
        paintAt(x);
        if (x + advance < local.width()) {
            paintAt(x + travel);
        }
        if (x > 0) {
            paintAt(x - travel);
        }
    } else {
        const bool rightToLeft = !direction.contains(QLatin1String("right"));
        const qreal x = rightToLeft ? rect.width() - offset : offset - advance;
        const auto paintAt = [&](qreal position) {
            painter.drawText(QRectF(position, 0, advance + 4, rect.height()),
                Qt::AlignLeft | Qt::AlignVCenter, text);
        };
        paintAt(x);
        if (x + advance < rect.width()) {
            paintAt(x + travel);
        }
        if (x > 0) {
            paintAt(x - travel);
        }
    }
    painter.restore();
}

void drawQrNode(QPainter& painter, const NodeData& node, const RenderContext& context)
{
    const QRectF rect(0, 0, node.size.width(), node.size.height());
    if (rect.isEmpty()) {
        return;
    }
    const qreal side = std::min(rect.width(), rect.height());
    const QRectF square(rect.center().x() - side / 2.0, rect.center().y() - side / 2.0, side,
        side);
    painter.fillRect(square, Qt::white);
    QString content = context.textOverrides.value(node.name);
    if (content.isEmpty()) {
        content = node.qrText;
    }
    const QVector<QVector<bool>> modules = qrMatrix(content);
    if (modules.isEmpty()) {
        // The 64-bit build encodes byte mode up to version 10; longer payloads
        // are marked instead of drawing a pattern that would not scan.
        painter.setPen(QColor(0xB0, 0x00, 0x20));
        painter.drawText(square, Qt::AlignCenter, QStringLiteral("QR"));
        return;
    }
    const int quiet = 4; // ISO/IEC 18004 quiet zone
    const int count = modules.size();
    const qreal cell = side / (count + quiet * 2);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);
    for (int row = 0; row < count; ++row) {
        for (int column = 0; column < count; ++column) {
            if (!modules.at(row).at(column)) {
                continue;
            }
            painter.drawRect(QRectF(square.left() + (quiet + column) * cell,
                square.top() + (quiet + row) * cell, cell + 0.15, cell + 0.15));
        }
    }
}

void drawNode(QPainter& painter, const NodeData& node, const RenderContext& context)
{
    if (!node.visible) {
        return;
    }
    const AnimationState animation = animationFor(node.name, context.animations,
        context.animationSeconds, QSizeF(context.model.designSize));
    if (animation.opacity <= 0.001 || animation.reveal <= 0.001) {
        return;
    }
    painter.save();
    painter.translate(node.location + animation.offset);
    const bool container = node.tag == QLatin1String("Layer")
        || node.tag == QLatin1String("Composition");
    const QRectF rect(0, 0, node.size.width(), node.size.height());
    if (!rect.isEmpty()) {
        // GT storyboards animate layers and compositions as often as they
        // animate leaves, so the same transform applies to either kind.
        painter.translate(rect.center());
        painter.rotate(node.rotation + animation.rotation);
        painter.translate(-rect.center());
        if (animation.scale != 1.0) {
            const QPointF anchor(rect.left() + animation.scaleAnchor.x() * rect.width(),
                rect.top() + animation.scaleAnchor.y() * rect.height());
            painter.translate(anchor);
            painter.scale(animation.scale, animation.scale);
            painter.translate(-anchor);
        }
        if (animation.reveal < 0.999) {
            const QString reveal = animation.revealDirection.toLower();
            QRectF clip = rect;
            if (reveal.contains(QLatin1String("center"))) {
                clip = QRectF(rect.center().x() - rect.width() * animation.reveal / 2.0,
                    rect.center().y() - rect.height() * animation.reveal / 2.0,
                    rect.width() * animation.reveal, rect.height() * animation.reveal);
            } else if (reveal.contains(QLatin1String("right"))) {
                clip.setLeft(rect.right() - rect.width() * animation.reveal);
            } else if (reveal.contains(QLatin1String("top"))) {
                clip.setHeight(rect.height() * animation.reveal);
            } else if (reveal.contains(QLatin1String("bottom"))) {
                clip.setTop(rect.bottom() - rect.height() * animation.reveal);
            } else {
                clip.setWidth(rect.width() * animation.reveal);
            }
            painter.setClipRect(clip);
        }
        painter.setOpacity(painter.opacity() * node.opacity * animation.opacity);
    } else {
        painter.setOpacity(painter.opacity() * node.opacity * animation.opacity);
    }

    if (container) {
        for (const NodeData& child : node.children) {
            drawNode(painter, child, context);
        }
        painter.restore();
        return;
    }

    if (node.tag == QLatin1String("TextBlock")) {
        drawTextNode(painter, node, context);
    } else if (node.tag == QLatin1String("Ticker")) {
        drawTickerNode(painter, node, context);
    } else if (node.tag == QLatin1String("QRCode")) {
        drawQrNode(painter, node, context);
    } else if (node.tag == QLatin1String("Image")) {
        QImage image = context.imageOverrides.value(node.name);
        if (image.isNull()) {
            image = context.model.images.value(node.imageSource);
        }
        paintImage(painter, rect, image, node.imageStretch);
        if (node.strokeThickness > 0) {
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(effectiveColour(node, node.stroke, context.colourOverrides,
                                   Qt::black),
                node.strokeThickness));
            painter.drawRect(rect);
        }
    } else if (node.tag == QLatin1String("Rectangle") || node.tag == QLatin1String("Ellipse")
        || node.tag == QLatin1String("Triangle")
        || node.tag == QLatin1String("RightTriangle")) {
        QPainterPath path;
        if (node.tag == QLatin1String("Rectangle")) {
            if (node.radius > 0) {
                path.addRoundedRect(rect, node.radius, node.radius);
            } else {
                path.addRect(rect);
            }
        } else if (node.tag == QLatin1String("Ellipse")) {
            path.addEllipse(rect);
        } else if (node.tag == QLatin1String("Triangle")) {
            path.moveTo(rect.center().x(), rect.top());
            path.lineTo(rect.right(), rect.bottom());
            path.lineTo(rect.left(), rect.bottom());
            path.closeSubpath();
        } else {
            path.moveTo(rect.left(), rect.top());
            path.lineTo(rect.left(), rect.bottom());
            path.lineTo(rect.right(), rect.bottom());
            path.closeSubpath();
        }
        const QBrush fill = makeBrush(node, node.fill, rect, context.colourOverrides,
            context.model);
        painter.setBrush(fill);
        if (node.strokeThickness > 0 && node.stroke.kind != BrushData::Transparent) {
            painter.setPen(QPen(effectiveColour(node, node.stroke, context.colourOverrides,
                                   Qt::black),
                node.strokeThickness));
        } else {
            painter.setPen(Qt::NoPen);
        }
        painter.drawPath(path);
    }
    painter.restore();
}

void collectElements(const NodeData& node, QVector<TitleElementInfo>& elements,
    QSet<QString>& seen)
{
    if (!node.name.isEmpty() && !seen.contains(node.name)) {
        QString type;
        QString value;
        if (node.tag == QLatin1String("TextBlock")) {
            type = QStringLiteral("text");
            value = node.text;
        } else if (node.tag == QLatin1String("Ticker")) {
            type = QStringLiteral("ticker");
            value = node.tickerText.isEmpty() ? node.text : node.tickerText;
        } else if (node.tag == QLatin1String("Image")) {
            type = QStringLiteral("image");
            value = node.imageSource;
        } else if (node.tag == QLatin1String("QRCode")) {
            type = QStringLiteral("qr");
            value = node.qrText;
        } else if (node.tag == QLatin1String("Rectangle")
            || node.tag == QLatin1String("Ellipse")
            || node.tag == QLatin1String("Triangle")
            || node.tag == QLatin1String("RightTriangle")) {
            if (node.fill.kind != BrushData::Transparent || node.strokeThickness > 0) {
                type = QStringLiteral("colour");
                value = solidColor(node.fill).name(QColor::HexRgb).toUpper();
            }
        }
        if (!type.isEmpty()) {
            elements.append({ node.name, type, value });
            seen.insert(node.name);
        }
    }
    for (const NodeData& child : node.children) {
        collectElements(child, elements, seen);
    }
}

} // namespace

struct TitlePackage::Data {
    bool valid = false;
    QString name;
    QString sourcePath;
    PackageModel model;
    mutable QHash<QString, QImage> overrideCache;

    QImage imageForPath(const QString& path) const
    {
        const auto it = overrideCache.constFind(path);
        if (it != overrideCache.constEnd()) {
            return it.value();
        }
        const QImage image(path);
        overrideCache.insert(path, image);
        return image;
    }
};

TitlePackage::TitlePackage() = default;
TitlePackage::~TitlePackage() = default;

TitlePackage TitlePackage::load(const QString& path, QString* error)
{
    TitlePackage package;
    const QFileInfo info(path);
    auto data = std::make_shared<Data>();
    QSet<QString> sources;

    ZipArchive archive;
    if (info.isFile() && archive.open(path, nullptr)) {
        const QByteArray document = archive.read(QStringLiteral("document.xml"));
        if (document.isEmpty()) {
            if (error) {
                *error = QStringLiteral("Title package has no document.xml");
            }
            return package;
        }
        if (!parseDocument(document, data->model)) {
            if (error) {
                *error = QStringLiteral("Title package XML is not a GT composition");
            }
            return package;
        }
        const QByteArray resources = archive.read(QStringLiteral("resources.xml"));
        if (!resources.isEmpty()) {
            parseResources(resources, data->model.resources);
        }
        collectImageSources(data->model.root, sources);
        for (const QString& source : sources) {
            const QString entry = resolveImageEntry(source, data->model, archive);
            if (entry.isEmpty()) {
                continue;
            }
            const QImage image = QImage::fromData(archive.read(entry));
            if (!image.isNull()) {
                data->model.images.insert(source, image);
            }
        }
    } else {
        // SinSam also imports loose GT composition XML (.xaml / .sinsamtitle)
        // and designers often keep an unzipped .gtzip folder. Both use the same
        // document.xml grammar and resolve images beside the document.
        const QString documentPath = info.isDir()
            ? QDir(path).filePath(QStringLiteral("document.xml"))
            : path;
        const QString baseDirectory = info.isDir() ? path : info.absolutePath();
        QFile file(documentPath);
        if (!file.open(QIODevice::ReadOnly)) {
            if (error) {
                *error = QStringLiteral("Cannot open %1").arg(documentPath);
            }
            return package;
        }
        if (file.size() <= 0 || file.size() > kMaxPackageBytes) {
            if (error) {
                *error = QStringLiteral("Title package is empty or too large");
            }
            return package;
        }
        const QByteArray document = file.readAll();
        if (!parseDocument(document, data->model)) {
            if (error) {
                *error = QStringLiteral("Title package XML is not a GT composition");
            }
            return package;
        }
        QFile resourcesFile(QDir(baseDirectory).filePath(QStringLiteral("resources.xml")));
        if (resourcesFile.open(QIODevice::ReadOnly)) {
            parseResources(resourcesFile.readAll(), data->model.resources);
        }
        collectImageSources(data->model.root, sources);
        for (const QString& source : sources) {
            QString candidate = QDir(baseDirectory).filePath(normalizedArchiveName(source));
            if (!QFileInfo::exists(candidate)) {
                const QString mapped = data->model.resources.value(
                    normalizedArchiveName(source));
                if (!mapped.isEmpty()) {
                    candidate = QDir(baseDirectory).filePath(normalizedArchiveName(mapped));
                }
            }
            const QImage image(candidate);
            if (!image.isNull()) {
                data->model.images.insert(source, image);
            }
        }
    }

    data->valid = true;
    data->sourcePath = path;
    data->name = info.completeBaseName();
    if (data->name.isEmpty()) {
        data->name = QFileInfo(path).fileName();
    }
    package.data_ = data;
    return package;
}

bool TitlePackage::isValid() const { return data_ && data_->valid; }

QString TitlePackage::name() const { return data_ ? data_->name : QString(); }

QString TitlePackage::sourcePath() const { return data_ ? data_->sourcePath : QString(); }

QSize TitlePackage::designSize() const { return data_ ? data_->model.designSize : QSize(); }

QVector<TitleElementInfo> TitlePackage::elements() const
{
    QVector<TitleElementInfo> elements;
    if (!data_ || !data_->valid) {
        return elements;
    }
    QSet<QString> seen;
    collectElements(data_->model.root, elements, seen);
    return elements;
}

QImage TitlePackage::render(QSize canvas, const QHash<QString, QString>& textOverrides,
    const QHash<QString, QColor>& colourOverrides,
    const QHash<QString, QString>& imageOverrides, double animationSeconds,
    double tickerSeconds, const QHash<QString, qint64>& countdownMilliseconds,
    double dataChangeSeconds) const
{
    QImage image(canvas, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    if (!data_ || !data_->valid || canvas.isEmpty() || data_->model.designSize.isEmpty()) {
        return image;
    }
    QHash<QString, QImage> resolvedImages;
    for (auto it = imageOverrides.constBegin(); it != imageOverrides.constEnd(); ++it) {
        if (it.value().isEmpty()) {
            continue;
        }
        const QImage override = data_->imageForPath(it.value());
        if (!override.isNull()) {
            resolvedImages.insert(it.key(), override);
        }
    }
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    // GT Vertical packages are authored on a 3412x1920 artboard whose central
    // 1080x1920 frame is the portrait output. Scale by height and center-crop
    // horizontally; other mismatched aspects are letterboxed without stretching.
    const QSizeF design(data_->model.designSize);
    const bool wideVertical = design.width() >= 3000.0 && design.height() >= 1800.0
        && canvas.height() > canvas.width();
    const qreal scale = wideVertical
        ? canvas.height() / design.height()
        : std::min(canvas.width() / design.width(), canvas.height() / design.height());
    const QPointF offset((canvas.width() - design.width() * scale) / 2.0,
        (canvas.height() - design.height() * scale) / 2.0);
    painter.setClipRect(QRectF(QPointF(0, 0), QSizeF(canvas)));
    painter.translate(offset);
    painter.scale(scale, scale);
    const bool dataChange = dataChangeSeconds >= 0.0;
    RenderContext context{ data_->model, textOverrides, colourOverrides, resolvedImages,
        countdownMilliseconds,
        dataChange ? data_->model.storyboard.dataChangeIn : data_->model.storyboard.enter,
        dataChange ? dataChangeSeconds : animationSeconds, tickerSeconds };
    drawNode(painter, data_->model.root, context);
    painter.end();
    return image;
}

QVector<TitleElementInfo> titlePackageElements(const QString& path)
{
    return TitlePackage::load(path).elements();
}

QString titlePackageClockText(const QString& text, qint64 countdownMilliseconds,
    const QDateTime& now)
{
    if (countdownMilliseconds < 0) {
        return timerText(text, nullptr, now);
    }
    const qint64 remaining = std::max<qint64>(countdownMilliseconds, 0);
    return timerText(text, &remaining, now);
}

} // namespace mixvera::ui