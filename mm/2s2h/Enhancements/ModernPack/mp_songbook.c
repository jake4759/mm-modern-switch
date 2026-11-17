// Ocarina songbook (OoT remake style).
// While playing the ocarina freely: a prompt bar shows the controls. Press the songbook button to open a
// panel with your learned songs drawn on a staff. Stick left/right picks a song, A plays it back so you can
// hear it, B (or the songbook button) closes it.
// "Listen" plays notes straight on the ocarina sound channel. It never goes through the game's song
// detector, so it can't accidentally trigger a song (e.g. reset time with the Song of Time).
#include "mp_common.h"
#include "mp_ui.h"
#include "audiothread_cmd.h"

#define MAX_NOTES 8
#define SFX_CHANNEL_OCARINA 0xD

typedef struct {
    s8 x;
    s8 y;
} MP_OcarinaControlStick;
#define STAFF_W 620.0f
#define STAFF_H 190.0f
#define NOTE_SIZE 44.0f
#define NOTE_FRAMES 9   // game frames per note during playback (20 fps game logic)
#define NOTE_GAP 2

extern OcarinaSongButtons gOcarinaSongButtons[];
extern s8 sOcarinaInstrumentId;
extern u8 sButtonToPitchMap[5];
extern u32 sOcarinaInputButtonCur;
extern MP_OcarinaControlStick sOcarinaInputStickRel;
extern Vec3f gSfxDefaultPos;
extern f32 gSfxDefaultFreqAndVolScale;
extern s8 gSfxDefaultReverb;

typedef struct {
    u8 song;        // OCARINA_SONG_*
    u8 questItem;   // QUEST_SONG_*, or 0xFF = needs Song of Time
    const char* name;
    const char* desc;
} SongInfo;

static const SongInfo sSongs[] = {
    { OCARINA_SONG_TIME, QUEST_SONG_TIME, "Song of Time", "Return to the dawn of the first day." },
    { OCARINA_SONG_INVERTED_TIME, 0xFF, "Inverted Song of Time", "Slows the flow of time." },
    { OCARINA_SONG_DOUBLE_TIME, 0xFF, "Song of Double Time", "Skip ahead to the next morning or night." },
    { OCARINA_SONG_HEALING, QUEST_SONG_HEALING, "Song of Healing", "Heals troubled spirits and turns them into masks." },
    { OCARINA_SONG_EPONAS, QUEST_SONG_EPONA, "Epona's Song", "Calls Epona to your side." },
    { OCARINA_SONG_SOARING, QUEST_SONG_SOARING, "Song of Soaring", "Warp to an Owl Statue you've activated." },
    { OCARINA_SONG_STORMS, QUEST_SONG_STORMS, "Song of Storms", "Calls up a rainstorm." },
    { OCARINA_SONG_SONATA, QUEST_SONG_SONATA, "Sonata of Awakening", "Awakens those who are asleep." },
    { OCARINA_SONG_GORON_LULLABY, QUEST_SONG_LULLABY, "Goron Lullaby", "Lulls listeners to sleep." },
    { OCARINA_SONG_NEW_WAVE, QUEST_SONG_BOSSA_NOVA, "New Wave Bossa Nova", "A song from the heart of a Zora." },
    { OCARINA_SONG_ELEGY, QUEST_SONG_ELEGY, "Elegy of Emptiness", "Creates a shell of your current form." },
    { OCARINA_SONG_OATH, QUEST_SONG_OATH, "Oath to Order", "Summons the giants." },
};

static s32 sAvailable[ARRAY_COUNT(sSongs)];
static s32 sNumAvailable = 0;
static s32 sSelected = 0;
static s32 sPromptShown = false;
static s32 sBookOpen = false;
static s32 sNavTimer = 0;

// Playback state
static s32 sPlaying = false;
static s32 sPlaySong = 0;
static s32 sPlayNoteIdx = 0;
static s32 sPlayTimer = 0;
static f32 sNoteFreq = 1.0f;
static f32 sNoteVolume = 87.0f / 127.0f;

// Y position (top of note circle) for each ocarina button on the staff. Lines are at 40/70/100/130/160.
static const f32 sNoteY[5] = {
    175.0f - NOTE_SIZE / 2, // A      = D4 (below the bottom line)
    145.0f - NOTE_SIZE / 2, // C-Down = F4
    115.0f - NOTE_SIZE / 2, // C-Right= A4
    100.0f - NOTE_SIZE / 2, // C-Left = B4
    70.0f - NOTE_SIZE / 2,  // C-Up   = D5
};
static const char* sNoteText[5] = { "A", "v", ">", "<", "^" };

s32 Songbook_IsBlockingOcarina(void) {
    return sBookOpen;
}

static void BuildAvailable(void) {
    s32 i;

    sNumAvailable = 0;
    for (i = 0; i < (s32)ARRAY_COUNT(sSongs); i++) {
        u8 q = (sSongs[i].questItem == 0xFF) ? QUEST_SONG_TIME : sSongs[i].questItem;
        if (CHECK_QUEST_ITEM(q)) {
            sAvailable[sNumAvailable++] = i;
        }
    }
}

