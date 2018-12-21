#include "Fonts.h"

int characterWidth(const char c, const FontInfo* font) {
	const int width = font->width;
	const int map_start = font->map_start;
	const int map_end = font->map_end;

	if (c < map_start) return 0;
	if (c > map_end) return 0;

	uint8_t const* metric_addr = font->metric + 2*(c - map_start);
	const int left = metric_addr[0];
	const int right = metric_addr[1];
	return width - left - right + 1;
}

int stringWidth(const char* s, const FontInfo* font) {
	int p = 0;
	for (; *s; ++s) {
		p += characterWidth(*s, font);
	}
	return p;
}

int fontWidth(const FontInfo* font) {
	return font->width;
}

int fontHeight(const FontInfo* font) {
	return font->height;
}
