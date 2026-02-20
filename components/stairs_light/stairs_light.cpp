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
      ha_light_ = new light::LightState(this);
      target_settings_light_name_ = this->name_ + " Settings";
      ha_light_->set_name(target_settings_light_name_.c_str());
      ha_light_->set_restore_mode(esphome::light::LightRestoreMode::LIGHT_ALWAYS_OFF);

      for (auto *animation : this->animations_)
      {
        ha_light_->add_effects({animation});
        animation->init_internal(this);
      }

      App.register_component(ha_light_);
      App.register_light(ha_light_);

      restore_settings();
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
      bool is_on = state->remote_values.get_state() > 0.1f;
      ESP_LOGD(TAG, "is_on: %.2f", is_on);

      if (is_on)
      {
        settings_.r = state->current_values.get_red();
        settings_.g = state->current_values.get_green();
        settings_.b = state->current_values.get_blue();
        settings_.brightness = state->current_values.get_brightness();
        settings_.animation_index = 0;

        auto old_animation_index = settings_.animation_index;
        auto current_effect_name = state->get_effect_name();
        if (!current_effect_name.empty())
        {
          auto effects = state->get_effects();
          for (size_t i = 0; i < effects.size(); i++)
          {
            if (effects[i]->get_name() == current_effect_name)
            {
              settings_.animation_index = i + 1; // +1, так как 0 зарезервирован под "None"
              break;
            }
          }
        }

        ESP_LOGD(TAG, "Write State: Red: %.0f, Green: %.0f, Blue: %.0f, Brightness: %.0f%%",
                 settings_.r * 255,
                 settings_.g * 255,
                 settings_.b * 255,
                 settings_.brightness * 100);

        // если сменили анимацию то выключаем и заново включаем
        if (old_animation_index != settings_.animation_index && settings_.animation_index > 0)
        {
          source_light_->turn_off().perform();
        }

        turn_on();
        save_settings();
      }
      else
      {
        stop_animation();
        turn_off();
      }
    }

    void StairsLight::add_animations(std::vector<StairsLightAnimation *> animations)
    {
      animations_.reserve(animations.size());
      for (auto *animation : animations)
      {
        this->animations_.push_back(animation);
      }
    }

    void StairsLight::turn_on(bool reversed, uint32_t animation_length, const std::string &animation_name)
    {
      auto current_animation_length = animation_length == 0 ? get_animation_length() : animation_length;
      ESP_LOGD(TAG, "Starting stairs animation (reversed: %s, transition: %ums, animation: '%s')",
               reversed ? "true" : "false",
               current_animation_length,
               animation_name.c_str());

      if (animation_name.empty() && settings_.animation_index == 0)
      {
        ESP_LOGD(TAG, "Animation is 'None'. Turn on without animation.");
        source_light_->turn_on()
            .set_brightness(ha_light_->current_values.get_brightness())
            .set_rgb(
                settings_.r,
                settings_.g,
                settings_.b)
            .perform();
      }
      else
      {
        StairsLightAnimation *animation = nullptr;

        if (!animation_name.empty())
        {
          for (uint32_t i = 0; i < this->animations_.size(); i++)
          {
            auto a = this->animations_[i];

            if (strcasecmp(animation_name.c_str(), a->get_name().c_str()) == 0)
            {
              animation = a;
              break;
            }
          }
        }
        else if (settings_.animation_index <= animations_.size())
        {
          animation = animations_[settings_.animation_index - 1];
        }

        if (animation == nullptr)
        {
          ESP_LOGW(TAG, "'%s' - No such animation '%s'", this->name_.c_str(), animation_name.c_str());
        }
        else
        {
          // if state OFF the turn on
          if (source_light_->remote_values.get_state() == 0.0f)
          {
            source_light_->turn_on()
                .set_brightness(1.f)
                .set_transition_length(0)
                .perform();
            auto output = (light::AddressableLight *)source_light_->get_output();
            output->all().set_rgb(0, 0, 0);
          }
          ESP_LOGD(TAG, "'%s' selected animation: %s", this->name_.c_str(), animation->get_name().c_str());
          current_animation_ = animation;
          animation->start(reversed);
          set_timeout("animation_end", current_animation_length, [this]()
                      { this->current_animation_ = nullptr; });
        }
      }
    }

    void StairsLight::turn_off(uint32_t transition_length)
    {
      source_light_->turn_off()
          .set_transition_length(transition_length)
          .perform();
    }

    void StairsLight::stop_animation()
    {
      if (current_animation_ != nullptr)
      {
        ESP_LOGD(TAG, "Stairs animation stop");
        current_animation_->stop();
        current_animation_ = nullptr;
        cancel_timeout("animation_end");
      }
      ESP_LOGD(TAG, "Stairs animation stopped");
    }

    void StairsLight::set_entry_light(float brightness)
    {
      stop_animation();
      auto name = new std::string("entry");
      auto animation = new EntryStepsAnimation(*name);
      animation->set_brightness(brightness);
      animation->start();
      current_animation_ = animation;
      set_timeout("animation_end", animation->transition_length, [this, animation, name]()
                  { 
                        this->current_animation_ = nullptr; 
                        delete animation, name; });
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

    void StairsLight::restore_settings()
    {
      pref_ = global_preferences->make_preference<StairsSettings>(812712);

      if (pref_.load(&settings_))
      {
        ESP_LOGD(TAG, "Restore settings from Flash... (R: %.2f; G: %.2f; B: %.2f)", settings_.r, settings_.g, settings_.b);
        esphome::light::LightStateRTCState recovered{};
        recovered.red = settings_.r;
        recovered.green = settings_.g;
        recovered.blue = settings_.b;
        recovered.brightness = settings_.brightness;
        ha_light_->set_initial_state(recovered);
        // ha_light_->make_call()
        //     .set_rgb(settings_.r, settings_.g, settings_.b)
        //     .set_brightness(settings_.brightness)
        //     .perform();
      }
    }

    void StairsLight::save_settings()
    {
      this->set_timeout("save_stairs_pref", 5000, [this]()
                        {
        ESP_LOGD(TAG, "Saving settings to Flash...");
        this->pref_.save(&this->settings_); });
    }
  } // namespace stairs_light
} // namespace esphome
