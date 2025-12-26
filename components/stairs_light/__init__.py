import copy
from esphome import automation
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.core import ID, CORE
from esphome.const import (
    CONF_ID, 
    CONF_SEGMENTS, 
    CONF_FROM, 
    CONF_TO, 
    CONF_REVERSED, 
    CONF_OUTPUT_ID, 
    CONF_NAME, 
    CONF_TYPE_ID,
    CONF_INTERNAL,
    CONF_RED,
    CONF_GREEN,
    CONF_BLUE,
    CONF_TRANSITION_LENGTH,
    CONF_BRIGHTNESS
)
from esphome.components import light
from esphome.components.partition.light import to_code as partition_light_to_code, PartitionLightOutput, CONFIG_SCHEMA as PARTITION_CONFIG_SCHEMA

from .const import CONF_LED_ID, CONF_STEPS, CONF_STEP, CONF_ANIMATION, CONF_ANIMATIONS, CONF_ANIMATION_LENGTH
from .types import stairs_light_ns, StairsLight
from .animations import STAIRS_LIGHT_ANIMATIONS

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(StairsLight),
    cv.Required(CONF_LED_ID): cv.use_id(light.AddressableLightState),
    cv.Required(CONF_STEPS): cv.ensure_list(cv.int_),
    cv.Optional(CONF_ANIMATION_LENGTH, default="500ms"): cv.positive_time_period_milliseconds,
    cv.Optional(CONF_ANIMATIONS): light.validate_effects(STAIRS_LIGHT_ANIMATIONS) 
}).extend(cv.COMPONENT_SCHEMA)

STAIRS_LIGHT_STEP_ON_ACTION_SCHEMA = cv.Schema({
    cv.Required(CONF_ID): cv.use_id(StairsLight),
    cv.Required(CONF_STEP): cv.templatable(cv.positive_int),
    cv.Optional(CONF_RED): cv.templatable(cv.percentage),
    cv.Optional(CONF_GREEN): cv.templatable(cv.percentage),
    cv.Optional(CONF_BLUE): cv.templatable(cv.percentage),
    cv.Optional(CONF_TRANSITION_LENGTH): cv.templatable(cv.positive_time_period_milliseconds),
    cv.Optional(CONF_BRIGHTNESS): cv.templatable(cv.percentage)
})

RUN_ACTION_SCHEMA = cv.Schema({
    cv.Required(CONF_ID): cv.use_id(StairsLight),
    cv.Optional(CONF_REVERSED, default=False): cv.templatable(cv.boolean),
    cv.Optional(CONF_ANIMATION_LENGTH): cv.templatable(cv.positive_time_period_milliseconds),
    cv.Optional(CONF_ANIMATION): cv.templatable(cv.string),
})

@automation.register_action(
    'stairs_light.step_on',
    stairs_light_ns.class_('StepOnAction', automation.Action),
    STAIRS_LIGHT_STEP_ON_ACTION_SCHEMA
)
async def step_on_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    
    step_num = await cg.templatable(config[CONF_STEP], args, cg.uint32)
    cg.add(var.set_step(step_num))
    
    if(CONF_BRIGHTNESS in config):
        brightness = await cg.templatable(config[CONF_BRIGHTNESS], args, cg.float_)
        cg.add(var.set_brightness(brightness))
    
    for color in [CONF_RED, CONF_GREEN, CONF_BLUE]:
        if color in config:
            color_value = await cg.templatable(config[color], args, cg.float_)
            cg.add(var.set_color(cg.std_string(color), color_value))
    
    if CONF_TRANSITION_LENGTH in config:
        transition_lenght = await cg.templatable(config[CONF_TRANSITION_LENGTH], args, cg.uint32)
        cg.add(var.set_transition_length(transition_lenght))
    
    return var

@automation.register_action(
    'stairs_light.run',
    stairs_light_ns.class_('RunAction', automation.Action),
    RUN_ACTION_SCHEMA
)
async def stairs_light_run_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)

    # Add reversed parameter
    if CONF_REVERSED in config:
        reversed_template = await cg.templatable(config[CONF_REVERSED], args, cg.bool_)
        cg.add(var.set_reversed(reversed_template))

    # Add animation_length parameter (optional)
    if CONF_ANIMATION_LENGTH in config:
        animation_template = await cg.templatable(config[CONF_ANIMATION_LENGTH], args, cg.uint32)
        cg.add(var.set_animation_length(animation_template))

    # Add animation parameter (optional)
    if CONF_ANIMATION in config:
        animation_template = await cg.templatable(config[CONF_ANIMATION], args, cg.std_string)
        cg.add(var.set_animation(animation_template))
    return var

async def to_code(config):
    cg.add_library("esp32-led-strip", None)

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    steps = config[CONF_STEPS]
    
    component_id = config[CONF_ID].id
    cg.add(var.set_name(component_id))
    light_parent = await cg.get_variable(config[CONF_LED_ID])
    cg.add(var.set_light(light_parent))
    cg.add(var.set_steps(steps))
    animations = await cg.build_registry_list(
        light.EFFECTS_REGISTRY, config.get(CONF_ANIMATIONS, [])
    )
    
    cg.add(var.add_animations(animations))
    cg.add(var.set_animation_length(int(config[CONF_ANIMATION_LENGTH].total_milliseconds)))
    return var
