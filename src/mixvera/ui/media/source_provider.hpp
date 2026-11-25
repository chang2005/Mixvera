#pragma once

#include "mixvera/core/domain_types.hpp"

#include <QImage>
#include <QObject>
#include <QSize>
#include <QString>

#include <memory>

namespace mixvera::ui {

/// Produces frames for one input. Providers never touch the UI services; the
/// compositor owns them and asks for the latest frame once per tick.
class SourceProvider {
public:
    virtual ~SourceProvider() = default;

    /// Latest native frame; may be null while a device is still starting.
    virtual QImage frame() = 0;

    /// Short human readable state, shown in the input list tooltip.
    virtual QString detail() const { return QString(); }

    /// Lets canvas-sized generators (titles) follow portrait or square shows.
    /// Capture and file providers keep their native frame size.
    virtual void setCanvasSize(QSize size) { Q_UNUSED(size); }

    /// Applies changed input settings without rebuilding the provider, so
    /// title animations, tickers and cached package assets keep running.
    /// Returns false when the change needs a fresh provider.
    virtual bool update(const core::InputDescriptor& input)
    {
        Q_UNUSED(input);
        return false;
    }

    virtual bool ready() const { return true; }
};

std::unique_ptr<SourceProvider> createSourceProvider(const core::InputDescriptor& input);

} // namespace mixvera::ui