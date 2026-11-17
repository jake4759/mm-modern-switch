// Main input handling for the MM Modern pack (port of main.c from the PC mod).
#include "mp_common.h"

s32 gModJumpPressed = false;
s32 gModJumpHeld = false;

s32 Mod_StrEq(const char* a, const char* b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return *a == *b;
}

void Mod_StrCpy(char* dst, const char* src) {
    while (*src) {
        *dst++ = *src++;
    }
    *dst = 0;
}

void Mod_StrCat(char* dst, const char* src) {
    while (*dst) {
        dst++;
    }
    Mod_StrCpy(dst, src);
}

void Mod_StrCatInt(char* dst, s32 value) {
    char buf[12];
    s32 i = 0;
    s32 neg = value < 0;
    u32 v = neg ? -value : value;

    do {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    } while (v != 0);
    if (neg) {
        buf[i++] = '-';
    }
    while (*dst) {
        dst++;
    }
    while (i > 0) {
        *dst++ = buf[--i];
    }
    *dst = 0;
}

static const char* sLabels[3][6] = {
    { "R1", "R2", "Triangle", "Options", "Square", "Circle" }, // PlayStation
    { "R", "ZR", "X", "+", "Y", "A" },                         // Switch
    { "RB", "RT", "Y", "Menu", "X", "B" },                     // Xbox
};

const char* Mod_Label(ModLabel which) {
    return sLabels[MP_LabelStyle()][which];
}

s32 Mod_StickNavX(Input* input, s32* repeatTimer, s32 useDpad) {
    s32 x = input->rel.stick_x;
    s32 dir = 0;

    if (useDpad && CHECK_BTN_ANY(input->cur.button, BTN_DRIGHT)) {
        x = 80;
    } else if (useDpad && CHECK_BTN_ANY(input->cur.button, BTN_DLEFT)) {
        x = -80;
    }

    if (x > 45) {
        dir = 1;
    } else if (x < -45) {
        dir = -1;
    }

    if (dir == 0) {
        *repeatTimer = 0;
        return 0;
    }
    if (*repeatTimer == 0) {
        *repeatTimer = 1;
        return dir;
    }
    (*repeatTimer)++;
    // First repeat after 8 frames, then every 3.
    if ((*repeatTimer >= 8) && (((*repeatTimer - 8) % 3) == 0)) {
        return dir;
    }
    return 0;
}

// D-pad acts like the stick in menus (pause menu, text choices, file select).
static void DpadToStick(Input* input) {
    s8 x = 0;
    s8 y = 0;
    u16 dpad = BTN_DUP | BTN_DDOWN | BTN_DLEFT | BTN_DRIGHT;

    if (CHECK_BTN_ANY(input->cur.button, BTN_DRIGHT)) x = 70;
    if (CHECK_BTN_ANY(input->cur.button, BTN_DLEFT)) x = -70;
    if (CHECK_BTN_ANY(input->cur.button, BTN_DUP)) y = 70;
    if (CHECK_BTN_ANY(input->cur.button, BTN_DDOWN)) y = -70;

    if ((x != 0) || (y != 0)) {
        if ((SQ((s32)input->rel.stick_x) + SQ((s32)input->rel.stick_y)) < SQ(20)) {
            input->rel.stick_x = input->cur.stick_x = x;
            input->rel.stick_y = input->cur.stick_y = y;
        }
    }
    input->cur.button &= ~dpad;
    input->press.button &= ~dpad;
    input->rel.button &= ~dpad;
}

void ModernPack_OnFileSelectMain(GameState* thisx) {
    Input* input = CONTROLLER1(thisx);

    // The jump button also confirms on the file select screen.
    if (CFG_ON("jump_enabled")) {
        if (CHECK_BTN_ALL(input->press.button, MP_JUMP_BTN)) {
            input->press.button |= BTN_A;
        }
        if (CHECK_BTN_ALL(input->cur.button, MP_JUMP_BTN)) {
            input->cur.button |= BTN_A;
        }
    }
    if (CFG_ON("dpad_menus")) {
        DpadToStick(input);
    }
}

// True while a normal text box is on screen (printing, waiting for input, or showing choices).
static s32 TextBoxActive(PlayState* play) {
    u8 mode = play->msgCtx.msgMode;
    return ((mode != MSGMODE_NONE) && (mode < MSGMODE_OCARINA_STARTING)) ||
           ((mode >= MSGMODE_TEXT_AWAIT_NEXT) && (mode <= MSGMODE_TEXT_DONE));
}

static void ClearInput(Input* input) {
    input->cur.button = 0;
    input->press.button = 0;
    input->rel.button = 0;
    input->cur.stick_x = input->cur.stick_y = 0;
    input->rel.stick_x = input->rel.stick_y = 0;
    input->press.stick_x = input->press.stick_y = 0;
}

