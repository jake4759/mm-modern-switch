# MM Modern for Switch

**Majora's Mask on a modded Nintendo Switch, with modern quality-of-life features.** The aim is for it to feel as
close as possible to the Ocarina of Time remake in controls and HUD style, without changing enemies, items or story.

This is a fork of the [2 Ship 2 Harkinian](https://github.com/HarbourMasters/2ship2harkinian) Switch port
(via [reflex909/2ship2harkinian-Switch](https://github.com/reflex909/2ship2harkinian-Switch)) with the
"MM Modern" features from the PC mod built in. **No game data is included**: you need your own Majora's Mask ROM.

## What it adds

| | |
|---|---|
| **Controls** | A = action · B = jump · Y = sword · X = Tatl · R (hold) = quick item menu · ZR = use item · ZL = target · L = shield |
| **Movement** | Jump button (Deku Scrub too), sprint (hold A after a roll), faster climbing and swimming, auto-shield when locked on |
| **Aiming** | Bow and Hookshot aim over the shoulder: no first-person view |
| **Quick item menu** | Hold R, flick the stick, let go to equip. Y sorts by recently used |
| **HUD** | Remake-style item box, Tatl and D-pad icons, fading "Talk" prompt near Link, minimap in the bottom right |
| **D-pad** | Left = Ocarina · Right = Tatl · Up tap = minimap / hold = Bombers' Notebook · hold Down = quick save |
| **Text** | A, B or Y shows the current text box at once (only that box) |
| **Cutscenes** | Press X, then + to skip to gameplay. Every story event still happens |
| **Saving** | Save anywhere: pause menu Y, or hold D-pad Down. Pause menu X twice = back to title |
| **Songbook** | While playing the ocarina, ZL opens your learned songs on a staff, and A plays them back |

It also comes with a settings file that turns on 2Ship's own time-savers (faster chests and doors, easier masks,
better Song of Double Time, autosave and more).

## Install

1. Download **MM-Modern-Switch.zip** from the [latest release](../../releases/latest).
2. Copy its `switch` folder to the root of your SD card.
3. Add **`mm.o2r`** to `/switch/2ship/`. Make it from your own Majora's Mask N64 (US) ROM with
   2Ship2Harkinian **5.0.1 "Battler Bravo"** on PC.
4. Hold **R** while starting any game to open the homebrew menu (not from the Album), then open MM Modern.

## Building

Every push builds automatically with GitHub Actions (`.github/workflows/main.yml`) using the devkitPro
`devkita64` container. The new code is in `mm/2s2h/Enhancements/ModernPack/`.

## Credits

2 Ship 2 Harkinian by HarbourMasters and contributors (see `docs/2SHIP_README.md` and `LICENSE`).
Switch port by its respective authors. MM Modern features ported from the Zelda64Recomp mod.
