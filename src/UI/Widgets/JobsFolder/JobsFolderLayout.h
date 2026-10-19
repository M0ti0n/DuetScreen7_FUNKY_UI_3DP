/*
 * JobsFolderLayout.h
 *
 * Positions on the jobs folder (1024 x 600), drawn by art/jobs/. The STOP sticker, the HOME
 * note, the side cards and the drawers are where they are on the object cancel folder.
 */

#pragma once

#include "UI/Widgets/ObjectFolder/ObjectFolderLayout.h"
#include <cstddef>
#include <cstdint>

namespace UI::JobsFolderLayout
{
	using Rect = ObjectFolderLayout::Rect;

	constexpr Rect ESTOP = ObjectFolderLayout::ESTOP;
	constexpr Rect HOME = ObjectFolderLayout::HOME;
	constexpr Rect PAGE_UP = ObjectFolderLayout::PAGE_UP;
	constexpr Rect SORT = {924, 262, 82, 64};
	constexpr Rect PAGE_DOWN = ObjectFolderLayout::PAGE_DOWN;
	constexpr int SORT_VALUE_Y = 304; /* centre of NEW / A-Z / FILA */

	/* Left page: details */
	constexpr int PHOTO_X = 87, PHOTO_Y = 10;	/* photo.png */
	constexpr Rect THUMB = {129, 52, 234, 234}; /* the window in the photo frame */
	constexpr int NAME_CX = 246, NAME_Y = 312, NAME_W = 360;
	constexpr int INFO_KEY_X = 70, INFO_VALUE_X = 172, INFO_Y = 342, INFO_STEP = 19;
	constexpr int STAMP_X = 300, STAMP_Y = 346; /* stamp.png, 140 x 90 */
	constexpr Rect STAMP_TAP = {304, 350, 132, 78};
	constexpr int PICK_CX = 246, PICK_Y = 240;

	/* Right page: header and rows */
	constexpr Rect HEADER = {496, 50, 368, 44};
	constexpr Rect SORT_UP = {610, 54, 48, 36};
	constexpr Rect SORT_DOWN = {662, 54, 48, 36};
	constexpr int PAGES_RIGHT = 850;
	constexpr size_t ROWS = 8;
	constexpr int ROW_Y = 108, ROW_STEP = 43, ROW_H = 38;
	constexpr int ROW_X = 496, ROW_W = 368;
	constexpr int NAME_X = 510, FOLDER_NAME_X = 540, AGE_RIGHT = 854;
	constexpr int EMPTY_CX = 680, EMPTY_Y = 260;
} // namespace UI::JobsFolderLayout
