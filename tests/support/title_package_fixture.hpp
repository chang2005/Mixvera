#pragma once

// Stored-ZIP GT title package fixtures shared by the media and UI tests.

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QPair>
#include <QString>
#include <QTemporaryDir>
#include <QVector>

inline void append16(QByteArray& data, quint16 value)
{
    data.append(static_cast<char>(value & 0xFF));
    data.append(static_cast<char>((value >> 8) & 0xFF));
}

inline void append32(QByteArray& data, quint32 value)
{
    data.append(static_cast<char>(value & 0xFF));
    data.append(static_cast<char>((value >> 8) & 0xFF));
    data.append(static_cast<char>((value >> 16) & 0xFF));
    data.append(static_cast<char>((value >> 24) & 0xFF));
}

inline quint32 crc32Of(const QByteArray& data)
{
    quint32 crc = 0xFFFFFFFFU;
    for (const char character : data) {
        crc ^= static_cast<unsigned char>(character);
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320U & (0U - (crc & 1U)));
        }
    }
    return crc ^ 0xFFFFFFFFU;
}

inline QByteArray makeStoredZip(const QVector<QPair<QString, QByteArray>>& entries)
{
    QByteArray archive;
    QVector<quint32> offsets;
    for (const auto& entry : entries) {
        offsets.append(static_cast<quint32>(archive.size()));
        const QByteArray name = entry.first.toUtf8();
        append32(archive, 0x04034B50U);
        append16(archive, 20);
        append16(archive, 0);
        append16(archive, 0);
        append16(archive, 0);
        append16(archive, 0);
        append32(archive, crc32Of(entry.second));
        append32(archive, static_cast<quint32>(entry.second.size()));
        append32(archive, static_cast<quint32>(entry.second.size()));
        append16(archive, static_cast<quint16>(name.size()));
        append16(archive, 0);
        archive.append(name);
        archive.append(entry.second);
    }
    const int directoryOffset = archive.size();
    for (int index = 0; index < entries.size(); ++index) {
        const QByteArray name = entries[index].first.toUtf8();
        append32(archive, 0x02014B50U);
        append16(archive, 20);
        append16(archive, 20);
        append16(archive, 0);
        append16(archive, 0);
        append16(archive, 0);
        append16(archive, 0);
        append32(archive, crc32Of(entries[index].second));
        append32(archive, static_cast<quint32>(entries[index].second.size()));
        append32(archive, static_cast<quint32>(entries[index].second.size()));
        append16(archive, static_cast<quint16>(name.size()));
        append16(archive, 0);
        append16(archive, 0);
        append16(archive, 0);
        append16(archive, 0);
        append32(archive, 0);
        append32(archive, offsets[index]);
        archive.append(name);
    }
    const int directorySize = archive.size() - directoryOffset;
    append32(archive, 0x06054B50U);
    append16(archive, 0);
    append16(archive, 0);
    append16(archive, static_cast<quint16>(entries.size()));
    append16(archive, static_cast<quint16>(entries.size()));
    append32(archive, static_cast<quint32>(directorySize));
    append32(archive, static_cast<quint32>(directoryOffset));
    append16(archive, 0);
    return archive;
}

