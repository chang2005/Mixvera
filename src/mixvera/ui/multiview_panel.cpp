#include "mixvera/ui/multiview_panel.hpp"

#include "mixvera/core/application.hpp"
#include "mixvera/services/input_service.hpp"
#include "mixvera/services/switcher_service.hpp"
#include "mixvera/ui/video_surface.hpp"

#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QStyle>
#include <QVBoxLayout>

namespace mixvera::ui {

class MultiviewTile final : public QFrame {
public:
    MultiviewTile(const QString& inputId, const QString& name, QWidget* parent)
        : QFrame(parent)
        , inputId_(inputId)
        , name_(name)
    {
        setObjectName("multiviewTile");
        setCursor(Qt::PointingHandCursor);
        setMinimumSize(160, 112);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(4, 4, 4, 6);
        layout->setSpacing(5);

        surface_ = new VideoSurface(this);
        surface_->setRole(VideoSurface::Role::Multiview);
        surface_->setCompact(true);
        surface_->setInputId(inputId);
        surface_->setAttribute(Qt::WA_TransparentForMouseEvents);
        layout->addWidget(surface_, 1);

        caption_ = new QLabel(QString("%1  ·  %2").arg(name.toUpper(), inputId.toUpper()), this);
        caption_->setObjectName("tileCaption");
        caption_->setAlignment(Qt::AlignCenter);
        layout->addWidget(caption_);

        setState(false, false);
    }

    void setSelectCallback(std::function<void(const QString&)> callback)
    {
        callback_ = std::move(callback);
    }

    void setState(bool program, bool preview)
    {
        QString state = "idle";
        if (program) {
            state = "program";
            caption_->setText(name_.toUpper() + "  ·  PGM");
        } else if (preview) {
            state = "preview";
            caption_->setText(name_.toUpper() + "  ·  PVW");
        } else {
            caption_->setText(name_.toUpper());
        }
        setProperty("state", state);
        style()->unpolish(this);
        style()->polish(this);
        surface_->setSelected(program || preview);
        update();
    }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && callback_) {
            callback_(inputId_);
        }
        QFrame::mousePressEvent(event);
    }

private:
    QString inputId_;
    QString name_;
    VideoSurface* surface_ = nullptr;
    QLabel* caption_ = nullptr;
    std::function<void(const QString&)> callback_;
};

MultiviewPanel::MultiviewPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    const auto& inputs = core::Application::instance().services().get<services::InputService>().inputs();
    int index = 0;
    for (const auto& input : inputs) {
        auto* tile = new MultiviewTile(QString::fromStdString(input.id),
            QString::fromStdString(input.name), this);
        tile->setSelectCallback([this](const QString& inputId) {
            if (callback_) {
                callback_(inputId);
            }
        });
        layout->addWidget(tile, index / 2, index % 2);
        tiles_.push_back({QString::fromStdString(input.id), tile});
        ++index;
    }
    refresh();
}

void MultiviewPanel::setSelectCallback(std::function<void(const QString&)> callback)
{
    callback_ = std::move(callback);
}

void MultiviewPanel::refresh()
{
    const auto& state = core::Application::instance().services().get<services::SwitcherService>().state();
    for (auto& tile : tiles_) {
        const bool isProgram = tile.inputId.toStdString() == state.programInputId;
        const bool isPreview = tile.inputId.toStdString() == state.previewInputId;
        tile.widget->setState(isProgram, isPreview);
    }
}

} // namespace mixvera::ui