static void RefreshBook(void) {
}

static void StopPlayback(void) {
    if (sPlaying) {
        AudioSfx_StopById(NA_SE_OC_OCARINA);
        sPlaying = false;
    }
}

static void PlayNote(u8 button) {
    u8 instrument = (sOcarinaInstrumentId != 0) ? sOcarinaInstrumentId : 1;

    AUDIOCMD_CHANNEL_SET_IO(SEQ_PLAYER_SFX, SFX_CHANNEL_OCARINA, 7, instrument - 1);
    AUDIOCMD_CHANNEL_SET_IO(SEQ_PLAYER_SFX, SFX_CHANNEL_OCARINA, 5, sButtonToPitchMap[button]);
    AUDIOCMD_CHANNEL_SET_IO(SEQ_PLAYER_SFX, SFX_CHANNEL_OCARINA, 6, 0);
    AudioSfx_PlaySfx(NA_SE_OC_OCARINA, &gSfxDefaultPos, 4, &sNoteFreq, &sNoteVolume, &gSfxDefaultReverb);
}

static void UpdatePlayback(void) {
    OcarinaSongButtons* btns;

    if (!sPlaying) {
        return;
    }
    btns = &gOcarinaSongButtons[sSongs[sPlaySong].song];
    sPlayTimer++;
    if (sPlayTimer == NOTE_FRAMES) {
        AudioSfx_StopById(NA_SE_OC_OCARINA);
    } else if (sPlayTimer >= NOTE_FRAMES + NOTE_GAP) {
        sPlayNoteIdx++;
        sPlayTimer = 0;
        if ((sPlayNoteIdx >= btns->numButtons) || (sPlayNoteIdx >= MAX_NOTES)) {
            sPlaying = false;
        } else {
            PlayNote(btns->buttonIndex[sPlayNoteIdx]);
        }
        RefreshBook();
    }
}

static void StartPlayback(void) {
    OcarinaSongButtons* btns = &gOcarinaSongButtons[sSongs[sAvailable[sSelected]].song];

    StopPlayback();
    if (btns->numButtons == 0) {
        return;
    }
    sPlaying = true;
    sPlaySong = sAvailable[sSelected];
    sPlayNoteIdx = 0;
    sPlayTimer = 0;
    PlayNote(btns->buttonIndex[0]);
    RefreshBook();
}

static void SetPrompt(s32 show, u16 bookBtn) {
    sPromptShown = show;
}

static void CloseBook(void) {
    if (sBookOpen) {
        StopPlayback();
        sBookOpen = false;
    }
}

static s32 InFreePlay(PlayState* play) {
    MessageContext* msgCtx = &play->msgCtx;

    return (msgCtx->msgMode == MSGMODE_OCARINA_PLAYING) &&
           ((msgCtx->ocarinaAction == OCARINA_ACTION_FREE_PLAY) ||
            (msgCtx->ocarinaAction == OCARINA_ACTION_CHECK_NOTIME)) &&
           !IS_PAUSED(&play->pauseCtx);
}

s32 Songbook_Update(PlayState* play, Input* input) {
    u16 bookBtn;
    s32 nav;

    if (!CFG_ON("songbook_enabled") || !InFreePlay(play)) {
        CloseBook();
        SetPrompt(false, 0);
        return false;
    }
    bookBtn = BTN_Z;

    if (!sBookOpen) {
        SetPrompt(true, bookBtn);
        if (CHECK_BTN_ALL(input->press.button, bookBtn)) {
            BuildAvailable();
            if (sSelected >= sNumAvailable) {
                sSelected = 0;
            }
            sNavTimer = 0;
            SetPrompt(false, 0);
            RefreshBook();
            sBookOpen = true;
            Audio_PlaySfx(NA_SE_SY_WIN_OPEN);
            return true;
        }
        return false;
    }

    // Book is open: we own the controller.
    if (CHECK_BTN_ALL(input->press.button, BTN_B) || CHECK_BTN_ALL(input->press.button, bookBtn) ||
        gModJumpPressed) {
        CloseBook();
        Audio_PlaySfx(NA_SE_SY_WIN_CLOSE);
        return true;
    }

    nav = Mod_StickNavX(input, &sNavTimer, false);
    if ((nav != 0) && (sNumAvailable > 0)) {
        StopPlayback();
        sSelected = (sSelected + nav + sNumAvailable) % sNumAvailable;
        Audio_PlaySfx(NA_SE_SY_CURSOR);
        RefreshBook();
    }

    if (CHECK_BTN_ALL(input->press.button, BTN_A) && (sNumAvailable > 0)) {
        StartPlayback();
    }

    UpdatePlayback();
    return true;
}

