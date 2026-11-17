// Draws UI the same way the game does (N64 display lists), so the quick menu and HUD look like part of the game.
// Port of ui_gfx.c from the PC mod: widescreen alignment uses 2Ship's own HUD edge helpers.
#include "mp_common.h"
#include "mp_ui.h"
#include "BenPort.h"

extern TexturePtr gItemIcons[];

static s32 sAlign = UI_ALIGN_CENTER;

void Ui_Init(void) {
}

static u8* Ui_GetItemIcon(u8 item) {
    if (item >= 0x50) {
        return NULL;
    }
    return (u8*)gItemIcons[item];
}

void Ui_Begin(GraphicsContext* gfxCtx, Gfx** gfxP) {
    Gfx* gfx = *gfxP;
    gDPPipeSync(gfx++);
    gDPSetOtherMode(gfx++,
                    G_AD_DISABLE | G_CD_DISABLE | G_CK_NONE | G_TC_FILT | G_TF_BILERP | G_TT_NONE | G_TL_TILE |
                        G_TD_CLAMP | G_TP_NONE | G_CYC_1CYCLE | G_PM_NPRIMITIVE,
                    G_AC_NONE | G_ZS_PRIM | G_RM_XLU_SURF | G_RM_XLU_SURF2);
    sAlign = UI_ALIGN_CENTER;
    *gfxP = gfx;
}

void Ui_End(Gfx** gfxP) {
    Gfx* gfx = *gfxP;
    gDPPipeSync(gfx++);
    sAlign = UI_ALIGN_CENTER;
    *gfxP = gfx;
}

void Ui_Align(Gfx** gfxP, s32 align) {
    sAlign = align;
}

// Map an x in 320-wide HUD space to the screen, keeping left/right anchored to the screen edges in widescreen.
static f32 UiMapX(f32 x, s32 isRightEdge) {
    switch (sAlign) {
        case UI_ALIGN_LEFT:
            return OTRGetDimensionFromLeftEdge(x);
        case UI_ALIGN_RIGHT:
            return OTRGetDimensionFromRightEdge(x);
        case UI_ALIGN_STRETCH:
            return isRightEdge ? OTRGetDimensionFromRightEdge(x) : OTRGetDimensionFromLeftEdge(x);
        default:
            return x;
    }
}

static void UiTexRect(Gfx** gfxP, f32 x0, f32 y0, f32 x1, f32 y1, s32 dsdx, s32 dtdy) {
    Gfx* gfx = *gfxP;
    f32 mx0 = UiMapX(x0, false);
    f32 mx1 = UiMapX(x1, true);
    gSPWideTextureRectangle(gfx++, (s32)(mx0 * 4), (s32)(y0 * 4), (s32)(mx1 * 4), (s32)(y1 * 4), G_TX_RENDERTILE, 0,
                            0, dsdx, dtdy);
    *gfxP = gfx;
}