// Runs once per game update, before the game reads input.
void ModernPack_OnPlayUpdateMain(PlayState* play) {
    Input* input = CONTROLLER1(&play->state);

    // Capture and hide the jump button.
    if (CFG_ON("jump_enabled")) {
        gModJumpPressed = CHECK_BTN_ALL(input->press.button, MP_JUMP_BTN);
        gModJumpHeld = CHECK_BTN_ALL(input->cur.button, MP_JUMP_BTN);
        input->press.button &= ~MP_JUMP_BTN;
        input->cur.button &= ~MP_JUMP_BTN;
        input->rel.button &= ~MP_JUMP_BTN;
    } else {
        gModJumpPressed = false;
        gModJumpHeld = false;
    }

    // Pause menu pages: L (N64 R) = previous page, R (quick-menu button) = next page.
    if (IS_PAUSED(&play->pauseCtx) && CFG_ON("bumper_pages")) {
        u16 cur = input->cur.button;
        u16 press = input->press.button;
        u16 newCur = cur & ~(BTN_R | MP_MENU_BTN);
        u16 newPress = press & ~(BTN_R | MP_MENU_BTN);

        if (cur & BTN_R) newCur |= BTN_Z;
        if (press & BTN_R) newPress |= BTN_Z;
        if (cur & MP_MENU_BTN) newCur |= BTN_R;
        if (press & MP_MENU_BTN) newPress |= BTN_R;
        input->cur.button = newCur;
        input->press.button = newPress;
    }

    PauseSave_Update(play, input);

    if (CFG_ON("dpad_menus") && (IS_PAUSED(&play->pauseCtx) || TextBoxActive(play))) {
        DpadToStick(input);
    }

    // Text boxes: while text is still appearing, A, B or the jump button shows the current box at once.
    // Once it's fully shown, the jump button advances it like A.
    if (TextBoxActive(play)) {
        s32 jumpText = CFG_ON("jump_advances_text");

        if (play->msgCtx.msgMode == MSGMODE_TEXT_DISPLAYING) {
            MessageContext* msgCtx = &play->msgCtx;

            if (CFG_ON("text_any_button_skip") &&
                (CHECK_BTN_ALL(input->press.button, BTN_A) || CHECK_BTN_ALL(input->press.button, BTN_B) ||
                 (jumpText && gModJumpPressed)) &&
                (msgCtx->textBoxType != TEXTBOX_TYPE_4)) {
                // Only the box that's on screen now; the game's own "skip" flag would chain into the next boxes.
                msgCtx->textDrawPos = msgCtx->decodedTextLen;
                input->press.button &= ~(BTN_A | BTN_B);
            }
        } else if (jumpText && (gModJumpPressed || gModJumpHeld)) {
            if (gModJumpPressed) {
                input->press.button |= BTN_A;
            }
            input->cur.button |= BTN_A;
        }
    }

    // D-Pad Up: tap = minimap toggle (the game's own button, N64 L, is used for jumping),
    //           hold = open the Bombers' Notebook from anywhere.
    {
        static s32 sDupFrames = 0;
        static s32 sDupUsed = false;
        s32 held = CHECK_BTN_ALL(input->cur.button, BTN_DUP);
        s32 canNotebook = CFG_ON("notebook_hold") && CHECK_QUEST_ITEM(QUEST_BOMBERS_NOTEBOOK) &&
                          !IS_PAUSED(&play->pauseCtx) && (play->msgCtx.msgMode == MSGMODE_NONE) &&
                          !Play_InCsMode(play) && (play->transitionTrigger == TRANS_TRIGGER_OFF) &&
                          (gSaveContext.gameMode == GAMEMODE_NORMAL);

        if (!IS_PAUSED(&play->pauseCtx) && !TextBoxActive(play)) {
            if (held) {
                sDupFrames++;
                if ((sDupFrames == 10) && !sDupUsed && canNotebook) {
                    play->pauseCtx.bombersNotebookOpen = true;
                    Audio_PlaySfx(NA_SE_SY_DECIDE);
                    sDupUsed = true;
                }
            } else {
                if ((sDupFrames > 0) && !sDupUsed && CFG_ON("dpad_up_minimap")) {
                    input->press.button |= BTN_L;
                    input->cur.button |= BTN_L;
                }
                sDupFrames = 0;
                sDupUsed = false;
            }
            input->cur.button &= ~BTN_DUP;
            input->press.button &= ~BTN_DUP;
        }
    }

    // Title screen: the jump button works like Start / A.
    if ((gSaveContext.gameMode != GAMEMODE_NORMAL) && gModJumpPressed) {
        input->press.button |= BTN_START | BTN_A;
        input->cur.button |= BTN_START | BTN_A;
    }

    QuickSave_Update(play, input);

    if (Switcher_Update(play, input)) {
        ClearInput(input);
    } else {
        AutoShield_Update(play, input);
        ThirdPersonAim_Update(play, input);
    }

    // Tatl button (C-Up) only talks to Tatl: no first-person view. D-Pad Right also talks to Tatl.
    if ((gSaveContext.gameMode == GAMEMODE_NORMAL) && !IS_PAUSED(&play->pauseCtx) &&
        (play->msgCtx.msgMode == MSGMODE_NONE) && !Play_InCsMode(play)) {
        Player* player = GET_PLAYER(play);

        if (player != NULL) {
            Actor* focus = player->focusActor;
            s32 tatlHasSomething =
                (player->tatlTextId != 0) ||
                ((focus != NULL) &&
                 (CHECK_FLAG_ALL(focus->flags, ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_TALK_WITH_C_UP) ||
                  (focus->hintId != TATL_HINT_ID_NONE)));

            if (CFG_ON("dpad_right_tatl") && CHECK_BTN_ALL(input->press.button, BTN_DRIGHT)) {
                input->press.button |= BTN_CUP;
                input->cur.button |= BTN_CUP;
            }
            if (CFG_ON("dpad_right_tatl")) {
                input->press.button &= ~BTN_DRIGHT;
                input->cur.button &= ~BTN_DRIGHT;
            }
            if (CFG_ON("no_first_person") && !tatlHasSomething) {
                input->press.button &= ~BTN_CUP;
                input->cur.button &= ~BTN_CUP;
            }
        }
    }

    if (CFG_ON("switcher_enabled") && !IS_PAUSED(&play->pauseCtx)) {
        input->cur.button &= ~MP_MENU_BTN;
        input->press.button &= ~MP_MENU_BTN;
    }
}
