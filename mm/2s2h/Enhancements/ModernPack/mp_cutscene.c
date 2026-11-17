// Cutscene skip. During a cutscene a small prompt appears. Press the button once and the game's update
// runs many times per frame until the cutscene is over (plain text boxes are advanced automatically).
// Nothing is actually jumped over, so every story flag, item and song a cutscene gives still happens.
// It stops by itself if the game asks a question or wants you to play a song.
#include "mp_common.h"
#include "mp_ui.h"

static s32 sShown = false;
static f32 sFade = 0.0f;
static s32 sInExtraUpdate = false;
static s32 sCanFF = false;
static s32 sSkipping = false;
static s32 sIdleFrames = 0;
static s32 sPromptTimer = 0;
static s32 sAdvanceCalls = 0; // frames with no cutscene while skipping

static void UpdateFade(void) {
    f32 target = sSkipping ? 1.0f : 0.0f;

    if (sFade < target) {
        sFade += 0.34f; // ~3 frames to black
        if (sFade > target) sFade = target;
    } else if (sFade > target) {
        sFade -= 0.12f; // ~8 frames back to the game
        if (sFade < target) sFade = target;
    }
}

static s32 InCutscene(PlayState* play) {
    return (play->csCtx.state != CS_STATE_IDLE) || (CutsceneManager_GetCurrentCsId() != CS_ID_NONE);
}

// "Something is playing that the player isn't controlling": a cutscene, or any text box / conversation.
static s32 SceneBusy(PlayState* play) {
    return InCutscene(play) || (play->msgCtx.msgMode != MSGMODE_NONE) || Play_InCsMode(play);
}

static s32 SafeToRun(PlayState* play) {
    return play->state.running && (play->transitionTrigger == TRANS_TRIGGER_OFF) &&
           (play->transitionMode == TRANS_MODE_OFF) && !IS_PAUSED(&play->pauseCtx) &&
           (gSaveContext.gameMode == GAMEMODE_NORMAL);
}

// While skipping, area loads / screen fades also run at skip speed.
static s32 CanRunFast(PlayState* play) {
    return play->state.running && !IS_PAUSED(&play->pauseCtx) && (gSaveContext.gameMode == GAMEMODE_NORMAL);
}

// Only things the player genuinely has to do stop the skip: answering a question, entering a number,
// or playing / recording a song. Every other text box gets pushed through.
static s32 NeedsPlayer(PlayState* play) {
    MessageContext* msgCtx = &play->msgCtx;
    u8 mode = msgCtx->msgMode;
    u8 end = msgCtx->textboxEndType;

    if (mode == MSGMODE_NONE) {
        return false;
    }
    if ((mode == MSGMODE_TEXT_DONE) &&
        ((end == TEXTBOX_ENDTYPE_TWO_CHOICE) || (end == TEXTBOX_ENDTYPE_THREE_CHOICE) ||
         (end == TEXTBOX_ENDTYPE_12) || ((end >= TEXTBOX_ENDTYPE_INPUT_BANK) && (end <= TEXTBOX_ENDTYPE_64)))) {
        return true;
    }
    switch (mode) {
        case MSGMODE_OCARINA_STARTING:
        case MSGMODE_SONG_PROMPT_STARTING:
        case MSGMODE_OCARINA_PLAYING:
        case MSGMODE_SONG_PROMPT:
        case MSGMODE_OCARINA_AWAIT_INPUT:
        case MSGMODE_SCARECROW_LONG_RECORDING_START:
        case MSGMODE_SCARECROW_LONG_RECORDING_ONGOING:
        case MSGMODE_SCARECROW_SPAWN_RECORDING_START:
        case MSGMODE_SCARECROW_SPAWN_RECORDING_ONGOING:
            return true;
        default:
            return false;
    }
}

// Push a text box along: show all its text, then "press" A on alternating updates so each press
// counts as a new press.
void Message_DrawText(PlayState* play, Gfx** gfxP);

// The game only moves a text box forward (to "waiting for A" / next box) while drawing it, which happens
// once per real frame. While skipping, run that step into a throwaway display list on every update.
static Gfx sScratchGfx[4096];

static void StepTextNow(PlayState* play) {
    Gfx* gfx = sScratchGfx;
    Message_DrawText(play, &gfx);
}

