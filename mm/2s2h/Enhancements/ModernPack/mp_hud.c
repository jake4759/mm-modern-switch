// Remake-style HUD: hides the N64 A / B / C / Start buttons and shows a single item slot in the top right,
// labelled with your controller's buttons (hold R1 = quick item menu above it, R2 = use item below it).
#include "mp_common.h"
#include "mp_ui.h"


static s16 sVanillaCRightAlpha = 0;

void Hud_Init(void) {
}

static s32 GetAmmo(u8 item) {
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


// Name of what a button currently does (the game's own "do action"), or NULL if nothing worth showing.
static const char* DoActionName(s32 action) {
    switch (action) {
        case DO_ACTION_ATTACK: return "Attack";
        case DO_ACTION_CHECK: return "Check";
        case DO_ACTION_ENTER: return "Enter";
        case DO_ACTION_RETURN: return "Return";
        case DO_ACTION_OPEN: return "Open";
        case DO_ACTION_JUMP: return "Jump";
        case DO_ACTION_DIVE: return "Dive";
        case DO_ACTION_FASTER: return "Faster";
        case DO_ACTION_THROW: return "Throw";
        case DO_ACTION_CLIMB: return "Climb";
        case DO_ACTION_DROP: return "Drop";
        case DO_ACTION_DOWN: return "Down";
        case DO_ACTION_SPEAK: return "Speak";
        case DO_ACTION_GRAB: return "Grab";
        case DO_ACTION_STOP: return "Stop";
        case DO_ACTION_PUTAWAY: return "Put Away";
        case DO_ACTION_REEL: return "Reel";
        case DO_ACTION_WARP: return "Warp";
        case DO_ACTION_SNAP: return "Snap";
        case DO_ACTION_EXPLODE: return "Explode";
        case DO_ACTION_DANCE: return "Dance";
        case DO_ACTION_MARCH: return "March";
        case DO_ACTION_CURL: return "Curl";
        case DO_ACTION_SURFACE: return "Surface";
        case DO_ACTION_SWIM: return "Swim";
        case DO_ACTION_PUNCH: return "Punch";
        case DO_ACTION_POUND: return "Pound";
        case DO_ACTION_HOOK: return "Hook";
        case DO_ACTION_SHOOT: return "Shoot";
        default: return NULL;
    }
}

// "[button] Action" tag, right edge at xRight, hand-lettered like the R1 label.
static void DrawActionPrompt(Gfx** gfxP, f32 xRight, f32 y, const char* button, const char* word, u8 a) {
    f32 capH = 5.0f;
    f32 tw = Ui_HandTextWidth(word, capH);
    f32 icon = 9.0f;
    f32 w = tw + icon + 10.0f;
    Ui_Tex(gfxP, gUiPanelTex, false, 64, 32, xRight - w, y - 1.5f, w, 12.0f, 0, 0, 0, (u8)((a * 1) / 2));
    Ui_ButtonIcon(gfxP, xRight - w + 2.5f, y, icon, button, a);
    Ui_HandText(gfxP, xRight - 3.5f, y + 4.5f, word, capH, 2, a);
}

// ---- Remake-style button diamond near Link: the A-action button lit up with a tag ("Talk", "Open"...),
//      fading in and out ----
static f32 sTalkFade = 0.0f;
static char sTalkWord[16];

static void DrawGlyph(Gfx** gfxP, s32 which, f32 cx, f32 cy, f32 size, u8 a) {
    // which: 0 top, 1 left, 2 bottom (inactive buttons). One finished texture each: solid dark disc, ring, symbol.
    u32 c = MP_LabelStyle();
    f32 r = 10.0f;
    if (c == 0) {
        static u8* sTex[3] = { gUiDiaTriangleTex, gUiDiaSquareTex, gUiDiaCrossTex };
        Ui_Tex(gfxP, sTex[which], true, 32, 32, cx - r / 2, cy - r / 2, r, r, 255, 255, 255, a);
    } else {
        static const char* sSwitch[3] = { "X", "Y", "B" };
        static const char* sXbox[3] = { "Y", "X", "A" };
        Ui_Tex(gfxP, gUiDiaBlankTex, true, 32, 32, cx - r / 2, cy - r / 2, r, r, 255, 255, 255, a); // disc + ring
        Ui_Label(gfxP, cx, cy, (c == 1) ? sSwitch[which] : sXbox[which], r * 0.38f, 240, 240, 240, a);
    }
}

// Only real interactions get the prompt (not the default "Attack" etc. that's on the button all the time).
static const char* InteractName(s32 action) {
    switch (action) {
        case DO_ACTION_SPEAK: return "Talk";
        case DO_ACTION_CHECK: return "Check";
        case DO_ACTION_OPEN: return "Open";
        case DO_ACTION_ENTER: return "Enter";
        case DO_ACTION_GRAB: return "Grab";
        case DO_ACTION_CLIMB: return "Climb";
        case DO_ACTION_DROP: return "Drop";
        case DO_ACTION_THROW: return "Throw";
        case DO_ACTION_DOWN: return "Down";
        case DO_ACTION_PUTAWAY: return "Put Away";
        case DO_ACTION_SURFACE: return "Surface";
        case DO_ACTION_DIVE: return "Dive";
        case DO_ACTION_JUMP: return "Jump";
        case DO_ACTION_WARP: return "Warp";
        case DO_ACTION_REEL: return "Reel";
        case DO_ACTION_STOP: return "Stop";
        default: return NULL;
    }
}

#define TALK_PROMPT_RANGE 75.0f // game units: about an arm's length from the person

static void DrawTalkPrompt(PlayState* play, Gfx** gfxP, s32 alpha) {
    const char* name = InteractName(play->interfaceCtx.aButtonDoAction);
    Player* player = GET_PLAYER(play);

    // Some characters can be talked to from far away; only show Talk/Check once you're actually close.
    if ((name != NULL) && (player != NULL) &&
        ((play->interfaceCtx.aButtonDoAction == DO_ACTION_SPEAK) ||
         (play->interfaceCtx.aButtonDoAction == DO_ACTION_CHECK))) {
        if ((player->talkActor == NULL) || (player->talkActorDistance > TALK_PROMPT_RANGE)) {
            name = NULL;
        }
    }
    s32 show = CFG_ON("action_prompts") && (name != NULL) && (play->msgCtx.msgMode == MSGMODE_NONE) &&
               (alpha > 0) && !IS_PAUSED(&play->pauseCtx);
    f32 cx = 214.0f, cy = 166.0f, off = 10.5f, capH = 5.6f; // right of Link, clear of the clock
    f32 tw;
    u8 a;

    if (show) {
        Mod_StrCpy(sTalkWord, name);
        sTalkFade += 0.18f;
    } else {
        sTalkFade -= 0.18f;
    }
    if (sTalkFade > 1.0f) {
        sTalkFade = 1.0f;
    }
    if (sTalkFade <= 0.0f) {
        sTalkFade = 0.0f;
        return;
    }
    a = (u8)(sTalkFade * ((alpha > 0) ? alpha : 255));
    Ui_Align(gfxP, UI_ALIGN_CENTER);
    DrawGlyph(gfxP, 0, cx, cy - off, 9.0f, a);
    DrawGlyph(gfxP, 1, cx - off, cy, 9.0f, a);
    DrawGlyph(gfxP, 2, cx, cy + off, 9.0f, a);
    tw = Ui_HandTextWidth(sTalkWord, capH);
    Ui_Tex(gfxP, gUiPanelTex, false, 64, 32, cx + off, cy - 6.5f, tw + 17.0f, 13.0f, 0, 0, 0, (u8)((a * 3) / 5));
    if (MP_LabelStyle() == 0) {
        Ui_Tex(gfxP, gUiDiaCircleOnTex, true, 32, 32, cx + off - 6.0f, cy - 6.0f, 12.0f, 12.0f, 255, 255, 255, a);
    } else {
        Ui_ButtonIcon(gfxP, cx + off - 5.5f, cy - 5.5f, 11.0f, Mod_Label(MOD_LABEL_A), a);
    }
    Ui_HandText(gfxP, cx + off + 8.5f + tw / 2, cy, sTalkWord, capH, 1, a);
}

void Hud_Update(PlayState* play) {
}

void Hud_Draw(PlayState* play, Gfx** gfxP) {
    u8 item;
    s32 ammo;
    s32 a;

    if (!CFG_ON("remake_hud") || (gSaveContext.gameMode != GAMEMODE_NORMAL) || IS_PAUSED(&play->pauseCtx)) {
        return;
    }
    a = sVanillaCRightAlpha;
    if (a <= 0) {
        return;
    }
    if (a > 255) {
        a = 255;
    }

    // ---- Item slot, top right: hand-drawn brown box, hand-lettered "R1" with tapered dots, outlined ammo ----
    Ui_Align(gfxP, UI_ALIGN_RIGHT);
    {
        f32 box = 28.0f;
        f32 bx = SCREEN_WIDTH - 22.0f - box;
        f32 by = 24.0f;
        f32 cx = bx + box / 2;
        f32 capH = 6.4f;
        f32 ly = by - 3.5f - capH / 2; // centre of the label letters
        f32 tw = Ui_HandTextWidth(Mod_Label(MOD_LABEL_MENU), capH);
        s32 side;

        Ui_HandText(gfxP, cx, ly, Mod_Label(MOD_LABEL_MENU), capH, 1, (u8)a);
        // dots: bigger one next to the label, smaller one outside
        for (side = -1; side <= 1; side += 2) {
            f32 x1 = cx + side * (tw / 2 + 2.7f);
            f32 x2 = cx + side * (tw / 2 + 6.0f);
            Ui_Dot(gfxP, x1 - 1.6f, ly - 1.6f, 3.2f, 3.2f, 0, 0, 0, (u8)((a * 3) / 4));
            Ui_Dot(gfxP, x1 - 1.25f, ly - 1.25f, 2.5f, 2.5f, 240, 236, 224, (u8)a);
            Ui_Dot(gfxP, x2 - 1.2f, ly - 1.2f, 2.4f, 2.4f, 0, 0, 0, (u8)((a * 3) / 4));
            Ui_Dot(gfxP, x2 - 0.85f, ly - 0.85f, 1.7f, 1.7f, 240, 236, 224, (u8)a);
        }

        // soft shadow, brushed-paint brown fill, wobbly ink border
        Ui_Tex(gfxP, gUiPanelTex, false, 64, 32, bx + 0.5f, by + 1.5f, box + 1.5f, box + 1.0f, 0, 0, 0, (u8)(a / 3));
        Ui_Tex(gfxP, gUiHandFillTex, false, 64, 64, bx - 0.7f, by - 0.7f, box + 1.4f, box + 1.4f, 92, 66, 40, (u8)a);
        Ui_Tex(gfxP, gUiHandInkTex, false, 64, 64, bx - 0.7f, by - 0.7f, box + 1.4f, box + 1.4f, 38, 26, 14, (u8)a);

        item = BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_C_RIGHT);
        if (item < 0x50) {
            Ui_Icon(gfxP, item, bx + 2.0f, by + 2.0f, box - 4.0f, (u8)a);
            ammo = GetAmmo(item);
            if (ammo >= 0) {
                char text[8];
                text[0] = 0;
                Mod_StrCatInt(text, ammo);
                Ui_HandText(gfxP, bx + box - 2.0f, by + box - 2.5f - 3.9f, text, 7.8f, 2, (u8)a);
            }
        }

        // small R2 (use item) under the box, same hand-lettered style, smaller than R1
        Ui_HandText(gfxP, cx, by + box + 2.5f + 2.3f, Mod_Label(MOD_LABEL_ITEM), 4.6f, 1, (u8)((a * 9) / 10));
    }

    // ---- What your buttons do right now (Dive, Speak, Open, Shoot...), under the item slot ----
    if (CFG_ON("action_prompts") && (play->msgCtx.msgMode == MSGMODE_NONE)) {
        const char* aName = NULL; // A is shown in the button diamond near Link
        const char* bName = DoActionName(play->interfaceCtx.bButtonPlayerDoActionActive
                                             ? play->interfaceCtx.bButtonPlayerDoAction
                                             : DO_ACTION_NONE);
        f32 y = 76.0f;
        if (aName != NULL) {
            DrawActionPrompt(gfxP, SCREEN_WIDTH - 20.0f, y, Mod_Label(MOD_LABEL_A), aName, (u8)a);
            y += 15.0f;
        }
        if (bName != NULL) {
            DrawActionPrompt(gfxP, SCREEN_WIDTH - 20.0f, y, Mod_Label(MOD_LABEL_SORT), bName, (u8)a);
        }
    }

    // ---- Top left under the hearts: D-pad icon and Tatl ----
    if (CFG_ON("dpad_items")) {
        f32 x = 26.0f;
        f32 y = 48.0f;
        Player* player = GET_PLAYER(play);
        s32 hasOcarina = (INV_CONTENT(ITEM_OCARINA_OF_TIME) == ITEM_OCARINA_OF_TIME);
        // Tatl only shows once she's actually with you
        s32 hasTatl = CFG_ON("dpad_right_tatl") && (player != NULL) && (player->tatlActor != NULL);

        Ui_Align(gfxP, UI_ALIGN_LEFT);
        if (hasOcarina || hasTatl) {
            Ui_Tex(gfxP, gUiDpadTex, true, 32, 32, x, y, 14.0f, 14.0f, 255, 255, 255, (u8)((a * 15) / 16));
        }
        if (hasTatl) {
            // dim when she has nothing to say; lit and pulsing when she wants to talk
            s32 calling = (play->interfaceCtx.tatlCalling != 0);
            s32 hint = (player->tatlTextId != 0);
            u8 fa;
            f32 size = 18.0f;

            if (calling || hint) {
                f32 t = (play->state.frames % 30) / 30.0f;
                f32 pulse = (t < 0.5f) ? (t * 2.0f) : (2.0f - t * 2.0f); // 0..1..0
                fa = (u8)a;
                size = 18.0f + pulse * (calling ? 3.0f : 1.5f);
                // soft glow behind her
                Ui_Fairy(gfxP, x + 14.5f - (size + 6.0f - 18.0f) / 2, y - 2.0f - (size + 6.0f - 18.0f) / 2, size + 6.0f,
                         (u8)((a * (calling ? (60 + (s32)(pulse * 80)) : (40 + (s32)(pulse * 40)))) / 255));
            } else {
                fa = (u8)((a * 2) / 5);
            }
            Ui_Fairy(gfxP, x + 14.5f - (size - 18.0f) / 2, y - 2.0f - (size - 18.0f) / 2, size, fa);
        }
        if (hasOcarina) {
            Ui_IconSilhouette(gfxP, ITEM_OCARINA_OF_TIME, x - 15.0f, y, 14.0f, 245, 245, 245, (u8)((a * 4) / 5));
        }
    }
    DrawTalkPrompt(play, gfxP, a);
    Ui_Align(gfxP, UI_ALIGN_CENTER);
}

