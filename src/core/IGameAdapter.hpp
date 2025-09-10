#pragma once

#include <string>
#include <vector>
#include <memory>

class Mod;
class VRFramework;

class IGameAdapter {
public:
    virtual ~IGameAdapter() = default;

    virtual const char* getGameKey() = 0;
    virtual std::string getConfigFileName() = 0;
    virtual std::string getImguiIniName() = 0;
    virtual std::vector<std::shared_ptr<Mod>> createMods() = 0;

    // Optional hooks
    virtual void preInit(VRFramework& framework) {}
    virtual bool onMessage(HWND wnd, UINT message, WPARAM w_param, LPARAM l_param) { return true; }
};
