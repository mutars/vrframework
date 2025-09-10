#pragma once

#include "core/IGameAdapter.hpp"

class GowAdapter : public IGameAdapter {
public:
    const char* getGameKey() override;
    std::string getConfigFileName() override;
    std::string getImguiIniName() override;
    std::vector<std::shared_ptr<Mod>> createMods() override;
};
