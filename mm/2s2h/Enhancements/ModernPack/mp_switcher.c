// Quick item switcher (OoT remake / TotK style).
// Hold the switcher button: a row of your items appears. Move the stick left/right to pick one,
// then press C-Left, C-Down or C-Right to put it on that button. Release to close.
#include "mp_common.h"
#include "mp_ui.h"

#define VISIBLE_SLOTS 5
#define CENTER (VISIBLE_SLOTS / 2)
#define ICON_SIZE 78.0f
#define CENTER_ICON_SIZE 96.0f
#define MAX_ENTRIES 48

typedef struct {
    u8 item;
    u8 slot;
} SwitcherEntry;


static SwitcherEntry sEntries[MAX_ENTRIES];
static s32 sNumEntries = 0;
static s32 sSelected = 0;
static s32 sOpen = false;
static s32 sNavTimer = 0;
static s32 sFlashTimer = 0;
static s32 sSortRecent = false;
static u32 sLastUsed[MAX_ENTRIES]; // per inventory slot: higher = more recently equipped
static u32 sUseCounter = 0;

static void BuildEntries(void) {
    s32 slot;
    s32 lastSlot = CFG_ON("switcher_masks") ? SLOT_MASK_FIERCE_DEITY : SLOT_BOTTLE_6;

    sNumEntries = 0;
    for (slot = 0; (slot <= lastSlot) && (sNumEntries < MAX_ENTRIES); slot++) {
        u8 item = gSaveContext.save.saveInfo.inventory.items[slot];
        if ((item != ITEM_NONE) && (item < 0x50)) {
            sEntries[sNumEntries].item = item;
            sEntries[sNumEntries].slot = slot;
            sNumEntries++;
        }
    }

    // "Recently Selected": most recently equipped first, the rest keep their usual order.
    if (sSortRecent) {
        s32 i, j;
        for (i = 1; i < sNumEntries; i++) {
            SwitcherEntry e = sEntries[i];
            u32 key = sLastUsed[e.slot];
            j = i - 1;
            while ((j >= 0) && (sLastUsed[sEntries[j].slot] < key)) {
                sEntries[j + 1] = sEntries[j];
                j--;
            }
            sEntries[j + 1] = e;
        }
    }
}

static void Refresh(void) {
    // Drawn every frame in Switcher_Draw.
}

static void Equip(PlayState* play, s32 btn, u8 item, u8 slot) {
    s32 other;
    u8 oldItem = BUTTON_ITEM_EQUIP(0, btn);
    u8 oldSlot = C_SLOT_EQUIP(0, btn);

    if (oldSlot == slot) {
        return;
    }
    // If the item is already on another C button, swap the two (like the pause menu does).
    for (other = EQUIP_SLOT_C_LEFT; other <= EQUIP_SLOT_C_RIGHT; other++) {
        if ((other != btn) && (C_SLOT_EQUIP(0, other) == slot)) {
            BUTTON_ITEM_EQUIP(0, other) = oldItem;
            C_SLOT_EQUIP(0, other) = oldSlot;
            Interface_LoadItemIconImpl(play, other);
        }
    }
    BUTTON_ITEM_EQUIP(0, btn) = item;
    C_SLOT_EQUIP(0, btn) = slot;
    Interface_LoadItemIconImpl(play, btn);
    if (slot < MAX_ENTRIES) {
        sLastUsed[slot] = ++sUseCounter;
    }
}

static s32 CanOpen(PlayState* play) {
    Player* player = GET_PLAYER(play);

    return (player != NULL) && (gSaveContext.gameMode == GAMEMODE_NORMAL) && !IS_PAUSED(&play->pauseCtx) &&
           (play->msgCtx.msgMode == MSGMODE_NONE) && !Play_InCsMode(play) &&
           (play->transitionTrigger == TRANS_TRIGGER_OFF) &&
           !(player->stateFlags1 & (PLAYER_STATE1_DEAD | PLAYER_STATE1_20000000 | PLAYER_STATE1_800000));
}

static void Close(void) {
    sOpen = false;
}

