// MM Modern pack for 2 Ship 2 Harkinian (Switch).
// Port of the Zelda64Recomp "Modern Moves" mod. Same features, same defaults.
#ifndef MP_COMMON_H
#define MP_COMMON_H

#include "global.h"

#ifdef __cplusplus
extern "C" {
#endif

// Settings live in the 2Ship config under gModernPack.<name>. 0 = default for that option.
s32 MP_CfgOn(const char* key);
f32 MP_CfgNum(const char* key);
s32 MP_CfgInt(const char* key);
#define CFG_ON(key) MP_CfgOn(key)
#define CFG_NUM(key) MP_CfgNum(key)

// Fixed button layout (set as the controller defaults in BenPort.cpp):
//   A = N64 A, B = jump (N64 L), Y = N64 B, X = Tatl (C-Up), R = quick menu (C-Left),
//   ZR = use item (C-Right), ZL = target (Z), L = shield (N64 R), + = Start
#define MP_JUMP_BTN BTN_L
#define MP_MENU_BTN BTN_CLEFT

s32 Mod_StickNavX(Input* input, s32* repeatTimer, s32 useDpad);

typedef enum {
    MOD_LABEL_MENU,  // hold for quick item menu (R1 / R / RB)
    MOD_LABEL_ITEM,  // use item (R2 / ZR / RT)
    MOD_LABEL_SKIP,  // open skip prompt (Triangle / X / Y)
    MOD_LABEL_START, // confirm skip (Options / + / Menu)
    MOD_LABEL_SORT,  // sort in quick menu (Square / Y / X); also the N64 B button
    MOD_LABEL_A,     // N64 A button (Circle / A / B)
} ModLabel;
const char* Mod_Label(ModLabel which);
s32 MP_LabelStyle(void); // 0 PlayStation, 1 Switch, 2 Xbox

extern s32 gModJumpPressed;
extern s32 gModJumpHeld;
extern const char* gModItemNames[0x50];

void Mod_StrCpy(char* dst, const char* src);
s32 Mod_StrEq(const char* a, const char* b);
void Mod_StrCat(char* dst, const char* src);
void Mod_StrCatInt(char* dst, s32 value);

// Modules
s32 Switcher_Update(PlayState* play, Input* input);
void AutoShield_Update(PlayState* play, Input* input);
void QuickSave_Update(PlayState* play, Input* input);
void PauseSave_Update(PlayState* play, Input* input);
void ThirdPersonAim_Update(PlayState* play, Input* input);
s32 MP_DoSave(PlayState* play); // returns true if saved
s32 MP_CanSaveHere(PlayState* play);

// Called from the game code (edited in place)
void ModernPack_OnPlayUpdateMain(PlayState* play);
void ModernPack_OnFileSelectMain(GameState* thisx);
void ModernPack_BeforeInterfaceDraw(PlayState* play);
void ModernPack_AfterInterfaceDraw(PlayState* play);
s32 ModernPack_TryJump(Player* player, PlayState* play);
void ModernPack_UpdateSprint(Player* player, PlayState* play, Input* input);
f32 ModernPack_SpeedScale(Player* player, s32 linearMode);
s32 ModernPack_FastClimb(Player* player, Input* input);
s32 ModernPack_ThirdPersonAimOn(void);
void ModernPack_AfterItemButtons(Player* player, PlayState* play, Input* input);

#ifdef __cplusplus
}
#endif

#endif
