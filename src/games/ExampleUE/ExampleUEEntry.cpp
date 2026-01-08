#include "ExampleUEEntry.h"

#include "ExampleUECameraModule.h"
#include "ExampleUERendererModule.h"
#include <imgui.h>

std::optional<std::string> ExampleUEEntry::on_initialize() {
    ExampleUERendererModule::get()->installHooks();
    ExampleUECameraModule::get()->installHooks();
    return std::nullopt;
}

void ExampleUEEntry::on_draw_ui() {
    if (!ImGui::CollapsingHeader(get_name().data())) {
        return;
    }

    m_hudScale->draw("HUD Scale");
    m_decoupledPitch->draw("Decoupled Pitch");
}
