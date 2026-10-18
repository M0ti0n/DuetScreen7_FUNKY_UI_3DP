/*
 * LaptopLayout.h
 *
 * Positions of the laptop page (1024 x 600). The art is made by the same script that
 * produced the mock-ups (art/laptop/), and the key rectangles below come from it.
 *
 * Screen up:      LCD (46,24)-(978,506), side column, page content.
 * Keyboard open:  the screen slides down to (46,18)-(978,222); only the bottom of the
 *                 page shows (the side column is pushed off the top and is inactive),
 *                 the keyboard fills the rest.
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace UI::LaptopLayout
{
	struct Rect
	{
		int16_t x, y, w, h;
	};

	/* LCD in the two states (screen coordinates) */
	constexpr Rect LCD_UP = {46, 24, 932, 482};
	constexpr Rect LCD_KEYBOARD = {46, 18, 932, 204};

	/* Page content, in LCD coordinates (the page is always LCD_UP sized) */
	constexpr int MARGIN = 14;
	constexpr int SIDE_X = 14;
	constexpr int SIDE_W = 168;
	constexpr int SIDE_GAP = 10;
	constexpr int CONTENT_X = SIDE_X + SIDE_W + 16; /* 198 */
	constexpr int CONTENT_R = LCD_UP.w - MARGIN;	/* 918 */

	/* Right column of ALERTS and SETTINGS: up / middle / down */
	constexpr int PAGER_W = 74;
	constexpr int PAGER_GAP = 10;
	constexpr int PAGER_X = CONTENT_R - PAGER_W; /* 844 */

	/* CONSOLE */
	constexpr int CONSOLE_INPUT_Y = 424;
	constexpr int CONSOLE_INPUT_H = 44;
	constexpr int CONSOLE_KB_BTN_W = 60;
	constexpr int CONSOLE_LINE_H = 30;
	constexpr int CONSOLE_AGE_X = CONTENT_X + 6;
	constexpr int CONSOLE_TEXT_X = CONTENT_X + 92;
	constexpr int CONSOLE_LOG_BOTTOM = CONSOLE_INPUT_Y - 12; /* bottom of the newest line */
	constexpr size_t CONSOLE_LINES = 13;

	/* ALERTS */
	constexpr int ALERT_ROWS = 7;
	constexpr int ALERT_ROW_H = 58;
	constexpr int ALERT_ROW_GAP = 8;

	/* SETTINGS */
	constexpr size_t SETTING_COLS = 3;
	constexpr size_t SETTING_ROWS = 3;
	constexpr int SETTING_TILE_H = 92;
	constexpr int SETTING_GAP_X = 14;
	constexpr int SETTING_GAP_Y = 16;
	constexpr int SETTING_TOP = 50;

	/* Keyboard */
	constexpr Rect KEYBOARD = {34, 262, 956, 330}; /* keys_shift.png covers this */

	enum class KeyKind : uint8_t
	{
		Char,
		Backspace,
		Up,		/* history: older */
		Down,	/* history: newer */
		Left,	/* cursor */
		Right,
		Shift,
		Space,
		Hide,
		Enter, /* L-shaped: two tap zones */
	};

	struct KeyDef
	{
		KeyKind kind;
		char normal;
		char shifted;
		Rect rect;
	};

	constexpr KeyDef KEYS[] = {
		{KeyKind::Char, '1', '!', {34, 262, 65, 60}},
		{KeyKind::Char, '2', '"', {106, 262, 65, 60}},
		{KeyKind::Char, '3', '#', {177, 262, 65, 60}},
		{KeyKind::Char, '4', '$', {249, 262, 65, 60}},
		{KeyKind::Char, '5', '%', {320, 262, 65, 60}},
		{KeyKind::Char, '6', '^', {392, 262, 65, 60}},
		{KeyKind::Char, '7', '&', {464, 262, 65, 60}},
		{KeyKind::Char, '8', '*', {535, 262, 65, 60}},
		{KeyKind::Char, '9', '(', {607, 262, 65, 60}},
		{KeyKind::Char, '0', ')', {678, 262, 65, 60}},
		{KeyKind::Char, '-', '_', {750, 262, 65, 60}},
		{KeyKind::Char, '+', '=', {822, 262, 65, 60}},
		{KeyKind::Backspace, 0, 0, {893, 262, 97, 60}},
		{KeyKind::Char, 'Q', 'q', {66, 329, 65, 60}},
		{KeyKind::Char, 'W', 'w', {138, 329, 65, 60}},
		{KeyKind::Char, 'E', 'e', {209, 329, 65, 60}},
		{KeyKind::Char, 'R', 'r', {281, 329, 65, 60}},
		{KeyKind::Char, 'T', 't', {353, 329, 65, 60}},
		{KeyKind::Char, 'Y', 'y', {424, 329, 65, 60}},
		{KeyKind::Char, 'U', 'u', {496, 329, 65, 60}},
		{KeyKind::Char, 'I', 'i', {567, 329, 65, 60}},
		{KeyKind::Char, 'O', 'o', {639, 329, 65, 60}},
		{KeyKind::Char, 'P', 'p', {711, 329, 65, 60}},
		{KeyKind::Char, '[', '{', {782, 329, 65, 60}},
		{KeyKind::Char, ']', '}', {854, 329, 65, 60}},
		{KeyKind::Char, 'A', 'a', {82, 397, 65, 60}},
		{KeyKind::Char, 'S', 's', {154, 397, 65, 60}},
		{KeyKind::Char, 'D', 'd', {226, 397, 65, 60}},
		{KeyKind::Char, 'F', 'f', {297, 397, 65, 60}},
		{KeyKind::Char, 'G', 'g', {369, 397, 65, 60}},
		{KeyKind::Char, 'H', 'h', {440, 397, 65, 60}},
		{KeyKind::Char, 'J', 'j', {512, 397, 65, 60}},
		{KeyKind::Char, 'K', 'k', {584, 397, 65, 60}},
		{KeyKind::Char, 'L', 'l', {655, 397, 65, 60}},
		{KeyKind::Char, ':', ';', {727, 397, 65, 60}},
		{KeyKind::Char, '@', '\'', {798, 397, 65, 60}},
		{KeyKind::Enter, 0, 0, {925, 329, 65, 60}},
		{KeyKind::Enter, 0, 0, {870, 397, 120, 60}},
		{KeyKind::Char, 'Z', 'z', {99, 464, 65, 60}},
		{KeyKind::Char, 'X', 'x', {170, 464, 65, 60}},
		{KeyKind::Char, 'C', 'c', {242, 464, 65, 60}},
		{KeyKind::Char, 'V', 'v', {313, 464, 65, 60}},
		{KeyKind::Char, 'B', 'b', {385, 464, 65, 60}},
		{KeyKind::Char, 'N', 'n', {457, 464, 65, 60}},
		{KeyKind::Char, 'M', 'm', {528, 464, 65, 60}},
		{KeyKind::Char, ',', '<', {600, 464, 65, 60}},
		{KeyKind::Char, '.', '>', {671, 464, 65, 60}},
		{KeyKind::Char, '/', '?', {743, 464, 65, 60}},
		{KeyKind::Up, 0, 0, {868, 464, 58, 60}},
		{KeyKind::Shift, 0, 0, {34, 532, 152, 60}},
		{KeyKind::Space, 0, 0, {193, 532, 485, 60}},
		{KeyKind::Hide, 0, 0, {685, 532, 111, 60}},
		{KeyKind::Left, 0, 0, {803, 532, 58, 60}},
		{KeyKind::Down, 0, 0, {868, 532, 58, 60}},
		{KeyKind::Right, 0, 0, {932, 532, 58, 60}},
	};
} // namespace UI::LaptopLayout
