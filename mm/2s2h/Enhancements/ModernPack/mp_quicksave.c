// Quick Save: hold D-Pad Down for about a second to save right where you are.
// Uses Zelda64Recomp's own autosave, so loading the file puts you back at that spot.
#include "mp_common.h"
#include "mp_ui.h"

#define SavePos_CanSaveHere(play) MP_CanSaveHere(play)

static s32 sHoldFrames = 0;
static s32 sUsed = false;
static s32 sMsgTimer = 0;
static s32 sMsgOk = false;

static s32 CanSave(PlayState* play) {
    return (gSaveContext.gameMode == GAMEMODE_NORMAL) && SavePos_CanSaveHere(play) && (play->msgCtx.msgMode == MSGMODE_NONE) &&
           (play->pauseCtx.state == PAUSE_STATE_OFF) && !Play_InCsMode(play) &&
           (play->transitionTrigger == TRANS_TRIGGER_OFF) && (gSaveContext.save.cutsceneIndex < 0xFFF0) &&
           (gSaveContext.minigameStatus == MINIGAME_STATUS_END) &&
           (gSaveContext.timerStates[TIMER_ID_MINIGAME_1] == TIMER_STATE_OFF) &&
           (gSaveContext.timerStates[TIMER_ID_MINIGAME_2] == TIMER_STATE_OFF) && (GET_PLAYER(play) != NULL) &&
           !(GET_PLAYER(play)->stateFlags1 & PLAYER_STATE1_DEAD);
}

void QuickSave_Update(PlayState* play, Input* input) {
    if (sMsgTimer > 0) {
        sMsgTimer--;
    }
    if (!CFG_ON("quick_save")) {
        return;
    }
    if (CHECK_BTN_ALL(input->cur.button, BTN_DDOWN) && !IS_PAUSED(&play->pauseCtx)) {
        sHoldFrames++;
        if ((sHoldFrames >= 20) && !sUsed) {
            sUsed = true;
            if (CanSave(play)) {
                sMsgOk = MP_DoSave(play);
                Audio_PlaySfx(sMsgOk ? NA_SE_SY_DECIDE : NA_SE_SY_ERROR);
            } else {
                Audio_PlaySfx(NA_SE_SY_ERROR);
                sMsgOk = false;
            }
            sMsgTimer = 50;
        }
    } else {
        sHoldFrames = 0;
        sUsed = false;
    }
    if (!IS_PAUSED(&play->pauseCtx)) {
        input->cur.button &= ~BTN_DDOWN;
        input->press.button &= ~BTN_DDOWN;
    }
}

// Pause menu: Square (N64 B) saves the game right where you are.
static s32 PauseIdle(PlayState* play) {
    PauseContext* pauseCtx = &play->pauseCtx;
    return (pauseCtx->state == PAUSE_STATE_MAIN) && (pauseCtx->mainState == PAUSE_MAIN_STATE_IDLE) &&
           !pauseCtx->itemDescriptionOn && (gSaveContext.gameMode == GAMEMODE_NORMAL);
}

static s32 sQuitConfirmTimer = 0;

static s32 SaveAllowed(void) {
    return (gSaveContext.save.cutsceneIndex < 0xFFF0) && (gSaveContext.minigameStatus == MINIGAME_STATUS_END) &&
           (gSaveContext.timerStates[TIMER_ID_MINIGAME_1] == TIMER_STATE_OFF) &&
           (gSaveContext.timerStates[TIMER_ID_MINIGAME_2] == TIMER_STATE_OFF);
}

void PauseSave_Update(PlayState* play, Input* input) {
    if (sQuitConfirmTimer > 0) {
        sQuitConfirmTimer--;
    }
    if (!PauseIdle(play)) {
        sQuitConfirmTimer = 0;
        return;
    }

    // Triangle (C-Up): return to the title screen. Press twice to confirm; saves first.
    if (CFG_ON("pause_quit") && CHECK_BTN_ALL(input->press.button, BTN_CUP)) {
        input->press.button &= ~BTN_CUP;
        input->cur.button &= ~BTN_CUP;
        if (sQuitConfirmTimer == 0) {
            sQuitConfirmTimer = 60;
            Audio_PlaySfx(NA_SE_SY_DECIDE);
        } else {
            sQuitConfirmTimer = 0;
            if (SaveAllowed() && SavePos_CanSaveHere(play)) {
                MP_DoSave(play);
            }
            Audio_PlaySfx(NA_SE_SY_PIECE_OF_HEART);
            // The game's own "fade to black, then go to the title screen" step.
            play->pauseCtx.state = PAUSE_STATE_SAVEPROMPT;
            play->pauseCtx.savePromptState = PAUSE_SAVEPROMPT_STATE_6;
        }
        return;
    }

    if (!CFG_ON("pause_save")) {
        return;
    }
    if (CHECK_BTN_ALL(input->press.button, BTN_B)) {
        input->press.button &= ~BTN_B;
        input->cur.button &= ~BTN_B;
        if (SaveAllowed() && SavePos_CanSaveHere(play) && MP_DoSave(play)) {
            Audio_PlaySfx(NA_SE_SY_PIECE_OF_HEART);
            sMsgOk = true;
        } else {
            Audio_PlaySfx(NA_SE_SY_ERROR);
            sMsgOk = false;
        }
        sMsgTimer = 50;
    } else {
        input->cur.button &= ~BTN_B;
    }
}

void QuickSave_Draw(PlayState* play, Gfx** gfxP) {
    u8 a;

    if (CFG_ON("pause_save") && PauseIdle(play) && (sMsgTimer <= 0)) {
        // Small prompt in the bottom-right corner, out of the way of the pause menu itself.
        Ui_Align(gfxP, UI_ALIGN_RIGHT);
        if (CFG_ON("pause_quit")) {
            Ui_ButtonPrompt(gfxP, SCREEN_WIDTH - 16.0f, 200.0f, Mod_Label(MOD_LABEL_SKIP),
                            (sQuitConfirmTimer > 0) ? "Press again to quit" : "Title Screen", 0.62f, 230);
        }
        Ui_ButtonPrompt(gfxP, SCREEN_WIDTH - 16.0f, 220.0f, Mod_Label(MOD_LABEL_SORT), "Save", 0.62f, 230);
        Ui_Align(gfxP, UI_ALIGN_CENTER);
    }
    if (sMsgTimer <= 0) {
        return;
    }
    a = (sMsgTimer > 10) ? 255 : (u8)(sMsgTimer * 25);
    if (IS_PAUSED(&play->pauseCtx)) {
        Ui_Align(gfxP, UI_ALIGN_RIGHT);
        Ui_Text(gfxP, SCREEN_WIDTH - 16.0f, 218.0f, sMsgOk ? "Game Saved" : "Can't save right now", 0.62f, 2, 255,
                255, sMsgOk ? 255 : 120, a);
    } else {
        Ui_Align(gfxP, UI_ALIGN_CENTER);
        Ui_Text(gfxP, SCREEN_WIDTH / 2.0f, 196, sMsgOk ? "Game Saved" : "Can't save right now", 0.75f, 1, 255, 255,
                sMsgOk ? 255 : 120, a);
    }
    Ui_Align(gfxP, UI_ALIGN_CENTER);
}
