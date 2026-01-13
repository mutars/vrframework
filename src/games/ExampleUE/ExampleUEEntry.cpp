#include "ExampleUEEntry.h"
#include "ExampleUERendererModule.h"
#include "ExampleUECameraModule.h"
#include <spdlog/spdlog.h>

std::optional<std::string> ExampleUEEntry::on_initialize() {
    spdlog::info("ExampleUEEntry::on_initialize - Installing hooks");
    
    // Install renderer hooks for frame/render lifecycle
    ExampleUERendererModule::get()->installHooks();
    
    // Install camera hooks for view/projection matrix injection
    ExampleUECameraModule::get()->installHooks();
    
    spdlog::info("ExampleUEEntry::on_initialize - Complete");
    return std::nullopt;
}

void ExampleUEEntry::on_draw_ui() {
    if (!ImGui::CollapsingHeader(get_name().data())) {
        return;
    }
    
    m_hudScale->draw("HUD Scale");
    m_decoupledPitch->draw("Decoupled Pitch");
}
