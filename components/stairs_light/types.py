import esphome.codegen as cg
from esphome.components import light


stairs_light_ns = cg.esphome_ns.namespace('stairs_light')
StairsLight = stairs_light_ns.class_('StairsLight', cg.Component)

StairsLightAnimation = stairs_light_ns.class_('StairsLightAnimation', light.types.LightEffect)
StairsLightFadeAnimation = stairs_light_ns.class_('StairsLightFadeAnimation', StairsLightAnimation)
StairsLightArrowAnimation = stairs_light_ns.class_('StairsLightArrowAnimation', StairsLightAnimation)