s32 Switcher_Update(PlayState* play, Input* input) {
    u16 holdBtn;
    s32 nav;

    if (!CFG_ON("switcher_enabled")) {
        Close();
        return false;
    }
    holdBtn = MP_MENU_BTN;

    if (!sOpen) {
        if (CHECK_BTN_ALL(input->press.button, holdBtn) && CanOpen(play)) {
            BuildEntries();
            // Start on whatever is on C-Right if it's in the list.
            sSelected = 0;
            {
                s32 i;
                for (i = 0; i < sNumEntries; i++) {
                    if (sEntries[i].slot == C_SLOT_EQUIP(0, EQUIP_SLOT_C_RIGHT)) {
                        sSelected = i;
                    }
                }
            }
            sNavTimer = 0;
            sFlashTimer = 0;
            sOpen = true;
            Audio_PlaySfx(NA_SE_SY_WIN_OPEN);
            return true;
        }
        return false;
    }

    if (!CHECK_BTN_ALL(input->cur.button, holdBtn) || !CanOpen(play)) {
        // Remake style: letting go puts the highlighted item on the item button (C-Right).
        if (CanOpen(play) && (sNumEntries > 0)) {
            Equip(play, EQUIP_SLOT_C_RIGHT, sEntries[sSelected].item, sEntries[sSelected].slot);
            Audio_PlaySfx(NA_SE_SY_DECIDE);
        } else {
            Audio_PlaySfx(NA_SE_SY_WIN_CLOSE);
        }
        Close();
        return true;
    }

    if (CHECK_BTN_ALL(input->press.button, BTN_B)) {
        u8 curSlot = (sNumEntries > 0) ? sEntries[sSelected].slot : SLOT_NONE;
        s32 i;

        sSortRecent = !sSortRecent;
        BuildEntries();
        sSelected = 0;
        for (i = 0; i < sNumEntries; i++) {
            if (sEntries[i].slot == curSlot) {
                sSelected = i;
            }
        }
        Audio_PlaySfx(NA_SE_SY_DECIDE);
        Refresh();
    }

    nav = Mod_StickNavX(input, &sNavTimer, true);
    if ((nav != 0) && (sNumEntries > 0)) {
        s32 next = sSelected + nav;
        if ((next >= 0) && (next < sNumEntries)) {
            sSelected = next;
            sFlashTimer = 0;
            Audio_PlaySfx(NA_SE_SY_CURSOR);
            Refresh();
        }
    }

    if (sFlashTimer > 0) {
        sFlashTimer--;
        if (sFlashTimer == 0) {
            Refresh();
        }
    }
    return true;
}

// ---- Drawing: OoT remake style, using Majora's Mask's own font and item icons ----
#define ROW_Y 112.0f
#define SPACING 46.0f

static s32 EntryAmmo(u8 item) {
    switch (item) {
        case ITEM_BOW:
        case ITEM_BOW_FIRE:
        case ITEM_BOW_ICE:
        case ITEM_BOW_LIGHT:
            return AMMO(ITEM_BOW);
        case ITEM_BOMB:
        case ITEM_BOMBCHU:
        case ITEM_DEKU_STICK:
        case ITEM_DEKU_NUT:
        case ITEM_MAGIC_BEANS:
        case ITEM_POWDER_KEG:
            return AMMO(item);
        default:
            return -1;
    }
}

// Remake-style item tile: brown square with an ornamented frame; the selected one is tan with a white glow.
static void DrawTile(Gfx** gfxP, f32 x, f32 y, f32 size, s32 selected, u8 a) {
    if (selected) {
        Ui_Tex(gfxP, gUiGlowFrameTex, false, 64, 64, x - 7, y - 7, size + 14, size + 14, 255, 255, 255, a);
        Ui_Rect(gfxP, x, y, size, size, 150, 113, 66, (u8)((a * 23) / 25));
        Ui_Tex(gfxP, gUiTileFrameTex, false, 64, 64, x, y, size, size, 232, 196, 140, (u8)((a * 6) / 7));
    } else {
        Ui_Rect(gfxP, x, y, size, size, 52, 38, 24, (u8)((a * 9) / 10));
        Ui_Tex(gfxP, gUiTileFrameTex, false, 64, 64, x, y, size, size, 130, 100, 66, a);
    }
}

