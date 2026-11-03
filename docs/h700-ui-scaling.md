# H700: scale the 640x480 UI to the TrimUI reference layout

## Problem

The UI is written in layout units (`PILL_SIZE 30`, `FONT_LARGE 16`, `PADDING`, ...) and `SCALE1()` multiplies
them by `FIXED_SCALE`, a whole number. The TrimUI brick runs 1024x768 at 3, which is a 341x256-unit canvas. The
H700's 640x480 panels (RG40XX H/V, RG28XX, default) run at 2, a 320x240 canvas, so every element is about 7%
larger relative to the screen and the main list shows 6 rows instead of 7.

## Scope

Only the 640x480 panels change: 15/8 gives 341x256 units, 7 main rows, 5-unit padding. The 720x480 (RG34XX, RG SP)
and 720x720 (RG Cube XX) panels and HDMI already fit at scale 2 and keep it, with their original `PADDING` and
`MAIN_ROW_COUNT`.

## Changes

1. **Fractional UI scale.** `defines.h`: `SCALE1..4` multiply by `UI_SCALE_NUM / UI_SCALE_DEN`. Defaults are
   `FIXED_SCALE / 1`, so other platforms are unchanged. `h700/platform/platform.h` sets 15/8 on 640x480. Integer rounding is by truncation, so avoid summing separately scaled values when one scaled
   total is possible.
2. **Logical size.** Code that divided by `FIXED_SCALE` to get the canvas (`minarch/ma_menu.c`, `clock/clock.c`)
   uses `LOGICAL_HEIGHT` / `UI_SCALE_*` instead.
3. **Sprites.** A sprite sheet cannot be shrunk by a fraction: rounded rects no longer line up and neighbouring
   sprites bleed. When the scale is fractional, each sprite is cut from the `@3x` sheet and area-averaged to its
   exact scaled size, then packed into an atlas (`common/ui_resample.h`, `GFX_loadSheet` in `api.c`). System icons
   go through `GFX_loadUIImage` and the `@3x` set (`GFX_uiSheetScale()`). Whole-number platforms load their own
   `@Nx` image unchanged.
4. **Odd sprite sizes.** Scaled sprites can be an odd number of pixels. `GFX_blitPillColor` and
   `GFX_blitRectColor` split sprites in halves; the second half now takes the leftover pixel, otherwise a 1px seam
   (black grid lines on tiles) appears.
5. **Rows and padding.** The fractional layout uses 5-unit `PADDING`, as on the brick; 7 rows plus the hint bar
   only fit with 5. `MAIN_ROW_COUNT` is 7 there. Other panels keep their original values.
6. **Battery gap.** `api.c` used `FIXED_SCALE` pixels for a 1-unit gap; now `SCALE1(1)`.

## Verification

- `make test` (host tests) and `make PLATFORMS=h700 all` build clean.
- 640x480 confirmed on an RG40XX H: launcher (7 rows, last row clear of hints), hints, battery/time, quick menu,
  in-game menu.
- Resampler checked on the host by rendering all 47 sprites from `assets@3x.png`.
- Other panels and HDMI are unchanged by construction: same scale, padding and row count as upstream `main`.

## Open questions / follow-ups

- Layouts that hard-code pixel offsets (e.g. the `+1`/`-1` text nudges in `GFX_blitBatteryAtPosition`) are not
  scale-aware.
- Settings (C++), Clock and Input apps were built with these changes but not visually reviewed.
