#include "mixvera/services/switcher_service.hpp"
#include "mixvera/services/input_service.hpp"

namespace mixvera::services {

SwitcherService::SwitcherService(core::EventBus& bus, InputService* inputs)
    : bus_(bus)
    , inputs_(inputs)
{
    if (inputs_) {
        state_.programInputId = inputs_->inputs().front().id;
        if (inputs_->inputs().size() > 1) {
            state_.previewInputId = inputs_->inputs()[1].id;
        }
    }
}

bool SwitcherService::setPreview(const std::string& inputId)
{
    if (inputs_ && inputs_->find(inputId) == nullptr) {
        return false;
    }
    state_.previewInputId = inputId;
    bus_.publish({"switcher/preview_changed", inputId});
    return true;
}

bool SwitcherService::take()
{
    if (state_.previewInputId.empty()) {
        return false;
    }
    std::swap(state_.programInputId, state_.previewInputId);
    bus_.publish({"switcher/program_changed", state_.programInputId});
    return true;
}

bool SwitcherService::cut()
{
    state_.transition.kind = core::TransitionKind::Cut;
    state_.transition.position = 0.0F;
    state_.transition.active = false;
    return take();
}

void SwitcherService::validateState() const
{
    // Reserved for invariant checking when the real engine is connected.
}

} // namespace mixvera::services
