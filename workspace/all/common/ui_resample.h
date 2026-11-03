#ifndef UI_RESAMPLE_H
#define UI_RESAMPLE_H

#include <SDL2/SDL.h>

// Header-only so every binary that links api.c gets it with no makefile change.
//
// Used when the UI scale is a fraction (see UI_SCALE_NUM / UI_SCALE_DEN in defines.h). A sprite sheet cut for a
// whole-number scale cannot simply be shrunk by a fraction: the rounded rects no longer line up with the
// shrunken sheet and neighbouring sprites bleed into each other. Instead each sprite is cut out of the large
// sheet and averaged down to the exact pixel size the layout expects.

// Averages the pixels of `sr` in `src` (32-bit ARGB8888) down or up to dw x dh. Alpha-weighted so
// transparent pixels do not darken the edges. Returns a new ARGB8888 surface, or NULL.
static inline SDL_Surface* UI_resampleRect(SDL_Surface* src, const SDL_Rect* sr, int dw, int dh) {
	if (!src || !sr || dw <= 0 || dh <= 0 || sr->w <= 0 || sr->h <= 0) return NULL;
	if (src->format->format != SDL_PIXELFORMAT_ARGB8888) return NULL;
	if (sr->x < 0 || sr->y < 0 || sr->x + sr->w > src->w || sr->y + sr->h > src->h) return NULL;

	SDL_Surface* out = SDL_CreateRGBSurfaceWithFormat(0, dw, dh, 32, SDL_PIXELFORMAT_ARGB8888);
	if (!out) return NULL;

	const float xs = (float)sr->w / dw;
	const float ys = (float)sr->h / dh;
	for (int dy = 0; dy < dh; dy++) {
		float y0 = dy * ys, y1 = y0 + ys;
		for (int dx = 0; dx < dw; dx++) {
			float x0 = dx * xs, x1 = x0 + xs;
			float sum_a = 0, sum_r = 0, sum_g = 0, sum_b = 0, area = 0;
			for (int sy = (int)y0; sy < y1 && sy < sr->h; sy++) {
				float wy = (sy + 1 < y1 ? sy + 1 : y1) - (sy > y0 ? sy : y0);
				const Uint32* row = (const Uint32*)((const Uint8*)src->pixels + (sr->y + sy) * src->pitch) + sr->x;
				for (int sx = (int)x0; sx < x1 && sx < sr->w; sx++) {
					float wx = (sx + 1 < x1 ? sx + 1 : x1) - (sx > x0 ? sx : x0);
					float w = wx * wy;
					Uint32 p = row[sx];
					float a = (p >> 24) & 0xff;
					sum_a += a * w;
					sum_r += ((p >> 16) & 0xff) * a * w;
					sum_g += ((p >> 8) & 0xff) * a * w;
					sum_b += (p & 0xff) * a * w;
					area += w;
				}
			}
			Uint32 px = 0;
			if (area > 0 && sum_a > 0) {
				int a = (int)(sum_a / area + 0.5f);
				int r = (int)(sum_r / sum_a + 0.5f);
				int g = (int)(sum_g / sum_a + 0.5f);
				int b = (int)(sum_b / sum_a + 0.5f);
				px = ((Uint32)a << 24) | ((Uint32)r << 16) | ((Uint32)g << 8) | (Uint32)b;
			}
			((Uint32*)((Uint8*)out->pixels + dy * out->pitch))[dx] = px;
		}
	}
	return out;
}

// Packs `count` sprites into one new ARGB8888 atlas, left to right in rows with a 1px gutter. Sprite i is cut
// from `sheet` at `src[i]` and averaged to dst[i].w x dst[i].h; dst[i].x/.y are then rewritten to where it was
// placed. Entries with no size are skipped. Returns the atlas, or NULL.
static inline SDL_Surface* UI_buildAtlas(SDL_Surface* sheet, const SDL_Rect* src, SDL_Rect* dst, int count, int atlas_w) {
	if (!sheet || !src || !dst || count <= 0 || atlas_w <= 0) return NULL;

	int x = 0, y = 0, row_h = 0, total_h = 0;
	for (int i = 0; i < count; i++) { // measure
		if (dst[i].w <= 0 || dst[i].h <= 0) continue;
		if (x + dst[i].w > atlas_w) { x = 0; y += row_h + 1; row_h = 0; }
		x += dst[i].w + 1;
		if (dst[i].h > row_h) row_h = dst[i].h;
		total_h = y + row_h;
	}
	if (total_h <= 0) return NULL;

	SDL_Surface* atlas = SDL_CreateRGBSurfaceWithFormat(0, atlas_w, total_h, 32, SDL_PIXELFORMAT_ARGB8888);
	if (!atlas) return NULL;
	SDL_FillRect(atlas, NULL, 0);

	x = 0; y = 0; row_h = 0;
	for (int i = 0; i < count; i++) {
		if (dst[i].w <= 0 || dst[i].h <= 0) continue;
		if (x + dst[i].w > atlas_w) { x = 0; y += row_h + 1; row_h = 0; }
		SDL_Surface* piece = UI_resampleRect(sheet, &src[i], dst[i].w, dst[i].h);
		if (piece) {
			SDL_SetSurfaceBlendMode(piece, SDL_BLENDMODE_NONE);
			SDL_BlitSurface(piece, NULL, atlas, &(SDL_Rect){x, y});
			SDL_FreeSurface(piece);
		}
		dst[i].x = x;
		dst[i].y = y;
		x += dst[i].w + 1;
		if (dst[i].h > row_h) row_h = dst[i].h;
	}
	SDL_SetSurfaceBlendMode(atlas, SDL_BLENDMODE_BLEND);
	return atlas;
}

#endif
