#include "mixvera/core/event_bus.hpp"
#include "mixvera/services/input_service.hpp"
#include "mixvera/services/switcher_service.hpp"
#include "mixvera/services/transition_service.hpp"

#include <cassert>
#include <string>

using namespace mixvera;

static void takeSwapsProgramAndPreview()
{
    core::EventBus bus;
    services::InputService inputs(bus);
    services::SwitcherService switcher(bus, &inputs);

    const auto initial = switcher.state();
    assert(switcher.take());
    assert(switcher.state().programInputId == initial.previewInputId);
    assert(switcher.state().previewInputId == initial.programInputId);
}

static void tbarReleaseCompletesTransition()
{
    core::EventBus bus;
    services::InputService inputs(bus);
    services::SwitcherService switcher(bus, &inputs);
    services::TransitionService transition(bus, &switcher);

    const auto initialProgram = switcher.state().programInputId;
    transition.setPosition(0.8F);
    transition.release();
    assert(switcher.state().programInputId != initialProgram);
}

int main()
{
    takeSwapsProgramAndPreview();
    tbarReleaseCompletesTransition();
    return 0;
}