inline QString writeScoreboardPackage(const QTemporaryDir& directory)
{
    const QByteArray document = R"XML(<Composition Width="1280" Height="720">
  <Layer Name="Layer1" Location="0,0">
    <Layer.Composition>
      <Composition Width="1280" Height="720">
        <Rectangle Name="Backdrop" Dimensions="1280,720">
          <Rectangle.Fill><Brush Color="#FF103050" /></Rectangle.Fill>
        </Rectangle>
        <TextBlock Name="Home Team" Dimensions="300,80" Location="60,100" Text="HOME" FontFamily="Arial" FontSize="40">
          <TextBlock.Fill><Brush Color="#FFFFFFFF" /></TextBlock.Fill>
        </TextBlock>
        <TextBlock Name="Home Score" Dimensions="120,80" Location="380,100" Text="0" FontFamily="Arial" FontSize="56">
          <TextBlock.Fill><Brush Color="#FFFFAA00" /></TextBlock.Fill>
        </TextBlock>
        <TextBlock Name="Away Team" Dimensions="300,80" Location="60,220" Text="AWAY" FontFamily="Arial" FontSize="40">
          <TextBlock.Fill><Brush Color="#FFFFFFFF" /></TextBlock.Fill>
        </TextBlock>
        <TextBlock Name="Away Score" Dimensions="120,80" Location="380,220" Text="0" FontFamily="Arial" FontSize="56">
          <TextBlock.Fill><Brush Color="#FFFFAA00" /></TextBlock.Fill>
        </TextBlock>
        <TextBlock Name="Game Clock" Dimensions="500,90" Location="700,100" Text="{0:HH:mm:ss}" FontFamily="Arial" FontSize="48">
          <TextBlock.Fill><Brush Color="#FFFFFFFF" /></TextBlock.Fill>
        </TextBlock>
        <TextBlock Name="Short Clock" Dimensions="500,90" Location="700,220" Text="{0:mm:ss}" FontFamily="Arial" FontSize="48">
          <TextBlock.Fill><Brush Color="#FFFFFFFF" /></TextBlock.Fill>
        </TextBlock>
      </Composition>
    </Layer.Composition>
  </Layer>
</Composition>)XML";
    const QString path = QDir(directory.path()).filePath(QStringLiteral("scoreboard.gtzip"));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return {};
    }
    const QByteArray archive = makeStoredZip({
        { QStringLiteral("document.xml"), document },
        { QStringLiteral("[Content_Types].xml"), QByteArray("<Types/>") },
    });
    if (file.write(archive) != archive.size()) {
        return {};
    }
    return path;
}

inline QString writeVerticalPackage(const QTemporaryDir& directory)
{
    const QByteArray document = R"XML(<Composition Width="3412" Height="1920">
  <Layer Name="Layer1" Location="0,0">
    <Layer.Composition>
      <Composition Width="3412" Height="1920">
        <Rectangle Name="Center" Dimensions="1080,1920,0" Location="1166,0">
          <Rectangle.Fill><Brush Color="#FFFF0000" /></Rectangle.Fill>
        </Rectangle>
        <Rectangle Name="Outside" Dimensions="100,100,0" Location="0,0">
          <Rectangle.Fill><Brush Color="#FF00FF00" /></Rectangle.Fill>
        </Rectangle>
        <Rectangle Name="Marker" Dimensions="100,100,0" Location="1706,910">
          <Rectangle.Fill><Brush Color="#FFFFFF00" /></Rectangle.Fill>
        </Rectangle>
      </Composition>
    </Layer.Composition>
  </Layer>
</Composition>)XML";
    const QString path = QDir(directory.path()).filePath(QStringLiteral("vertical.gtzip"));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return {};
    }
    const QByteArray archive = makeStoredZip({
        { QStringLiteral("document.xml"), document },
        { QStringLiteral("[Content_Types].xml"), QByteArray("<Types/>") },
    });
    if (file.write(archive) != archive.size()) {
        return {};
    }
    return path;
}

