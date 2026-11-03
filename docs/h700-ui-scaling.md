# H700: scale the UI to the TrimUI reference layout on every panel

## Problem

The UI is written in layout units (`PILL_SIZE 30`, `FONT_LARGE 16`, `PADDING`, ...) and `SCALE1()` multiplies
them by a scale. It used to be the whole number `FIXED_SCALE`. The TrimUI brick runs 1024x768 at 3, which is a
341x256-unit canvas. The H700 ran 640x480 at 2, a 320x240 canvas, so every element was about 7% larger relative
to the screen and the main list showed 6 rows instead of 7. The other H700 panels had the same problem.

## Scope: only the 640x480 panels

`common/ui_scale.h`, `UI_panelScale()`. The 640x480 panels (RG40XX H/V, RG28XX, default) get a derived fractional
scale: the largest scale at which the panel still contains the reference canvas, `min(h / 256, w / 341)` =
480/256 = 1.875, a 341x256-unit canvas with 7 main rows. `PLAT_initPlatform` computes it once
(`ui_scale_num` / `ui_scale_den`).

Every other panel keeps exactly the behaviour it had on upstream `main`: they already fit at the whole-number scale
(an RG34XX and an RG Cube XX were checked side by side with the 640x480 build), so scale 2, and the original
`PADDING` and `MAIN_ROW_COUNT`.

| Panel | Devices | Scale | Canvas (units) | Main rows | Padding |
|---|---|---|---|---|---|
| 640x480 | RG40XX H/V, RG28XX, default | 480/256 = 1.875 | 341x256 | 7 | 5 |
| 720x480 | RG34XX, RG34XXSP, RG SP | 2 (unchanged) | 360x240 | 6 | 10 |
| 720x720 | RG Cube XX | 2 (unchanged) | 360x360 | 10 | 5 |
| HDMI 1280x720 | all | 2 (unchanged) | 640x360 | 10 | 5 |

## Changes

1. **Fractional UI scale.** `defines.h`: `SCALE1..4` multiply by `UI_SCALE_NUM / UI_SCALE_DEN`, rounded to the
   nearest pixel. Defaults are `FIXED_SCALE / 1`, so other platforms are unchanged. `h700/platform/platform.h`
   reads the derived fraction. Each call rounds, so prefer one `SCALE1` of a sum over a sum of `SCALE1`s.
2. **Logical size.** Code that divided by `FIXED_SCALE` to get the canvas (`minarch/ma_menu.c`, `clock/clock.c`)
   uses `LOGICAL_HEIGHT` / `UI_SCALE_*` instead.
3. **Sprites.** A sprite sheet cannot be shrunk by a fraction: rounded rects no longer line up and neighbouring
   sprites bleed. When the scale is fractional, each sprite is cut from the `@3x` sheet and area-averaged to its
   exact scaled size, then packed into an atlas (`common/ui_resample.h`, `GFX_loadSheet` in `api.c`). System icons
   go through `GFX_loadUIImage` and the `@3x` set (`GFX_uiSheetScale()`, which falls back to `@2x` if `@3x` is
   not installed). Whole-number platforms load their own `@Nx` image unchanged. The launcher's quick menu loads
   each icon once and keeps it (it used to load, and leak, every icon on every frame).
4. **Odd sprite sizes.** Scaled sprites can be an odd number of pixels. `GFX_blitPillColor` and
   `GFX_blitRectColor` split sprites in halves; the second half now takes the leftover pixel, otherwise a 1px seam
   (black grid lines on tiles) appears.
5. **Rows and padding.** `PADDING` is 5 units everywhere, as on the brick; 7 rows plus the hint bar only fit with
   5. Only the fractional 640x480 layout changes `PADDING` and `MAIN_ROW_COUNT` (table above).
6. **Battery gap.** `api.c` used `FIXED_SCALE` pixels for a 1-unit gap; now `SCALE1(1)`.

## Verification

- `make test` (host tests) and `make PLATFORMS=h700 all` build clean.
- 640x480 confirmed on an RG40XX H: launcher (7 rows, last row clear of hints), hints, battery/time, quick menu,
  in-game menu.
- Resampler checked on the host by rendering all 47 sprites from `assets@3x.png`.
- **Not yet checked on device:** 720x480, 720x720 and HDMI. Check: list rows vs hint bar, status pill padding,
  quick menu tile seams, Settings app, minarch menu, clock, game switcher.

## Known limits

- Row pitch is `SCALE1(PADDING + j * PILL_SIZE)`, so on 1.875 it alternates between 56 and 57 px (a pill is 56.25).
  The pills do not touch, so it is not visible; a continuous highlight would show it.
- The `+1`/`-1` nudges in `GFX_blitBatteryAtPosition` are deliberate raw pixels (glyph bearing, not layout).
- Host test: `cc -I workspace/all/common -o /tmp/t workspace/all/common/ui_scale_test.c && /tmp/t`.
