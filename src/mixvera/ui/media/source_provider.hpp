#pragma once

#include "mixvera/core/domain_types.hpp"

#include <QImage>
#include <QObject>
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

    virtual bool ready() const { return true; }
};

std::unique_ptr<SourceProvider> createSourceProvider(const core::InputDescriptor& input);

} // namespace mixvera::ui