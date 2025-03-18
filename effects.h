#include "esphome/components/light/addressable_light.h"

using esphome::light::AddressableLightState;

void ColorSwitch(AddressableLightState *line)
{
    static int state = 0;
    auto call = line->make_call();
    // Transition of 1000ms = 1s
    call.set_transition_length(1000);
    call.set_brightness(75);
    if (state == 0)
    {
        call.set_rgb(1.0, 1.0, 1.0);
    }
    else if (state == 1)
    {
        call.set_rgb(1.0, 0.0, 1.0);
    }
    else if (state == 2)
    {
        call.set_rgb(0.0, 0.0, 1.0);
    }
    else
    {
        call.set_rgb(1.0, 0.0, 0.0);
    }
    call.perform();
    state += 1;
    if (state == 4)
        state = 0;
}
