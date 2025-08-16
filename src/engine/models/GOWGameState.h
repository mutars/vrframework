#pragma once
#include <engine/memory/offsets.h>

struct GOWRGameState
{
    enum GameState {
        G_UNK,
        G_Menu,
        G_Game,
    };

    inline void setEnforceFlatScreen(bool enforce) {
        enforceFlatScreen = enforce;
        m_current_state = G_UNK;
    }
    bool enforceFlatScreen{false};

    GameState m_current_state{G_UNK};

    static inline void disableBadSettings() {
        static auto windowed_mode_fn = memory::setWindowModeFn();
        windowed_mode_fn(0);
        static auto aspect_ratio_fn = memory::setAspectRatioFn();
        aspect_ratio_fn(0);
        static auto frame_rate_fn = memory::setFrameRateFn();
        frame_rate_fn(0);
        static auto vsync_fn = memory::setVSyncFn();
        vsync_fn(0);
        static auto framegen_mode_fn = memory::setFramegenModeFn();
        framegen_mode_fn(0);
    }
};