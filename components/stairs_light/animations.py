import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import CONF_NAME
from .types import StairsLightFadeAnimation, StairsLightArrowAnimation

STAIRS_LIGHT_ANIMATIONS=[]

def register_stairs_light_animation(name, animation_type, default_name, schema, *extra_validators):
    STAIRS_LIGHT_ANIMATIONS.append(name)
    return light.effects.register_effect(name, animation_type, default_name, schema, *extra_validators)

@register_stairs_light_animation(
    "fade",
    StairsLightFadeAnimation,
    "Fade",
    {        
    })
def fade_effect_to_code(config, animation_id):
    animation = cg.new_Pvariable(animation_id, config[CONF_NAME])
    return animation

@register_stairs_light_animation(
    "arrow",
    StairsLightArrowAnimation,
    "Arrow",
    {  
        cv.Optional("increment"): cv.positive_int
    })
def fade_effect_to_code(config, animation_id):
    animation = cg.new_Pvariable(animation_id, config[CONF_NAME])
    if 'increment' in config:
        cg.add( animation.set_increment(config['increment']))
    return animation

