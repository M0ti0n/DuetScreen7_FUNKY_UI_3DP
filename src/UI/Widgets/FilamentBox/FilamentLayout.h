/*
 * FilamentLayout.h
 *
 * Positions on the filament page (1024 x 600), made by art/filament/export_filament.py.
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace UI::FilamentLayout
{
	struct Rect
	{
		int16_t x, y, w, h;
	};
	struct Sprite
	{
		const char* file;
		int16_t x, y;
	};

	constexpr const char* BACKGROUND = "filament/background.png";

	/* One bay of the MMU box: the spool (filament area see-through), the filament (white, tinted),
	 * its shading, an empty spool, the red glow, and the screen in front of it */
	struct Bay
	{
		Sprite spool, filament, shade, empty, glow;
		Rect screen;
	};
	constexpr Bay BAYS[4] = {
		{{"filament/bay0_spool.png", 191, 77},
		 {"filament/bay0_fil.png", 210, 103},
		 {"filament/bay0_shade.png", 210, 105},
		 {"filament/bay0_empty.png", 210, 103},
		 {"filament/bay0_glow.png", 165, 241},
		 {184, 260, 148, 173}},
		{{"filament/bay1_spool.png", 359, 77},
		 {"filament/bay1_fil.png", 379, 103},
		 {"filament/bay1_shade.png", 380, 105},
		 {"filament/bay1_empty.png", 379, 103},
		 {"filament/bay1_glow.png", 336, 241},
		 {355, 260, 148, 173}},
		{{"filament/bay2_spool.png", 518, 77},
		 {"filament/bay2_fil.png", 539, 103},
		 {"filament/bay2_shade.png", 539, 105},
		 {"filament/bay2_empty.png", 539, 103},
		 {"filament/bay2_glow.png", 507, 241},
		 {525, 260, 148, 173}},
		{{"filament/bay3_spool.png", 683, 77},
		 {"filament/bay3_fil.png", 704, 103},
		 {"filament/bay3_shade.png", 704, 105},
		 {"filament/bay3_empty.png", 704, 103},
		 {"filament/bay3_glow.png", 674, 241},
		 {693, 260, 148, 173}},
	};

	/* Rows on a screen, relative to its corner */
	constexpr Rect ROW_TOP = {0, 6, 148, 42};
	constexpr Rect ROW_MATERIAL = {8, 56, 132, 45};
	constexpr Rect ROW_BUTTON = {8, 118, 132, 48};

	/* "< 1 / 2 >" plate on the top rail; wooden arrows under the desk (more than one page) */
	constexpr Sprite PLATE = {"filament/plate.png", 414, 28};
	constexpr Rect PLATE_FACE = {416, 29, 192, 31};
	constexpr Sprite ARROW_NEXT = {"filament/arrow_next.png", 3, 511}; /* points left */
	constexpr Rect ARROW_NEXT_TAP = {4, 522, 98, 62};
	constexpr Sprite ARROW_BACK = {"filament/arrow_back.png", 920, 511}; /* points right */
	constexpr Rect ARROW_BACK_TAP = {922, 522, 98, 62};

	/* Wall: the fire bell (emergency stop) and the house picture (home) */
	constexpr Rect ESTOP = {0, 0, 100, 96};
	constexpr Rect HOME = {900, 20, 120, 110};
} // namespace UI::FilamentLayout