inline QByteArray syntheticDocument()
{
    return R"XML(<Composition Width="1280" Height="720">
  <Layer Name="Layer1" Location="0,0">
    <Layer.Composition>
      <Composition Width="1280" Height="720">
        <Rectangle Name="Backdrop" Dimensions="1280,720">
          <Rectangle.Fill><Brush Color="#FF202020" /></Rectangle.Fill>
        </Rectangle>
        <TextBlock Name="Headline" Dimensions="1000,160" Location="140,120" Text="DEFAULT HEADLINE" FontFamily="Arial" FontSize="64" FontWeight="Bold">
          <TextBlock.Fill><Brush Color="#FFFFFFFF" /></TextBlock.Fill>
        </TextBlock>
        <TextBlock Name="Description" Dimensions="1000,60" Location="140,300" Text="Default description" FontFamily="Arial" FontSize="32">
          <TextBlock.Fill><Brush Color="#FFFFAA00" /></TextBlock.Fill>
        </TextBlock>
        <Ticker Name="Ticker" Dimensions="1280,80" Location="0,640" FontSize="40" Speed="4">
          <Ticker.Fill><Brush Color="#FFFFFFFF" /></Ticker.Fill>
          <Ticker.Template><TextBlock Text="DEFAULT TICKER" /></Ticker.Template>
        </Ticker>
      </Composition>
    </Layer.Composition>
  </Layer>
  <Storyboard>
    <Storyboard.Animations>
      <Fade Object="Backdrop" Duration="0.4" />
      <Fade Object="Headline" Duration="0.4" />
      <Fade Object="Description" Duration="0.4" Delay="0.2" />
      <Fade Object="Ticker" Duration="0.4" />
    </Storyboard.Animations>
  </Storyboard>
  <Storyboard Type="DataChangeIn">
    <Storyboard.Animations>
      <Fade Object="Headline" Duration="0.4" />
    </Storyboard.Animations>
  </Storyboard>
</Composition>)XML";
}

inline QString writeLoosePackage(const QTemporaryDir& directory)
{
    const QString path = QDir(directory.path()).filePath(QStringLiteral("loose.xaml"));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return {};
    }
    const QByteArray document = syntheticDocument();
    if (file.write(document) != document.size()) {
        return {};
    }
    return path;
}

inline QString writeSyntheticPackage(const QTemporaryDir& directory)
{
    const QByteArray document = R"XML(<Composition Width="1280" Height="720">
  <Layer Name="Layer1" Location="0,0">
    <Layer.Composition>
      <Composition Width="1280" Height="720">
        <Rectangle Name="Backdrop" Dimensions="1280,720">
          <Rectangle.Fill><Brush Color="#FF202020" /></Rectangle.Fill>
        </Rectangle>
        <TextBlock Name="Headline" Dimensions="1000,160" Location="140,120" Text="DEFAULT HEADLINE" FontFamily="Arial" FontSize="64" FontWeight="Bold">
          <TextBlock.Fill><Brush Color="#FFFFFFFF" /></TextBlock.Fill>
        </TextBlock>
        <TextBlock Name="Description" Dimensions="1000,60" Location="140,300" Text="Default description" FontFamily="Arial" FontSize="32">
          <TextBlock.Fill><Brush Color="#FFFFAA00" /></TextBlock.Fill>
        </TextBlock>
        <Ticker Name="Ticker" Dimensions="1280,80" Location="0,640" FontSize="40" Speed="4">
          <Ticker.Fill><Brush Color="#FFFFFFFF" /></Ticker.Fill>
          <Ticker.Template><TextBlock Text="DEFAULT TICKER" /></Ticker.Template>
        </Ticker>
      </Composition>
    </Layer.Composition>
  </Layer>
  <Storyboard>
    <Storyboard.Animations>
      <Fade Object="Backdrop" Duration="0.4" />
      <Fade Object="Headline" Duration="0.4" />
      <Fade Object="Description" Duration="0.4" Delay="0.2" />
      <Fade Object="Ticker" Duration="0.4" />
    </Storyboard.Animations>
  </Storyboard>
  <Storyboard Type="DataChangeIn">
    <Storyboard.Animations>
      <Fade Object="Headline" Duration="0.4" />
    </Storyboard.Animations>
  </Storyboard>
</Composition>)XML";
    const QString path = QDir(directory.path()).filePath(QStringLiteral("synthetic.gtzip"));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return {};
    }
    const QByteArray archive = makeStoredZip({
        { QStringLiteral("document.xml"), document },
        { QStringLiteral("[Content_Types].xml"), QByteArray("<Types/>") },
    });
    if (file.write(archive) != archive.size()) {
        return {};
    }
    return path;
}