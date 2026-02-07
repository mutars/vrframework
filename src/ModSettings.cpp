#include "ModSettings.h"

namespace ModSettings {
    InternalSettings g_internalSettings;

    bool showFlatScreenDisplay()
    {
        return g_internalSettings.forceFlatScreen || g_internalSettings.showQuadDisplay;
    }
}
