#pragma once

#include "esphome/components/light/light_effect.h"
#include "stairs_light.h"

namespace esphome
{
    namespace stairs_light
    {
        class StairsLightAnimation : public light::LightEffect
        {
            using light::LightEffect::start;

        public:
            StairsLightAnimation(const std::string &name) : light::LightEffect(name) {}

            void init_internal(StairsLight *stairs_light)
            {
                stairs_light_ = stairs_light;
                const auto &settings = stairs_light->get_settings();
                state_ = &settings;
            }

            void start(bool reverce)
            {
                reverce_ = reverce;
                this->start();
            }

        protected:
            const StairsSettings *state_;
            uint32_t animation_length() { return stairs_light_->get_animation_length(); }
            uint32_t size() { return stairs_light_->size(); }
            light::ESPRangeView *step(uint32_t num) { return stairs_light_->get_step(reverce_ ? size() - 1 - num : num); }

            bool reverce_{false};

        private:
            StairsLight *stairs_light_{nullptr};
        };

        class StairsLightFadeAnimation : public StairsLightAnimation
        {
        public:
            StairsLightFadeAnimation(const std::string &name) : StairsLightAnimation(name) {}

            void start() override
            {
                if (size() == 0)
                {
                    ESP_LOGW("StairsLightFadeAnimation", "No steps configured");
                    return;
                }
                step_transiton_length_ = animation_length() / size();
                cur_step_num_ = 0;
                start_time_ = millis();
                red_ = state_->r;
                green_ = state_->g;
                blue_ = state_->b;
                ESP_LOGD("StairsLightFadeAnimation", "Starting fade animation: %d steps, %dms per step",
                         size(), step_transiton_length_);
            }

            void apply() override
            {
                auto now = millis();
                if (now < start_time_ + animation_length())
                {
                    auto elapsed = now - start_time_;
                    auto step_num = elapsed / step_transiton_length_;

                    if (cur_step_num_ != step_num && step_num != 0 && step_num <= size())
                    {
                        set_colors(cur_step_num_, state_->brightness * 255);
                    }
                    // Bounds check
                    if (step_num < size())
                    {
                        auto progress = (elapsed - step_transiton_length_ * step_num) / (float)step_transiton_length_;
                        set_colors(step_num, progress * state_->brightness * 255);
                        cur_step_num_ = step_num;
                    }
                }
            }

        private:
            uint32_t start_time_{0};
            uint32_t step_transiton_length_{0};
            uint32_t cur_step_num_{0};
            float red_{0}, green_{0}, blue_{0};

            void set_colors(uint32_t step_num, uint32_t brightness)
            {
                step(step_num)->set_rgb(
                    red_ * brightness,
                    green_ * brightness,
                    blue_ * brightness);

                ESP_LOGD("StairsLightFadeAnimation", "Step %d: R=%.2f G=%.2f B=%.2f Br=%.2f%",
                         step_num, red_, green_, blue_, brightness / 255.f);
            };
        };

        class StairsLightArrowAnimation : public StairsLightAnimation
        {
            uint32_t frame_cnt{0};
            uint32_t cur_frame{0};
            uint32_t frame_lenght{0};
            uint32_t start_time;
            uint32_t red_{0}, green_{0}, blue_{0};
            uint32_t last_step_num_{0};
            uint32_t increment_{1};

        public:
            StairsLightArrowAnimation(const std::string &name) : StairsLightAnimation(name) {}

            void set_increment(uint32_t inc) { increment_ = inc; }
            void start() override
            {
                if (size() == 0)
                {
                    ESP_LOGW("StairsLightArrowAnimation", "No steps configured");
                    return;
                }
                last_step_num_ = size() - 1;
                auto last_step = step(last_step_num_);
                frame_cnt = size() + last_step->size() / (2 * increment_);
                ESP_LOGD("StairsLightArrowAnimation", "Frames count: %u", frame_cnt);
                frame_lenght = animation_length() / frame_cnt;
                cur_frame = 0;
                start_time = millis();
                red_ = state_->r * state_->brightness * 255;
                green_ = state_->g * state_->brightness * 255;
                blue_ = state_->b * state_->brightness * 255;
            }

            void apply() override
            {
                if (frame_cnt > 0)
                {
                    auto elapsed = millis() - start_time;
                    auto frame = elapsed / frame_lenght;
                    if (cur_frame < frame + 1)
                    {
                        // ESP_LOGD("StairsLightArrowAnimation", "Frame %u", frame);
                        for (int step_num = std::min(last_step_num_, frame); step_num >= 0; step_num--)
                        {
                            // ESP_LOGD("StairsLightArrowAnimation", "   Step %u", step_num);

                            auto s = step(step_num);
                            auto base_width = s->size() % 2 == 1 ? 1 : 2;
                            auto width = (frame - step_num) * increment_ * 2 + base_width;
                            if (width <= s->size() + increment_ * 2)
                            {
                                auto start = std::max(((s->size() - (int)width) / 2), 0);
                                auto end = std::min(start + width - 1, (uint32_t)s->size() - 1);
                                // ESP_LOGD("StairsLightArrowAnimation", "      Base width: %u, Width: %u, Size: %u, Start: %u, End: %u",
                                //         base_width, width, s->size(), start, end);

                                for (int i = start; i <= end; i++)
                                {
                                    // ESP_LOGD("StairsLightArrowAnimation", "      led: %u, red: %u, green: %u, blue: %u", i, red_, green_, blue_);
                                    (*s)[i].set_rgb(red_, green_, blue_);
                                }
                            }
                        }

                        cur_frame = frame + 1;
                    }
                }
            }
        };

        class EntryStepsAnimation : public StairsLightAnimation
        {
        public:
            EntryStepsAnimation(std::string &name) : StairsLightAnimation(name) {}
            const uint32_t transition_length{5000};
            void set_brightness(float brightness) { brightness_ = brightness; }
            void start() override { 
                start_time_ = millis(); 
                ESP_LOGD("EntryStepsAnimation", "Animation started!");
            }
            void apply() override
            {
                auto progress = std::min(1.0, (millis() - start_time_) * 1.0 / transition_length);

                auto entryColor = Color(
                    (uint8_t)(state_->r * brightness_ * 255),
                    (uint8_t)(state_->g * brightness_ * 255),
                    (uint8_t)(state_->b * brightness_ * 255));
                for (int i = 0; i < size(); i++)
                {
                    auto s = step(i);
                    auto targetColor = i == 0 || i + 1 == size()
                                           ? entryColor
                                           : Color::BLACK;
                    for (int l = 0; l < s->size(); l++)
                    {
                        auto curColor = (*s)[l].get();
                        (*s)[l] = lerp_color(curColor, targetColor, progress);
                    }
                }
            };

        private:
            float brightness_{0.5};
            uint16_t start_time_{0};

            Color lerp_color(Color a, Color b, float t)
            {
                return Color(
                    esphome::lerp(t, a.red, b.red),
                    esphome::lerp(t, a.green, b.green),
                    esphome::lerp(t, a.blue, b.blue),
                    esphome::lerp(t, a.white, b.white));
            }
        };
    }
}