void Ui_Rect(Gfx** gfxP, f32 x, f32 y, f32 w, f32 h, u8 r, u8 g, u8 b, u8 a) {
    Gfx* gfx = *gfxP;
    if (a == 0 || w <= 0 || h <= 0) {
        return;
    }
    gDPPipeSync(gfx++);
    gDPSetCombineMode(gfx++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    gDPSetPrimColor(gfx++, 0, 0, r, g, b, a);
    *gfxP = gfx;
    UiTexRect(gfxP, x, y, x + w, y + h, 1 << 10, 1 << 10);
    gfx = *gfxP;
    *gfxP = gfx;
}

// Rectangle with softened corners (good enough for pills and boxes at this size).
void Ui_RoundRect(Gfx** gfxP, f32 x, f32 y, f32 w, f32 h, f32 rad, u8 r, u8 g, u8 b, u8 a) {
    Ui_Rect(gfxP, x + rad, y, w - 2 * rad, h, r, g, b, a);
    Ui_Rect(gfxP, x, y + rad, rad, h - 2 * rad, r, g, b, a);
    Ui_Rect(gfxP, x + w - rad, y + rad, rad, h - 2 * rad, r, g, b, a);
    if (rad >= 2.0f) {
        f32 q = rad * 0.4f;
        Ui_Rect(gfxP, x + q, y + q, rad - q, rad - q, r, g, b, a);
        Ui_Rect(gfxP, x + w - rad, y + q, rad - q, rad - q, r, g, b, a);
        Ui_Rect(gfxP, x + q, y + h - rad, rad - q, rad - q, r, g, b, a);
        Ui_Rect(gfxP, x + w - rad, y + h - rad, rad - q, rad - q, r, g, b, a);
    }
}

void Ui_Frame(Gfx** gfxP, f32 x, f32 y, f32 w, f32 h, f32 t, u8 r, u8 g, u8 b, u8 a) {
    Ui_Rect(gfxP, x, y, w, t, r, g, b, a);
    Ui_Rect(gfxP, x, y + h - t, w, t, r, g, b, a);
    Ui_Rect(gfxP, x, y + t, t, h - 2 * t, r, g, b, a);
    Ui_Rect(gfxP, x + w - t, y + t, t, h - 2 * t, r, g, b, a);
}

void Ui_Icon(Gfx** gfxP, u8 item, f32 x, f32 y, f32 size, u8 a) {
    Gfx* gfx;
    u8* tex = Ui_GetItemIcon(item);
    s32 step;

    if (tex == NULL || a == 0) {
        return;
    }
    gfx = *gfxP;
    step = (s32)((32.0f / size) * (1 << 10));
    gDPPipeSync(gfx++);
    gDPSetCombineMode(gfx++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPSetPrimColor(gfx++, 0, 0, 255, 255, 255, a);
    gDPLoadTextureBlock(gfx++, tex, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                        G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    *gfxP = gfx;
    UiTexRect(gfxP, x, y, x + size, y + size, step, step);
    gfx = *gfxP;
    *gfxP = gfx;
}

// Item icon drawn as a solid-colour silhouette (uses only the icon's shape).
void Ui_IconSilhouette(Gfx** gfxP, u8 item, f32 x, f32 y, f32 size, u8 r, u8 g, u8 b, u8 a) {
    Gfx* gfx;
    u8* tex = Ui_GetItemIcon(item);
    s32 step;

    if (tex == NULL || a == 0) {
        return;
    }
    gfx = *gfxP;
    step = (s32)((32.0f / size) * (1 << 10));
    gDPPipeSync(gfx++);
    gDPSetCombineLERP(gfx++, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0);
    gDPSetPrimColor(gfx++, 0, 0, r, g, b, a);
    gDPLoadTextureBlock(gfx++, tex, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                        G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    *gfxP = gfx;
    UiTexRect(gfxP, x, y, x + size, y + size, step, step);
    gfx = *gfxP;
    *gfxP = gfx;
}

// ---- Button labels: clean bold font, centred on the visible letters ----

typedef struct {
    u8* glyphs;
    f32* adv;
    u8* inkL;
    u8* inkR;
    s32 capTop;
    s32 hasLower;
} LabelFont;

static LabelFont sLabelFont = { gUiLabelGlyphs, gUiLabelAdvance, gUiLabelInkL, gUiLabelInkR, UI_LABEL_CAP_TOP, false };
static LabelFont sHandFont = { gUiHandGlyphs, gUiHandAdvance, gUiHandInkL, gUiHandInkR, UI_HAND_CAP_TOP, true };
static LabelFont* sCurFont = &sLabelFont;

static s32 LabelIndex(u8 c) {
    if (c >= 'a' && c <= 'z') {
        if (sCurFont->hasLower) {
            return 36 + (c - 'a');
        }
        c -= 'a' - 'A';
    }
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'A' && c <= 'Z') {
        return 10 + (c - 'A');
    }
    return -1;
}



// Width of the visible ink of a label at the given capital-letter height.
static f32 LabelInkWidth(const char* str, f32 capH, f32* firstInkL) {
    LabelFont* f = sCurFont;
    f32 k = capH / (UI_FONT_BASELINE - f->capTop); // units per font pixel
    f32 pen = 0, left = 0, right = 0;
    s32 first = true;
    for (; *str; str++) {
        s32 i = LabelIndex((u8)*str);
        if (i < 0) {
            pen += capH * 0.3f;
            continue;
        }
        if (first) {
            left = f->inkL[i] * k;
            first = false;
        }
        right = pen + f->inkR[i] * k;
        pen += f->adv[i] * k;
    }
    if (firstInkL != NULL) {
        *firstInkL = left;
    }
    return right - left;
}

// Draw a label so its visible letters are centred on (cx, cy).
void Ui_Label(Gfx** gfxP, f32 cx, f32 cy, const char* str, f32 capH, u8 r, u8 g, u8 b, u8 a) {
    Gfx* gfx;
    LabelFont* f = sCurFont;
    f32 k = capH / (UI_FONT_BASELINE - f->capTop);
    f32 cell = UI_FONT_CELL * k;
    f32 inkL;
    f32 w = LabelInkWidth(str, capH, &inkL);
    f32 pen = cx - w / 2 - inkL;
    f32 top = cy - capH / 2 - f->capTop * k;
    s32 step = (s32)(((f32)UI_FONT_CELL / cell) * (1 << 10));

    if (a == 0) {
        return;
    }
    gfx = *gfxP;
    gDPPipeSync(gfx++);
    gDPSetCombineLERP(gfx++, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0);
    gDPSetPrimColor(gfx++, 0, 0, r, g, b, a);
    for (; *str; str++) {
        s32 i = LabelIndex((u8)*str);
        if (i < 0) {
            pen += capH * 0.3f;
            continue;
        }
        gDPLoadTextureBlock_4b(gfx++, &f->glyphs[i * (UI_FONT_CELL * UI_FONT_CELL / 2)], G_IM_FMT_I, UI_FONT_CELL,
                               UI_FONT_CELL, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK,
                               G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        *gfxP = gfx;
    UiTexRect(gfxP, pen, top, pen + cell, top + cell, step, step);
    gfx = *gfxP;
        pen += f->adv[i] * k;
    }
    *gfxP = gfx;
}

// Hand-lettered text (MedievalSharp) with a dark outline. align: 1 = centre on cx, 2 = right edge at cx.
// cy = vertical centre of the capital letters.
void Ui_HandText(Gfx** gfxP, f32 cx, f32 cy, const char* str, f32 capH, s32 align, u8 a) {
    static const f32 sOff[5][2] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 }, { 0.8f, 0.8f } };
    f32 o = capH * 0.07f;
    f32 inkL;
    f32 w;
    s32 k;

    sCurFont = &sHandFont;
    w = LabelInkWidth(str, capH, &inkL);
    if (align == 2) {
        cx -= w / 2;
    } else if (align == 0) {
        cx += w / 2;
    }
    for (k = 0; k < 5; k++) {
        Ui_Label(gfxP, cx + sOff[k][0] * o, cy + sOff[k][1] * o, str, capH, 0, 0, 0, a);
    }
    Ui_Label(gfxP, cx, cy, str, capH, 245, 238, 220, a);
    sCurFont = &sLabelFont;
}

f32 Ui_HandTextWidth(const char* str, f32 capH) {
    f32 w;
    sCurFont = &sHandFont;
    w = LabelInkWidth(str, capH, NULL);
    sCurFont = &sLabelFont;
    return w;
}

// Light "key cap" with the button name centred inside, like the remake's button labels.
// y = top of the cap; scale 0.62 = the HUD size.
void Ui_KeyCap(Gfx** gfxP, f32 cx, f32 y, const char* text, f32 scale, u8 a) {
    f32 m = scale / 0.62f;
    f32 capH = 4.0f * m;
    f32 h = 9.0f * m;
    f32 w = LabelInkWidth(text, capH, NULL) + 7.0f * m;
    Ui_Tex(gfxP, gUiKeyCapTex, true, 64, 32, cx - w / 2, y, w, h, 255, 255, 255, (u8)((a * 15) / 16));
    // the cap art has a 2px transparent margin top and bottom, so its visual middle is the rect's middle
    Ui_Label(gfxP, cx, y + h / 2, text, capH, 30, 30, 30, a);
}

// Near-square white key (like the remake's small "R" key). size = key height.
void Ui_KeySquare(Gfx** gfxP, f32 cx, f32 y, const char* text, f32 size, u8 a) {
    f32 capH = size * 0.5f;
    f32 w = LabelInkWidth(text, capH, NULL) + size * 0.55f;
    if (w < size) {
        w = size;
    }
    Ui_Tex(gfxP, gUiKeyCapTex, true, 64, 32, cx - w / 2, y - size * 0.07f, w, size * 1.14f, 255, 255, 255, a);
    Ui_Label(gfxP, cx, y + size / 2, text, capH, 30, 30, 30, a);
}

f32 Ui_KeySquareWidth(const char* text, f32 size) {
    f32 w = LabelInkWidth(text, size * 0.5f, NULL) + size * 0.55f;
    return (w < size) ? size : w;
}

f32 Ui_KeyCapWidth(const char* text, f32 scale) {
    f32 m = scale / 0.62f;
    return LabelInkWidth(text, 4.0f * m, NULL) + 7.0f * m;
}

// Tatl
void Ui_Fairy(Gfx** gfxP, f32 x, f32 y, f32 size, u8 a) {
    Ui_Tex(gfxP, gUiFairyTex, true, 44, 44, x, y, size, size, 255, 255, 255, a);
}

// glyph cell size (in screen units) per 1.0 of "scale", normalised so capital letters are always the same height
#define FONT_K (24.0f * 27.0f / UI_FONT_CAP_HEIGHT)

f32 Ui_TextWidth(const char* str, f32 scale) {
    f32 cell = scale * FONT_K;
    f32 w = 0;
    for (; *str; str++) {
        u8 c = (u8)*str;
        if (c >= 32 && c < 127) {
            w += gUiFontAdvance[c - 32] * cell / UI_FONT_CELL;
        }
    }
    return w;
}

static void DrawTextPass(Gfx** gfxP, f32 x, f32 y, const char* str, f32 scale, u8 r, u8 g, u8 b, u8 a) {
    Gfx* gfx = *gfxP;
    f32 cell = scale * FONT_K;
    s32 step = (s32)(((f32)UI_FONT_CELL / cell) * (1 << 10));
    f32 cy = y + 2.5f * scale - UI_FONT_CAP_TOP * cell / UI_FONT_CELL;
    f32 lead = 2.0f * cell / UI_FONT_CELL;

    gDPPipeSync(gfx++);
    gDPSetCombineLERP(gfx++, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0);
    gDPSetPrimColor(gfx++, 0, 0, r, g, b, a);
    for (; *str; str++) {
        u8 c = (u8)*str;
        if (c < 32 || c >= 127) {
            continue;
        }
        if (c != ' ') {
            f32 gx = x - lead;
            gDPLoadTextureBlock_4b(gfx++, &gUiFontGlyphs[(c - 32) * (UI_FONT_CELL * UI_FONT_CELL / 2)], G_IM_FMT_I,
                                   UI_FONT_CELL, UI_FONT_CELL, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                                   G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            *gfxP = gfx;
    UiTexRect(gfxP, gx, cy, gx + cell, cy + cell, step, step);
    gfx = *gfxP;
        }
        x += gUiFontAdvance[c - 32] * cell / UI_FONT_CELL;
    }
    *gfxP = gfx;
}

void Ui_TextEx(Gfx** gfxP, f32 x, f32 y, const char* s, f32 scale, s32 align, u8 r, u8 g, u8 b, u8 a, s32 shadow) {
    if (a == 0) {
        return;
    }
    if (align == 1) {
        x -= Ui_TextWidth(s, scale) / 2;
    } else if (align == 2) {
        x -= Ui_TextWidth(s, scale);
    }
    if (shadow) {
        DrawTextPass(gfxP, x + 0.45f * scale, y + 0.45f * scale, s, scale, 0, 0, 0, (u8)((a * 3) / 5));
    }
    DrawTextPass(gfxP, x, y, s, scale, r, g, b, a);
}

// align: 0 = left edge at x, 1 = centered on x, 2 = right edge at x
void Ui_Text(Gfx** gfxP, f32 x, f32 y, const char* s, f32 scale, s32 align, u8 r, u8 g, u8 b, u8 a) {
    Ui_TextEx(gfxP, x, y, s, scale, align, r, g, b, a, true);
}

// Draw one of the mod's textures (IA8 or IA16), tinted by (r,g,b,a).
void Ui_Tex(Gfx** gfxP, u8* tex, s32 is16, s32 tw, s32 th, f32 x, f32 y, f32 w, f32 h, u8 r, u8 g, u8 b, u8 a) {
    Gfx* gfx = *gfxP;
    s32 sx, sy;

    if (a == 0 || w <= 0 || h <= 0) {
        return;
    }
    sx = (s32)(((f32)tw / w) * (1 << 10));
    sy = (s32)(((f32)th / h) * (1 << 10));
    gDPPipeSync(gfx++);
    gDPSetCombineMode(gfx++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPSetPrimColor(gfx++, 0, 0, r, g, b, a);
    if (is16) {
        gDPLoadTextureBlock(gfx++, tex, G_IM_FMT_IA, G_IM_SIZ_16b, tw, th, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                            G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    } else {
        gDPLoadTextureBlock(gfx++, tex, G_IM_FMT_IA, G_IM_SIZ_8b, tw, th, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                            G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    }
    *gfxP = gfx;
    UiTexRect(gfxP, x, y, x + w, y + h, sx, sy);
    gfx = *gfxP;
    *gfxP = gfx;
}

// Round controller button icon. PlayStation symbols get their shape; other controllers get a letter on a disc.
void Ui_ButtonIcon(Gfx** gfxP, f32 x, f32 y, f32 size, const char* label, u8 a) {
    u8* tex = gUiBtnBlankTex;
    if (Mod_StrEq(label, "Square")) {
        tex = gUiBtnSquareTex;
    } else if (Mod_StrEq(label, "Circle")) {
        tex = gUiBtnCircleTex;
    } else if (Mod_StrEq(label, "Triangle")) {
        tex = gUiBtnTriangleTex;
    } else if (Mod_StrEq(label, "Cross")) {
        tex = gUiBtnCrossTex;
    }
    Ui_Tex(gfxP, tex, true, 32, 32, x, y, size, size, 255, 255, 255, a);
    if (tex == gUiBtnBlankTex) {
        f32 sc = size / 14.0f;
        Ui_TextEx(gfxP, x + size / 2, y + size / 2 - 5.5f * sc, label, 0.55f * sc, 1, 30, 30, 30, a, false);
    }
}

// Prompt like "[icon] Save" in a dark rounded box; right edge at xRight.
void Ui_ButtonPrompt(Gfx** gfxP, f32 xRight, f32 y, const char* button, const char* action, f32 scale, u8 a) {
    f32 aw = Ui_TextWidth(action, scale);
    f32 icon = 9.5f;
    f32 pad = 5.0f;
    f32 w = pad + icon + 4.0f + aw + pad;
    f32 h = 14.0f;
    f32 x = xRight - w;

    Ui_Tex(gfxP, gUiPanelTex, false, 64, 32, x, y, w, h, 0, 0, 0, (u8)((a * 3) / 5));
    Ui_ButtonIcon(gfxP, x + pad, y + (h - icon) / 2, icon, button, a);
    Ui_Text(gfxP, x + pad + icon + 4.0f, y + 2.0f, action, scale, 0, 245, 243, 232, a);
}

void Ui_Dot(Gfx** gfxP, f32 x, f32 y, f32 w, f32 h, u8 r, u8 g, u8 b, u8 a) {
    Ui_Tex(gfxP, gUiDotTex, true, 16, 16, x, y, w, h, r, g, b, a);
}