// Drive the text system forward directly, doing exactly what the game does when you press A,
// so it doesn't depend on button presses being noticed.
static void AdvanceText(PlayState* play, Input* input, s32 tick) {
    MessageContext* msgCtx = &play->msgCtx;
    u8 mode = msgCtx->msgMode;
    u8 end = msgCtx->textboxEndType;

    if ((mode == MSGMODE_NONE) || NeedsPlayer(play)) {
        return;
    }
    sAdvanceCalls++;

    switch (mode) {
        case MSGMODE_TEXT_DISPLAYING:
            msgCtx->textboxSkipped = true;
            msgCtx->textUnskippable = false;
            msgCtx->textDrawPos = msgCtx->decodedTextLen;
            StepTextNow(play);
            break;

        case MSGMODE_TEXT_DELAYED_BREAK:
            msgCtx->stateTimer = 1;
            break;

        case MSGMODE_TEXT_AWAIT_NEXT:
            msgCtx->msgMode = MSGMODE_TEXT_NEXT_MSG;
            msgCtx->msgBufPos++;
            break;

        case MSGMODE_TEXT_DONE:
            if ((end == TEXTBOX_ENDTYPE_EVENT) || (end == TEXTBOX_ENDTYPE_EVENT2) ||
                (end == TEXTBOX_ENDTYPE_PERSISTENT) || (end == TEXTBOX_ENDTYPE_PAUSE_MENU)) {
                // Closed by the character / cutscene; give it the "A pressed" it may be waiting for.
                if ((tick & 1) == 0) {
                    input->press.button |= BTN_A;
                    input->cur.button |= BTN_A;
                }
            } else if ((end == TEXTBOX_ENDTYPE_FADE_NORMAL) || (end == TEXTBOX_ENDTYPE_FADE_SKIPPABLE) ||
                       (end == TEXTBOX_ENDTYPE_FADE_STAGES_1) || (end == TEXTBOX_ENDTYPE_FADE_STAGES_2) ||
                       (end == TEXTBOX_ENDTYPE_FADE_STAGES_3)) {
                msgCtx->stateTimer = 1;
                msgCtx->textColorAlpha = 0;
                if ((end == TEXTBOX_ENDTYPE_FADE_STAGES_1) || (end == TEXTBOX_ENDTYPE_FADE_STAGES_2)) {
                    msgCtx->textboxEndType = TEXTBOX_ENDTYPE_FADE_STAGES_3;
                }
            } else if (msgCtx->nextTextId != 0xFFFF) {
                Message_ContinueTextbox(play, msgCtx->nextTextId);
            } else if (msgCtx->bombersNotebookEventQueueCount == 0) {
                Message_CloseTextbox(play);
            } else if ((tick & 1) == 0) {
                input->press.button |= BTN_A;
                input->cur.button |= BTN_A;
            }
            break;

        default:
            // Box opening / closing animations etc.: also nudge with A.
            if ((tick & 1) == 0) {
                input->press.button |= BTN_A;
                input->cur.button |= BTN_A;
            }
            break;
    }
}

static s32 sAdvanceTick = 0;
static s32 sStallFrames = 0;
static u16 sLastCsFrame = 0xFFFF;
static u8 sLastMsgMode = 0xFF;
static u16 sLastTextId = 0xFFFF;


void CutsceneFF_UpdatePrompt(PlayState* play, Input* input) {
    s32 inCs;
    s32 show;
    s32 state;

    if (sInExtraUpdate) {
        return;
    }
    inCs = CFG_ON("cutscene_ff") && (InCutscene(play) || Play_InCsMode(play)) && SafeToRun(play);

    if (inCs && !sSkipping) {
        // Triangle (C-Up): ask. Start: confirm.
        if (CHECK_BTN_ALL(input->press.button, BTN_CUP) && !NeedsPlayer(play)) {
            sPromptTimer = 80; // ~4 seconds
        } else if ((sPromptTimer > 0) && CHECK_BTN_ALL(input->press.button, BTN_START) && !NeedsPlayer(play)) {
            sSkipping = true;
            sIdleFrames = 0;
            sPromptTimer = 0;
            Audio_PlaySfx(NA_SE_SY_DECIDE);
        }
        input->press.button &= ~(BTN_CUP | BTN_START);
        input->cur.button &= ~(BTN_CUP | BTN_START);
    }
    if (sPromptTimer > 0) {
        sPromptTimer--;
        if (!inCs) {
            sPromptTimer = 0;
        }
    }

    // Once skipping, keep going through area loads and back-to-back cutscenes until Link is
    // playable again (no cutscene for about half a second).
    if (sSkipping) {
        if (!CFG_ON("cutscene_ff")) {
            sSkipping = false;
        } else if (SceneBusy(play) || !SafeToRun(play)) {
            sIdleFrames = 0;
        } else if (++sIdleFrames > 10) {
            sSkipping = false;
        }
    }
    if (sSkipping && NeedsPlayer(play)) {
        sSkipping = false; // hand control back for questions / songs
    }
    // Watchdog: if nothing moves for ~2 seconds, stop skipping and give control back.
    if (sSkipping) {
        u16 csFrame = play->csCtx.curFrame;
        u8 mode = play->msgCtx.msgMode;
        u16 textId = play->msgCtx.currentTextId;

        if ((csFrame == sLastCsFrame) && (mode == sLastMsgMode) && (textId == sLastTextId)) {
            if (++sStallFrames > 160) { // ~8 seconds
                sSkipping = false;
                sStallFrames = 0;
            }
        } else {
            sStallFrames = 0;
        }
        sLastCsFrame = csFrame;
        sLastMsgMode = mode;
        sLastTextId = textId;
    } else {
        sStallFrames = 0;
    }

    sCanFF = sSkipping && CanRunFast(play);
    if (sSkipping) {
        AdvanceText(play, input, sAdvanceTick++);
        // Swallow Start so the game doesn't pause mid-skip.
        input->press.button &= ~BTN_START;
        input->cur.button &= ~BTN_START;
    }

    UpdateFade();

    sShown = (sPromptTimer > 0) && !sSkipping;
}