// Keep the ocarina from hearing button presses while the songbook is open.
void Songbook_AfterOcarinaRead(void) {
    if (sBookOpen) {
        sOcarinaInputButtonCur = 0;
        sOcarinaInputStickRel.x = 0;
        sOcarinaInputStickRel.y = 0;
    }
}

// ---- Drawing (parchment panel with the song on a staff, like the remake's songbook) ----
void Songbook_Draw(PlayState* play, Gfx** gfxP) {
    f32 cx = SCREEN_WIDTH / 2.0f;

    if (sPromptShown && !sBookOpen) {
        f32 w;
        const char* text = "Songbook";
        Ui_Align(gfxP, UI_ALIGN_CENTER);
        w = Ui_TextWidth(text, 0.6f) + 30.0f;
        Ui_Tex(gfxP, gUiPanelTex, false, 64, 32, cx - w / 2, 14.0f, w, 15.0f, 0, 0, 0, 150);
        Ui_KeyCap(gfxP, cx - w / 2 + 11.0f, 17.0f, "ZL", 0.55f, 255);
        Ui_Text(gfxP, cx - w / 2 + 21.0f, 16.0f, text, 0.6f, 0, 245, 243, 232, 255);
    }
    if (!sBookOpen) {
        return;
    }
    {
        f32 px = 44.0f, py = 52.0f, pw = SCREEN_WIDTH - 88.0f, ph = 140.0f;
        f32 staffX = px + 18.0f, staffW = pw - 36.0f, staffTop = py + 30.0f;
        f32 k = 0.32f;
        s32 i;

        Ui_Align(gfxP, UI_ALIGN_CENTER);
        Ui_RoundRect(gfxP, px - 2, py - 2, pw + 4, ph + 4, 5.0f, 120, 80, 40, 255);
        Ui_RoundRect(gfxP, px, py, pw, ph, 4.0f, 236, 218, 172, 250);

        if (sNumAvailable == 0) {
            Ui_TextEx(gfxP, cx, py + 60.0f, "No songs learned yet", 0.8f, 1, 70, 40, 15, 255, false);
            return;
        }
        {
            const SongInfo* info = &sSongs[sAvailable[sSelected]];
            OcarinaSongButtons* btns = &gOcarinaSongButtons[info->song];
            s32 n = (btns->numButtons > MAX_NOTES) ? MAX_NOTES : btns->numButtons;
            f32 spacing = (staffW - 26.0f) / (n > 1 ? n - 1 : 1);
            char counter[16];

            if (spacing > 24.0f) {
                spacing = 24.0f;
            }
            Ui_TextEx(gfxP, px + 10.0f, py + 8.0f, info->name, 0.85f, 0, 70, 40, 15, 255, false);
            counter[0] = 0;
            Mod_StrCat(counter, "< ");
            Mod_StrCatInt(counter, sSelected + 1);
            Mod_StrCat(counter, " / ");
            Mod_StrCatInt(counter, sNumAvailable);
            Mod_StrCat(counter, " >");
            Ui_TextEx(gfxP, px + pw - 10.0f, py + 10.0f, counter, 0.55f, 2, 110, 80, 50, 255, false);

            for (i = 0; i < 5; i++) {
                Ui_Rect(gfxP, staffX, staffTop + (40.0f + 30.0f * i) * k - 0.5f, staffW, 1.0f, 150, 115, 75, 255);
            }
            for (i = 0; i < n; i++) {
                u8 b = btns->buttonIndex[i];
                s32 lit = sPlaying && (i == sPlayNoteIdx);
                f32 size = NOTE_SIZE * k;
                f32 x = staffX + 8.0f + spacing * i;
                f32 y = staffTop + sNoteY[b > OCARINA_BTN_C_UP ? 0 : b] * k;

                if (b > OCARINA_BTN_C_UP) {
                    b = OCARINA_BTN_A;
                }
                Ui_Dot(gfxP, x - 0.7f, y - 0.7f, size + 1.4f, size + 1.4f, 40, 30, 20, 255);
                if (b == OCARINA_BTN_A) {
                    Ui_Dot(gfxP, x, y, size, size, lit ? 140 : 60, lit ? 190 : 120, 255, 255);
                    Ui_Label(gfxP, x + size / 2, y + size / 2, "A", size * 0.42f, 255, 255, 255, 255);
                } else {
                    static const char* sArrow[5] = { "A", "D", "R", "L", "U" };
                    Ui_Dot(gfxP, x, y, size, size, 255, lit ? 255 : 210, lit ? 160 : 50, 255);
                    Ui_Label(gfxP, x + size / 2, y + size / 2, sArrow[b], size * 0.42f, 60, 40, 0, 255);
                }
            }
            Ui_TextEx(gfxP, cx, py + ph - 34.0f, info->desc, 0.55f, 1, 70, 40, 15, 255, false);
            Ui_TextEx(gfxP, cx, py + ph - 17.0f, "Stick: choose   A: listen   B / ZL: close", 0.5f, 1, 110, 80, 50, 255,
                      false);
        }
    }
}