void Switcher_Draw(PlayState* play, Gfx** gfxP) {
    s32 i;
    f32 cx = SCREEN_WIDTH / 2.0f;
    const char* sortName = sSortRecent ? "Recently Selected" : "Default";

    if (!sOpen) {
        return;
    }

    // Darken the game behind the menu (full widescreen width)
    Ui_Align(gfxP, UI_ALIGN_STRETCH);
    Ui_Rect(gfxP, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, 150);
    Ui_Align(gfxP, UI_ALIGN_CENTER);

    // Header: sort name on a dark rounded band, two page dots, "Sort" + button
    {
        f32 tw = Ui_TextWidth(sortName, 0.62f);
        Ui_Tex(gfxP, gUiPanelTex, false, 64, 32, cx - tw / 2 - 12, 42, tw + 24, 21, 0, 0, 0, 130);
        Ui_Text(gfxP, cx, 44.5f, sortName, 0.62f, 1, 245, 243, 232, 255);
        Ui_Dot(gfxP, cx - 4.2f, 57.5f, 2.6f, 2.6f, 255, 255, 255, sSortRecent ? 110 : 255);
        Ui_Dot(gfxP, cx + 1.6f, 57.5f, 2.6f, 2.6f, 255, 255, 255, sSortRecent ? 255 : 110);

        {
            f32 group;
            tw = Ui_TextWidth("Sort", 0.58f);
            group = tw + 3.0f + 9.0f;
            Ui_Text(gfxP, cx - group / 2, 67.0f, "Sort", 0.58f, 0, 245, 243, 232, 255);
            Ui_ButtonIcon(gfxP, cx - group / 2 + tw + 3.0f, 67.5f, 9.0f, Mod_Label(MOD_LABEL_SORT), 255);
        }
    }

    // Item row
    for (i = -(VISIBLE_SLOTS / 2); i <= VISIBLE_SLOTS / 2; i++) {
        s32 idx = sSelected + i;
        s32 dist = (i < 0) ? -i : i;
        s32 sel = (i == 0);
        f32 size = sel ? 34.0f : 31.0f;
        f32 x = cx + i * SPACING - size / 2;
        f32 y = ROW_Y - size / 2;
        s32 ammo;
        u8 a;

        if ((idx < 0) || (idx >= sNumEntries)) {
            continue;
        }
        a = (u8)((dist * 50 > 200) ? 55 : (255 - dist * 50));
        DrawTile(gfxP, x, y, size, sel, a);
        Ui_Icon(gfxP, sEntries[idx].item, x + 2.0f, y + 2.0f, size - 4.0f, a);
        ammo = EntryAmmo(sEntries[idx].item);
        if (ammo >= 0) {
            char text[8];
            text[0] = 0;
            Mod_StrCatInt(text, ammo);
            Ui_Text(gfxP, x + size - 2.0f, y + size - 11.0f, text, 0.7f, 2, 255, 255, 255, a);
        }
    }
    if (sFlashTimer > 0) {
        Ui_Tex(gfxP, gUiGlowFrameTex, false, 64, 64, cx - 24, ROW_Y - 24, 48, 48, 120, 255, 140, 255);
    }
    // Arrows beside the selected tile
    Ui_Tex(gfxP, gUiChevronLTex, true, 16, 32, cx - 27.0f, ROW_Y - 6.0f, 5.0f, 10.0f, 255, 255, 255,
           (sSelected > 0) ? 255 : 60);
    Ui_Tex(gfxP, gUiChevronRTex, true, 16, 32, cx + 22.0f, ROW_Y - 6.0f, 5.0f, 10.0f, 255, 255, 255,
           (sSelected < sNumEntries - 1) ? 255 : 60);

    // Item name
    Ui_Text(gfxP, cx, ROW_Y + 30.0f, (sNumEntries > 0) ? gModItemNames[sEntries[sSelected].item] : "No items yet",
            0.95f, 1, 250, 248, 238, 255);

    // "Release [R1]"
    {
        f32 tw = Ui_TextWidth("Release", 0.58f);
        f32 kw = Ui_KeyCapWidth(Mod_Label(MOD_LABEL_MENU), 0.55f);
        f32 total = tw + 4.0f + kw;
        Ui_Text(gfxP, cx - total / 2 + tw / 2, ROW_Y + 54.0f, "Release", 0.58f, 1, 245, 243, 232, 255);
        Ui_KeyCap(gfxP, cx + total / 2 - kw / 2, ROW_Y + 55.5f, Mod_Label(MOD_LABEL_MENU), 0.55f, 255);
    }
}