// Drawn with the HUD: the "Skip?" prompt (bottom right) and the black screen while skipping.
void CutsceneFF_Draw(PlayState* play, Gfx** gfxP) {
    if (sFade > 0.0f) {
        u8 a = (u8)(sFade * 255.0f);
        Ui_Align(gfxP, UI_ALIGN_STRETCH);
        Ui_Rect(gfxP, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, a);
        Ui_Align(gfxP, UI_ALIGN_CENTER);
        Ui_Text(gfxP, SCREEN_WIDTH / 2.0f, 112.0f, "Skipping...", 0.8f, 1, 255, 255, 255, (u8)((a * 140) / 255));
    }
    if (sShown) {
        Ui_Align(gfxP, UI_ALIGN_RIGHT);
        Ui_ButtonPrompt(gfxP, SCREEN_WIDTH - 16.0f, 206.0f, Mod_Label(MOD_LABEL_START), "Skip", 0.62f, 230);
        Ui_Text(gfxP, SCREEN_WIDTH - 16.0f, 192.0f, "Skip cutscene?", 0.6f, 2, 245, 243, 232, 230);
        Ui_Align(gfxP, UI_ALIGN_CENTER);
    }
}

static PlayState* sHookPlay = NULL;

void CutsceneFF_BeforeUpdate(PlayState* play) {
    if (!sInExtraUpdate) {
        sHookPlay = play;
    }
}

void CutsceneFF_AfterUpdate(void) {
    PlayState* play = sHookPlay;
    s32 i;
    s32 extra;

    if (play == NULL) {
        return;
    }

    if (sInExtraUpdate || !sCanFF) {
        return;
    }
    extra = (s32)CFG_NUM("skip_rate") - 1;
    sInExtraUpdate = true;
    for (i = 0; i < extra; i++) {
        Input* input;
        u16 savedPress;

        if (!CanRunFast(play) || NeedsPlayer(play)) {
            break;
        }
        // Area changes (and their fades) run at normal speed, so scene setup and story events at the start of a
        // new area happen exactly like they normally do.
        if ((play->transitionTrigger != TRANS_TRIGGER_OFF) || (play->transitionMode != TRANS_MODE_OFF)) {
            break;
        }
        // Mirror what Play_Main does between updates.
        CutsceneManager_Update();
        CutsceneManager_ClearWaiting();
        // Don't repeat button presses in the extra updates.
        input = CONTROLLER1(&play->state);
        savedPress = input->press.button;
        input->press.button = 0;
        // Advance plain text boxes: B shows the rest of the text, A moves on.
        AdvanceText(play, input, i);
        {
            // Don't let the clock run hundreds of times faster while skipping: undo the normal per-update tick
            // (cutscenes that deliberately set the time make bigger jumps, and those are kept).
            u16 t0 = gSaveContext.save.time;
            s32 d0 = gSaveContext.save.day;
            u16 dt;

            Play_Update(play);
            dt = (u16)(gSaveContext.save.time - t0);
            if ((gSaveContext.save.day == d0) && (dt > 0) && (dt <= 12)) {
                gSaveContext.save.time = t0;
            }
        }
        input->press.button = savedPress;
    }
    sInExtraUpdate = false;
}
