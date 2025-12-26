#include "stairs_light.h"
#include "animations.h"
#include "esphome/core/log.h"

namespace esphome
{
  namespace stairs_light
  {
    static const char *const TAG = "StairsLight";

    void StairsLight::setup()
    {
      ESP_LOGD(TAG, "StairsLight component setup complete with %d steps", steps_.size());
      target_settings_light = new light::LightState(this);
      target_settings_light_name_ = this->name_ + " Settings";
      target_settings_light->set_name(target_settings_light_name_.c_str());

      for (auto *animation : this->animations_)
      {
        target_settings_light->add_effects({animation});
        animation->init_internal(target_settings_light, this);
      }

      App.register_light(target_settings_light);
    }

    void StairsLight::loop()
    {
      if (current_animation_ != nullptr)
      {
        current_animation_->apply();
        schedule_show();
      }
    }

    // support settings from HA
    light::LightTraits StairsLight::get_traits()
    {
      auto traits = light::LightTraits();
      traits.set_supported_color_modes({light::ColorMode::RGB});
      return traits;
    }

    // support settings from HA
    void StairsLight::write_state(light::LightState *state)
    {
      auto red = (uint8_t)(state->current_values.get_red() * 255.0f);
      auto green = (uint8_t)(state->current_values.get_green() * 255.0f);
      auto blue = (uint8_t)(state->current_values.get_blue() * 255.0f);
      auto brightness = (uint8_t)(state->current_values.get_brightness() * 100.0f);

      ESP_LOGD(TAG, "Write State: Red: %u, Green: %u, Blue: %u, Brightness: %u%",
               red, green, blue, brightness);
    }

    void StairsLight::add_animations(std::vector<StairsLightAnimation *> animations)
    {
      animations_.reserve(animations.size());
      for (auto *animation : animations)
      {
        this->animations_.push_back(animation);
      }
    }

    void StairsLight::start_run(bool reversed, uint32_t animation_length, const std::string &animation_name)
    {
      auto target_animation_name = !animation_name.empty() ? animation_name : target_settings_light->get_effect_name();
      current_animation_length_ = animation_length == 0 ? get_animation_length() : animation_length;
      ESP_LOGD(TAG, "Starting stairs animation (reversed: %s, transition: %ums, animation: '%s')",
               reversed ? "true" : "false",
               current_animation_length_,
               animation_name.c_str());

      if (strcasecmp(target_animation_name.c_str(), "None") == 0)
      {
        ESP_LOGD(TAG, "Animation is 'None'. Turn on without animation.");
        source_light_->turn_on()
            .set_brightness(target_settings_light->current_values.get_brightness())
            .set_rgb(
                target_settings_light->current_values.get_red(),
                target_settings_light->current_values.get_green(),
                target_settings_light->current_values.get_blue())
            .perform();
      }

      StairsLightAnimation *animation = nullptr;

      for (uint32_t i = 0; i < this->animations_.size(); i++)
      {
        auto a = this->animations_[i];

        if (strcasecmp(target_animation_name.c_str(), a->get_name().c_str()) == 0)
        {
          animation = a;
          break;
        }
      }

      // if stste OFF the turn on
      if (source_light_->remote_values.get_state() == 0.0f)
      {
        source_light_->turn_on()
            .set_brightness(1.f)
            .set_transition_length(0)
            .perform();
        auto output = (light::AddressableLight *)source_light_->get_output();
        output->all().set_rgb(0, 0, 0);
      }

      if (animation == nullptr)
      {
        ESP_LOGW(TAG, "'%s' - No such animation '%s'", this->name_.c_str(), target_animation_name.c_str());
      }
      else
      {
        ESP_LOGD(TAG, "'%s' selected animation: %s", this->name_.c_str(), animation->get_name().c_str());
        current_animation_ = animation;
        running_ = true;
        animation->start(reversed);
        set_timeout("animation_end", current_animation_length_, [this]()
                    {
          this->current_animation_ = nullptr;
          this->running_ = false; });
      }
    }

    void StairsLight::stop_run()
    {
      running_ = false;
      if (current_animation_ != nullptr)
      {
        current_animation_->stop();
        current_animation_ = nullptr;
        cancel_timeout("animation_end");
      }
      ESP_LOGD(TAG, "Stairs animation stopped");
    }

    void StairsLight::set_steps(const std::vector<int> &steps)
    {
      auto output = (light::AddressableLight *)source_light_->get_output();
      steps_.clear();
      steps_.reserve(steps.size());
      for (int32_t i = 0, begin = 0; i < steps.size(); i++)
      {
        int32_t end = begin + abs(steps[i]);
        steps_.push_back(output->range(begin, end));
        begin = end;
      }
    }

  } // namespace stairs_light
} // namespace esphome
