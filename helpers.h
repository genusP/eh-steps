#pragma once
#include "esphome/components/light/addressable_light.h"

using esphome::light::LightCall;
using esphome::light::LightState;

LightCall CopyLightState(LightState *target, LightState *source)
{
    return target->make_call()
        .set_brightness(source->current_values.get_brightness())
        .from_light_color_values(source->current_values);
}
