// Settings for the MM Modern pack. Everything defaults to the same values as the PC mod, so there is
// nothing to set up. Each can be changed in the 2Ship config file as gModernPack.<name>.
#include <libultraship/bridge/consolevariablebridge.h>
#include <string>
#include <cstring>

extern "C" {
#include "mp_common.h"
}

static const char* sOffByDefault[] = {
    "big_wallets", "cheat_health", "cheat_magic", "cheat_rupees", "cheat_ammo", "cheat_moon_jump", "cheat_freeze_time",
};

struct NumDefault {
    const char* key;
    float value;
};
static const NumDefault sNumDefaults[] = {
    { "jump_height", 8.0f }, { "sprint_speed", 1.4f }, { "swim_speed", 1.6f }, { "skip_rate", 400.0f },
};

extern "C" s32 MP_CfgOn(const char* key) {
    int def = 1;
    for (const char* k : sOffByDefault) {
        if (strcmp(k, key) == 0) {
            def = 0;
        }
    }
    return CVarGetInteger(("gModernPack." + std::string(key)).c_str(), def) != 0;
}

extern "C" s32 MP_CfgInt(const char* key) {
    return CVarGetInteger(("gModernPack." + std::string(key)).c_str(), 0);
}

extern "C" f32 MP_CfgNum(const char* key) {
    float def = 0.0f;
    for (const NumDefault& d : sNumDefaults) {
        if (strcmp(d.key, key) == 0) {
            def = d.value;
        }
    }
    return CVarGetFloat(("gModernPack." + std::string(key)).c_str(), def);
}

// 0 PlayStation, 1 Switch, 2 Xbox. Switch labels by default on this build.
extern "C" s32 MP_LabelStyle(void) {
    int c = CVarGetInteger("gModernPack.button_labels", 1);
    return (c < 0 || c > 2) ? 1 : c;
}
