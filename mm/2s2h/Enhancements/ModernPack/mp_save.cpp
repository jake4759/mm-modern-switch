// Save anywhere for the MM Modern pack: writes the same kind of save 2Ship's own autosave / pause save makes,
// so loading the file puts you back where you saved (with "Remember Save Location" on).
#include "BenPort.h"
#include "2s2h/Enhancements/Saving/SavingEnhancements.h"

extern "C" {
#include <variables.h>
#include <functions.h>
#include "mp_common.h"
}

extern "C" s32 MP_CanSaveHere(PlayState* play) {
    return SavingEnhancements_CanSave() && (play->pauseCtx.state == PAUSE_STATE_OFF || IS_PAUSED(&play->pauseCtx));
}

extern "C" s32 MP_DoSave(PlayState* play) {
    if (!SavingEnhancements_CanSave()) {
        return false;
    }
    bool currentOwlSaveState = gSaveContext.save.isOwlSave;

    gSaveContext.save.isOwlSave = true;
    SavingEnhancements_PersistSaveEntranceInfo();
    SavingEnhancements_AdvancePlaytime();
    Play_SaveCycleSceneFlags(play);
    gSaveContext.save.saveInfo.playerData.savedSceneId = play->sceneId;
    func_8014546C(&play->sramCtx);
    Sram_SetFlashPagesOwlSave(&play->sramCtx, gFlashOwlSaveStartPages[gSaveContext.fileNum * FLASH_SAVE_MAIN_MULTIPLIER],
                              gFlashOwlSaveNumPages[gSaveContext.fileNum * FLASH_SAVE_MAIN_MULTIPLIER]);
    Sram_StartWriteToFlashOwlSave(&play->sramCtx);
    gSaveContext.save.isOwlSave = currentOwlSaveState;
    SavingEnhancements_ClearSaveEntranceInfo();
    return true;
}
