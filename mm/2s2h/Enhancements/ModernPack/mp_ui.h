#ifndef MP_UI_H
#define MP_UI_H
#include "global.h"
#include "mp_ui_assets.h"

enum { UI_ALIGN_CENTER, UI_ALIGN_LEFT, UI_ALIGN_RIGHT, UI_ALIGN_STRETCH };

void Ui_Init(void);
void Ui_Begin(GraphicsContext* gfxCtx, Gfx** gfxP);
void Ui_End(Gfx** gfxP);
void Ui_Align(Gfx** gfxP, s32 align);
void Ui_Rect(Gfx** gfxP, f32 x, f32 y, f32 w, f32 h, u8 r, u8 g, u8 b, u8 a);
void Ui_RoundRect(Gfx** gfxP, f32 x, f32 y, f32 w, f32 h, f32 rad, u8 r, u8 g, u8 b, u8 a);
void Ui_Frame(Gfx** gfxP, f32 x, f32 y, f32 w, f32 h, f32 t, u8 r, u8 g, u8 b, u8 a);
void Ui_Icon(Gfx** gfxP, u8 item, f32 x, f32 y, f32 size, u8 a);
void Ui_IconSilhouette(Gfx** gfxP, u8 item, f32 x, f32 y, f32 size, u8 r, u8 g, u8 b, u8 a);
void Ui_HandText(Gfx** gfxP, f32 cx, f32 cy, const char* str, f32 capH, s32 align, u8 a);
f32 Ui_HandTextWidth(const char* str, f32 capH);
void Ui_KeySquare(Gfx** gfxP, f32 cx, f32 y, const char* text, f32 size, u8 a);
f32 Ui_KeySquareWidth(const char* text, f32 size);
f32 Ui_KeyCapWidth(const char* text, f32 scale);
void Ui_Label(Gfx** gfxP, f32 cx, f32 cy, const char* str, f32 capH, u8 r, u8 g, u8 b, u8 a);
void Ui_KeyCap(Gfx** gfxP, f32 cx, f32 y, const char* text, f32 scale, u8 a);
void Ui_ButtonIcon(Gfx** gfxP, f32 x, f32 y, f32 size, const char* label, u8 a);
void Ui_Fairy(Gfx** gfxP, f32 x, f32 y, f32 size, u8 a);
void Ui_TextEx(Gfx** gfxP, f32 x, f32 y, const char* s, f32 scale, s32 align, u8 r, u8 g, u8 b, u8 a, s32 shadow);
void Ui_Tex(Gfx** gfxP, u8* tex, s32 is16, s32 tw, s32 th, f32 x, f32 y, f32 w, f32 h, u8 r, u8 g, u8 b, u8 a);
void Ui_Dot(Gfx** gfxP, f32 x, f32 y, f32 w, f32 h, u8 r, u8 g, u8 b, u8 a);
f32 Ui_TextWidth(const char* s, f32 scale);
void Ui_Text(Gfx** gfxP, f32 x, f32 y, const char* s, f32 scale, s32 align, u8 r, u8 g, u8 b, u8 a);

void Ui_ButtonPrompt(Gfx** gfxP, f32 xRight, f32 y, const char* button, const char* action, f32 scale, u8 a);
void Switcher_Draw(PlayState* play, Gfx** gfxP);
void Hud_Draw(PlayState* play, Gfx** gfxP);
void QuickSave_Draw(PlayState* play, Gfx** gfxP);
void CutsceneFF_Draw(PlayState* play, Gfx** gfxP);
void Songbook_Draw(PlayState* play, Gfx** gfxP);
#endif
