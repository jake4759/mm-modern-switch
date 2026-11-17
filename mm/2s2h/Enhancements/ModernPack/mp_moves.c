// Modern moves: manual jump (any form, Deku Scrub included), sprint, faster swim and climb, third-person aim.
#include "mp_common.h"
#include "assets/objects/gameplay_keep/gameplay_keep.h"

void func_80834D50(PlayState* play, Player* this, PlayerAnimationHeader* anim, f32 speed, u16 sfxId);
void Player_Action_26(Player* this, PlayState* play); // rolling

static s32 sSprintActive = false;

static s32 FormCanJump(Player* this) {
    // Goron Link is too heavy (and rolls on A); every other form jumps.
    return (this->transformation == PLAYER_FORM_HUMAN) || (this->transformation == PLAYER_FORM_ZORA) ||
           (this->transformation == PLAYER_FORM_FIERCE_DEITY) || (this->transformation == PLAYER_FORM_DEKU);
}

// Called from Player_TryActionHandlerList when Link is in a free-movement state. Returns true if a jump started.
s32 ModernPack_TryJump(Player* this, PlayState* play) {
    PlayerAnimationHeader* anim;
    f32 height;

    if (!CFG_ON("jump_enabled") || !gModJumpPressed) {
        return false;
    }
    if (!FormCanJump(this) || !(this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
        return false;
    }
    // Not while swimming, carrying something, or in other special states.
    if (this->stateFlags1 & (PLAYER_STATE1_8000000 | PLAYER_STATE1_CARRYING_ACTOR | PLAYER_STATE1_800000 |
                             PLAYER_STATE1_400000)) {
        return false;
    }

    anim = (PlayerAnimationHeader*)((this->speedXZ > 4.0f) ? gPlayerAnim_link_normal_run_jump
                                                            : gPlayerAnim_link_normal_jump);
    height = CFG_NUM("jump_height");
    if (this->transformation == PLAYER_FORM_DEKU) {
        height *= 0.85f; // little legs
    }
    gModJumpPressed = false; // consume
    // Same routine the game uses for its own automatic ledge jumps. Ledge grabbing mid-air still works.
    func_80834D50(play, this, anim, height, NA_SE_VO_LI_SWORD_N);
    this->av2.actionVar2 = 1; // landing animation
    return true;
}

// Sprint: hold A through a roll, keep holding A and the stick. Stops when you let go.
void ModernPack_UpdateSprint(Player* this, PlayState* play, Input* input) {
    s32 aHeld;
    s32 stickHeld;

    if (!CFG_ON("sprint_enabled") || (input == NULL) || (this != GET_PLAYER(play))) {
        sSprintActive = false;
        return;
    }

    aHeld = CHECK_BTN_ALL(input->cur.button, BTN_A);
    stickHeld = (SQ((s32)input->rel.stick_x) + SQ((s32)input->rel.stick_y)) > SQ(20);

    if ((this->actionFunc == Player_Action_26) && aHeld &&
        ((this->transformation == PLAYER_FORM_HUMAN) || (this->transformation == PLAYER_FORM_ZORA) ||
         (this->transformation == PLAYER_FORM_FIERCE_DEITY))) {
        sSprintActive = true;
    }

    if (!aHeld || !stickHeld || (this->stateFlags1 & (PLAYER_STATE1_8000000 | PLAYER_STATE1_800000)) ||
        (this->transformation == PLAYER_FORM_GORON) || (this->transformation == PLAYER_FORM_DEKU) ||
        (this->focusActor != NULL)) {
        sSprintActive = false;
    }
}

// Multiplier for the run speed target (1.0 = unchanged).
f32 ModernPack_SpeedScale(Player* this, s32 linearMode) {
    f32 scale = 1.0f;

    if (sSprintActive && !linearMode && (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
        scale *= CFG_NUM("sprint_speed");
    }
    // Faster swimming for human Link while holding the jump button (Zora Link is untouched).
    if (CFG_ON("fast_swim") && (this->stateFlags1 & PLAYER_STATE1_8000000) &&
        (this->transformation == PLAYER_FORM_HUMAN) && gModJumpHeld) {
        scale *= CFG_NUM("swim_speed");
    }
    return scale;
}

// Climbing a ladder or vines with A held runs the climbing step twice per frame.
s32 ModernPack_FastClimb(Player* this, Input* input) {
    return CFG_ON("fast_climb") && (input != NULL) && CHECK_BTN_ALL(input->cur.button, BTN_A) &&
           ((input->rel.stick_y > 20) || (input->rel.stick_y < -20) || (input->rel.stick_x > 20) ||
            (input->rel.stick_x < -20));
}

s32 ModernPack_ThirdPersonAimOn(void) {
    return CFG_ON("third_person_aim");
}

static s32 IsRanged(u8 item) {
    return (item == ITEM_BOW) || (item == ITEM_BOW_FIRE) || (item == ITEM_BOW_ICE) || (item == ITEM_BOW_LIGHT) ||
           (item == ITEM_HOOKSHOT);
}

// Holding a ranged item's button also holds Z: the game's own over-the-shoulder aim.
void ThirdPersonAim_Update(PlayState* play, Input* input) {
    static const u16 sCButtons[3] = { BTN_CLEFT, BTN_CDOWN, BTN_CRIGHT };
    s32 i;

    if (!CFG_ON("third_person_aim") || (gSaveContext.gameMode != GAMEMODE_NORMAL) || IS_PAUSED(&play->pauseCtx) ||
        (play->msgCtx.msgMode != MSGMODE_NONE) || Play_InCsMode(play)) {
        return;
    }
    if (GET_PLAYER(play) == NULL || GET_PLAYER(play)->transformation != PLAYER_FORM_HUMAN) {
        return;
    }
    for (i = 0; i < 3; i++) {
        if (CHECK_BTN_ALL(input->cur.button, sCButtons[i]) && IsRanged(BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_C_LEFT + i))) {
            input->cur.button |= BTN_Z;
            if (CHECK_BTN_ALL(input->press.button, sCButtons[i])) {
                input->press.button |= BTN_Z;
            }
            return;
        }
    }
}

// Auto-shield (OoT remake): locked on to an enemy and standing still raises the shield.
void AutoShield_Update(PlayState* play, Input* input) {
    static s32 sWasInjecting = false;
    Player* player = GET_PLAYER(play);
    s32 inject = false;

    if (CFG_ON("auto_shield") && (player != NULL) && !IS_PAUSED(&play->pauseCtx) &&
        (play->msgCtx.msgMode == MSGMODE_NONE) &&
        ((player->transformation == PLAYER_FORM_HUMAN) || (player->transformation == PLAYER_FORM_FIERCE_DEITY)) &&
        Player_CheckHostileLockOn(player) && (player->actor.bgCheckFlags & BGCHECKFLAG_GROUND)) {
        s32 stickMag = SQ((s32)input->rel.stick_x) + SQ((s32)input->rel.stick_y);
        u16 busy = BTN_A | BTN_B | BTN_CLEFT | BTN_CDOWN | BTN_CRIGHT | BTN_CUP;

        if ((stickMag < SQ(15)) && !CHECK_BTN_ANY(input->cur.button, busy)) {
            inject = true;
        }
    }

    if (inject) {
        input->cur.button |= BTN_R;
        if (!sWasInjecting) {
            input->press.button |= BTN_R;
        }
    }
    sWasInjecting = inject;
}

// D-Pad Left = Ocarina (the remake moved it to the D-pad). Runs right after the game's own item buttons.
void Player_UseItem(PlayState* play, Player* this, ItemId item);

void ModernPack_AfterItemButtons(Player* this, PlayState* play, Input* input) {
    if (!CFG_ON("dpad_items") || (this != GET_PLAYER(play)) || (input == NULL)) {
        return;
    }
    if ((this->stateFlags1 & (PLAYER_STATE1_CARRYING_ACTOR | PLAYER_STATE1_20000000)) ||
        (this->stateFlags2 & PLAYER_STATE2_2000000) || (this->stateFlags3 & PLAYER_STATE3_20000000) ||
        func_801240DC(this)) {
        return;
    }
    if ((this->itemAction != this->heldItemAction) || (this->unk_AA5 != PLAYER_UNKAA5_0)) {
        return;
    }
    if (CHECK_BTN_ALL(input->press.button, BTN_DLEFT) && (INV_CONTENT(ITEM_OCARINA_OF_TIME) == ITEM_OCARINA_OF_TIME)) {
        Player_UseItem(play, this, ITEM_OCARINA_OF_TIME);
    }
}
