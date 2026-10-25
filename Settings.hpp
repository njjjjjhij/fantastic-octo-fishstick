#pragma once
#include <Geode/Geode.hpp>
#include <algorithm>

namespace w {
    constexpr auto K_CBFH = "cbfh";
    constexpr auto K_FRAMES = "cbfh-frames";
    constexpr auto K_FRAMECOUNT = "frame-counter";
    constexpr auto K_CLICKCOUNT = "click-counter";
    constexpr auto K_DIM = "dim-screen";
    constexpr auto K_FLASH = "click-flash";

    inline bool on(char const* k) {
        return geode::Mod::get()->getSavedValue<bool>(k, false);
    }
    inline void set(char const* k, bool v) {
        geode::Mod::get()->setSavedValue<bool>(k, v);
    }
    // CBFH window, 1-20 (counted in physics steps)
    inline int frames() {
        return std::clamp(geode::Mod::get()->getSavedValue<int>(K_FRAMES, 5), 1, 20);
    }
    inline void setFrames(int v) {
        geode::Mod::get()->setSavedValue<int>(K_FRAMES, std::clamp(v, 1, 20));
    }

    // live per-attempt stats
    inline int g_frames = 0;
    inline int g_clicks = 0;
    inline int g_fixed = 0;
    inline bool g_reset = false;
}
