#ifndef UI_SCALE_H
#define UI_SCALE_H

// Derives the UI scale from the panel, so a new panel size needs no hand-picked fraction.
//
// UI sizes are written in units (PILL_SIZE 30, FONT_LARGE 16, ...) laid out on the TrimUI brick's canvas of
// 341 x 256 units. A panel runs at `num / den` pixels per unit, the largest scale that still fits both axes of
// that canvas: its height is the limit for wide panels (640x480, 720x480), its width for squarer ones (720x720).
//
//   640x480 -> 480/256 = 1.875  (341 x 256 units)
//   720x480 -> 480/256 = 1.875  (384 x 256 units)
//   720x720 -> 720/341 = 2.111  (341 x 341 units)
//
// Not every panel wants it: see UI_panelScale.

#define UI_REF_WIDTH 341
#define UI_REF_HEIGHT 256

static inline void UI_deriveScale(int panel_w, int panel_h, int* num, int* den) {
	if (panel_w <= 0 || panel_h <= 0) {
		*num = UI_REF_HEIGHT;
		*den = UI_REF_HEIGHT; // 1:1, nothing sensible to derive
		return;
	}
	// panel_h / REF_HEIGHT <= panel_w / REF_WIDTH  <=>  height is the tighter axis
	if ((long)panel_h * UI_REF_WIDTH <= (long)panel_w * UI_REF_HEIGHT) {
		*num = panel_h;
		*den = UI_REF_HEIGHT;
	} else {
		*num = panel_w;
		*den = UI_REF_WIDTH;
	}
}

// Which panels get the derived (fractional) scale. Only the 4:3 640x480 panels do: at a whole-number scale of 2
// they are a 320x240 canvas, about 7% too coarse for the layout. The wider 720x480 and the 720x720 panels already
// fit at the whole-number scale `whole`, so they keep it (den 1) and behave exactly as before.
static inline void UI_panelScale(int panel_w, int panel_h, int whole, int* num, int* den) {
	if (panel_w == 640 && panel_h == 480) {
		UI_deriveScale(panel_w, panel_h, num, den);
	} else {
		*num = whole;
		*den = 1;
	}
}

#endif
