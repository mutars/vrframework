#pragma once

#include "Mod.hpp"

class ExampleUEEntry : public Mod {
public:
    static std::shared_ptr<ExampleUEEntry>& get() {
        static auto inst = std::make_shared<ExampleUEEntry>();
        return inst;
    }

    std::string_view get_name() const override { return "ExampleUE VR"; }
    std::optional<std::string> on_initialize() override;
    void on_draw_ui() override;

private:
    ModSlider::Ptr m_hudScale = ModSlider::create("HudScale", 0.1f, 1.0f, 0.5f);
    ModToggle::Ptr m_decoupledPitch = ModToggle::create("DecoupledPitch", false);
};
