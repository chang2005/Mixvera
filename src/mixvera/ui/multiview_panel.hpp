#pragma once

#include <QString>
#include <QWidget>
#include <functional>
#include <vector>

namespace mixvera::ui {

class MultiviewTile;

class MultiviewPanel final : public QWidget {
public:
    explicit MultiviewPanel(QWidget* parent = nullptr);

    void setSelectCallback(std::function<void(const QString&)> callback);
    void refresh();

private:
    struct Tile {
        QString inputId;
        MultiviewTile* widget = nullptr;
    };

    std::vector<Tile> tiles_;
    std::function<void(const QString&)> callback_;
};

} // namespace mixvera::ui
