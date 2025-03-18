#pragma once
#include "esphome/components/light/addressable_light.h"
#include "esphome/components/light/automation.h"
#include "esphome/components/partition/light_partition.h"
#include "esphome/core/application.h"
#include "esphome/core/automation.h"
#include "esphome/core/base_automation.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

using esphome::App;
using esphome::light::LightCall;
using esphome::light::LightState;
using esphome::partition::PartitionLightOutput;

LightCall CopyLightState(LightState *target, LightState *source)
{
    return target->make_call()
        .set_brightness(source->current_values.get_brightness())
        .set_transition_length(source->get_default_transition_length())
        .from_light_color_values(source->current_values);
}

LightState *GetStep(uint8_t step_num)
{
    auto name = "step_" + std::to_string(step_num);
    return App.get_light_by_key(esphome::fnv1_hash(name), true);
}

void StepOn(uint8_t step_num, LightState *settings)
{
    auto step = GetStep(step_num);
    auto call = CopyLightState(step, settings);
    call.set_state(true);
    call.perform();
}

void GenerateSteps(LightState *target, std::vector<uint8_t> &steps_width)
{
    auto pos = 0;
    auto result = new std::vector<PartitionLightOutput *>();
    for (size_t i = 0; i < steps_width.size(); i++)
    {
        auto width = steps_width[i];
        auto segment = esphome::partition::AddressableSegment(target, pos, width, false);

        auto partiton = new PartitionLightOutput({segment});
        partiton->set_component_source("partial.light");
        App.register_component(partiton);

        auto state = new esphome::light::AddressableLightState(partiton);
        App.register_light(state);
        state->set_component_source("light");
        App.register_component(state);
        auto key = ("step_" + std::to_string(i)).c_str();
        state->set_object_id(key);
        state->set_internal(true);

        pos += width;
    }
}
