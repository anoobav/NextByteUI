// Host test for UI_deriveScale and the rounding rule. Build and run:
//   cc -I. -o /tmp/ui_scale_test ui_scale_test.c && /tmp/ui_scale_test
#include <stdio.h>
#include "ui_scale.h"

static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { printf("FAIL: " __VA_ARGS__); printf("\n"); failures++; } } while (0)

static int scale1(int a, int num, int den) { return (a * num + den / 2) / den; }

static void panel(int w, int h, int want_units_w, int want_units_h, int want_rows) {
	int num, den;
	UI_deriveScale(w, h, &num, &den);
	int uw = w * den / num, uh = h * den / num;
	int rows = (uh - 2 * 5 - 30) / 30;
	CHECK(uw == want_units_w && uh == want_units_h, "%dx%d -> %dx%d units (want %dx%d)", w, h, uw, uh, want_units_w, want_units_h);
	CHECK(rows == want_rows, "%dx%d -> %d rows (want %d)", w, h, rows, want_rows);
	// the rows and the hint bar must not overlap, in pixels
	int last_row_bottom = scale1(5 + rows * 30, num, den);
	int hints_top = h - scale1(5 + 30, num, den);
	CHECK(last_row_bottom <= hints_top, "%dx%d rows end at %d, hints start at %d", w, h, last_row_bottom, hints_top);
	CHECK(uw >= UI_REF_WIDTH, "%dx%d narrower than the reference canvas", w, h);
}

int main(void) {
	panel(640, 480, 341, 256, 7);
	panel(1024, 768, 341, 256, 7); // the reference itself

	// rounding, not truncation: the values the battery sprite offsets and fonts rely on at 15/8
	CHECK(scale1(30, 480, 256) == 56, "pill");
	CHECK(scale1(3, 480, 256) == 6, "battery fill x");
	CHECK(scale1(2, 480, 256) == 4, "battery fill y");
	CHECK(scale1(5, 480, 256) == 9, "padding");
	CHECK(scale1(10, 480, 256) == 19, "FONT_TINY");
	CHECK(scale1(12, 480, 256) == 23, "FONT_SMALL");
	CHECK(scale1(16, 480, 256) == 30, "FONT_LARGE");
	CHECK(scale1(7, 3, 1) == 21 && scale1(30, 2, 1) == 60, "whole-number scales unchanged");

	// only 640x480 is fractional; the other panels keep the whole-number scale, as before
	int num, den;
	UI_panelScale(640, 480, 2, &num, &den);
	CHECK(num == 480 && den == 256, "640x480 derives 480/256");
	UI_panelScale(720, 480, 2, &num, &den);
	CHECK(num == 2 && den == 1, "720x480 keeps scale 2");
	UI_panelScale(720, 720, 2, &num, &den);
	CHECK(num == 2 && den == 1, "720x720 keeps scale 2");

	UI_deriveScale(0, 0, &num, &den);
	CHECK(num == den, "bad panel size falls back to 1:1");

	puts(failures ? "FAILED" : "ok");
	return failures != 0;
}
