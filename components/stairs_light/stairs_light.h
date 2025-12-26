#pragma once
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/components/light/light_state.h"
#include "esphome/components/light/addressable_light.h"
#include <vector>
#include <memory>
#include <map>
#include <string>

namespace esphome
{
  namespace stairs_light
  {
    class StairsLightAnimation;

    class StairsLight : public Component, public light::LightOutput
    {
    public:
      StairsLight() = default;

      // Entities
      light::LightState *target_settings_light;

      void setup() override;
      void loop() override;

      void set_light(light::AddressableLightState *light) { this->source_light_ = light; }
      void set_name(const std::string &name) { this->name_ = name; }
      void set_steps(const std::vector<int> &steps);
      void set_animation_length(uint32_t length) { this->animation_length_ = length; }
      void add_animations(std::vector<StairsLightAnimation *> animations);

      uint32_t get_animation_length() const { return this->animation_length_; }
      light::AddressableLightState *get_stepLight(const uint32_t pos);
      uint32_t size() { return steps_.size(); }
      light::ESPRangeView *get_step(uint32_t num) { return num < steps_.size() ? &(steps_[num]) : nullptr; }
      void schedule_show() { ((light::AddressableLight *)source_light_->get_output())->schedule_show(); }

      // Animation control methods
      void start_run(bool reversed = false, uint32_t animation_length = 0, const std::string &animation_name = "");
      void stop_run();
      bool is_running() const { return this->running_; }

      void write_state(light::LightState *state) override;
      light::LightTraits get_traits() override;

    protected:
      void turn_on_step_(uint32_t step);

      light::AddressableLightState *source_light_{nullptr};
      uint32_t animation_length_{0};
      std::string name_{};
      std::vector<light::ESPRangeView> steps_{};
      std::vector<StairsLightAnimation *> animations_{};

      // Animation state
      bool running_{false};
      uint32_t current_animation_length_{0};
      StairsLightAnimation *current_animation_;

    private:
      std::string target_settings_light_name_;
    };

    template <typename... Ts>
    class StepOnAction : public Action<Ts...>
    {
      u_int32_t time_elapsed_{0};

    public:
      explicit StepOnAction(StairsLight *parent) : parent_(parent) {}

      void set_step(uint32_t step) { this->step_ = step; }
      void set_brightness(float brightness) { this->brightness_ = brightness; }
      void set_color(const std::string &color_name, float value)
      {
        if (color_name == "red")
          this->red_ = value;
        else if (color_name == "green")
          this->green_ = value;
        else if (color_name == "blue")
          this->blue_ = value;
      }
      void set_transition_length(uint32_t length) { this->transition_length_ = length; }

      void play(Ts... x) override
      {
        auto time_interval = 20;
        time_elapsed_ = 0;
        auto *step = parent_->get_step(step_);

        // Создаем интервал (например, каждые 20мс для 50 FPS)
        App.scheduler.set_interval(this->parent_, "fade_task", time_interval, [time_interval, step, this, x...]() mutable
                                   {
          this->time_elapsed_ += time_interval;
          float progress = this->time_elapsed_ / this->transition_length_; // Шаг изменения (скорость)

          if (progress <= 1.0f) {
            // Плавный расчет цвета (от 0 до 255)
            uint8_t val = static_cast<uint8_t>(progress * 255);
            step->set_red(this->red_ * 255 * progress); // Плавный красный
            step->set_green(this->green_ * 255 * progress); // Плавный красный
            step->set_blue(this->blue_ * 255 * progress); // Плавный красный
            this->parent_->schedule_show();
          } else {
            App.scheduler.cancel_interval( this->parent_,"fade_task");
           
          } });
      }

    protected:
      StairsLight *parent_;
      uint32_t step_{0};
      float brightness_{1.0f};
      float red_{0.0f}, green_{0.0f}, blue_{0.0f};
      std::string effect_{""};
      uint32_t transition_length_{0};
    };

    template <typename... Ts>
    class RunAction : public Action<Ts...>
    {
    public:
      explicit RunAction(StairsLight *parent) : parent_(parent) {}

      void set_reversed(bool reversed) { this->reversed_ = reversed; }
      void set_animation_length(uint32_t length) { this->animation_length_ = length; }
      void set_animation(const std::string &animation) { this->animation_ = animation; }

      void play(Ts... x) override
      {
        this->parent_->start_run(this->reversed_, this->animation_length_, this->animation_);
      }

    protected:
      StairsLight *parent_{nullptr};
      bool reversed_{false};
      uint32_t animation_length_{0};
      std::string animation_{""};
    };

  } // namespace stairs_light
} // namespace esphome
