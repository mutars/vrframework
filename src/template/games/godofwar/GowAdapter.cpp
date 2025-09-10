#include "GowAdapter.hpp"

#include "mods/GOWVRConfig.hpp"
#include "mods/VR.hpp"
#include "engine/EngineEntry.h"

const char* GowAdapter::getGameKey() {
    return "gow";
}

std::string GowAdapter::getConfigFileName() {
    return "gowvr_config.txt";
}

std::string GowAdapter::getImguiIniName() {
    return "gowvr_ui.ini";
}

std::vector<std::shared_ptr<Mod>> GowAdapter::createMods() {
    return {
        GOWVRConfig::get(),
        VR::get(),
        EngineEntry::Get()
    };
}