// Hide the N64 buttons only while the HUD is drawn, then put the game's values back so its own fading
// logic keeps working.
static PlayState* sDrawPlay = NULL;
static s16 sSaved[6];

void ModernPack_BeforeInterfaceDraw(PlayState* play) {
    InterfaceContext* ic = &play->interfaceCtx;

    sDrawPlay = NULL;
    sVanillaCRightAlpha = ic->cRightAlpha;
    if (!CFG_ON("remake_hud")) {
        return;
    }
    sDrawPlay = play;
    sSaved[0] = ic->aAlpha;
    sSaved[1] = ic->bAlpha;
    sSaved[2] = ic->cLeftAlpha;
    sSaved[3] = ic->cDownAlpha;
    sSaved[4] = ic->cRightAlpha;
    sSaved[5] = ic->startAlpha;
    ic->aAlpha = 0;
    ic->bAlpha = 0;
    ic->cLeftAlpha = 0;
    ic->cDownAlpha = 0;
    ic->cRightAlpha = 0;
    ic->startAlpha = 0;
}

void ModernPack_AfterInterfaceDraw(PlayState* play) {
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    Gfx* gfx = gfxCtx->overlay.p;
    InterfaceContext* ic;

    if (play->pauseCtx.debugEditor == DEBUG_EDITOR_NONE) {
        Ui_Begin(gfxCtx, &gfx);
        Hud_Draw(play, &gfx);
        Switcher_Draw(play, &gfx);
        QuickSave_Draw(play, &gfx);
        Songbook_Draw(play, &gfx);
        CutsceneFF_Draw(play, &gfx);
        Ui_End(&gfx);
        gfxCtx->overlay.p = gfx;
    }

    if (sDrawPlay == NULL) {
        return;
    }
    ic = &sDrawPlay->interfaceCtx;
    ic->aAlpha = sSaved[0];
    ic->bAlpha = sSaved[1];
    ic->cLeftAlpha = sSaved[2];
    ic->cDownAlpha = sSaved[3];
    ic->cRightAlpha = sSaved[4];
    ic->startAlpha = sSaved[5];
    sDrawPlay = NULL;
}
