/*
 * ObjectFolderLayout.h
 *
 * Positions on the object cancel page (1024 x 600), drawn by art/objects/.
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace UI::ObjectFolderLayout
{
	struct Rect
	{
		int16_t x, y, w, h;
	};

	constexpr Rect ESTOP = {0, 0, 98, 98};		  /* the red sticker in the corner */
	constexpr Rect HOME = {912, 6, 102, 98};	  /* the yellow sticky note */
	constexpr Rect PAGE_UP = {924, 112, 82, 140}; /* paper cards right of the folder */
	constexpr Rect PAGE_LABEL = {924, 266, 82, 56};
	constexpr Rect PAGE_DOWN = {924, 336, 82, 128};

	/* Left page: the bed between the pencil axes */
	constexpr Rect CANVAS = {92, 118, 338, 296};
	constexpr int MARKER = 40;	   /* sprite size */
	constexpr int MARKER_GAP = 36; /* markers closer than this are moved apart */

	/* Right page */
	constexpr int JOB_X = 552, JOB_Y = 72, JOB_W = 300;
	constexpr size_t ROWS = 5;
	constexpr int ROW_Y = 112, ROW_STEP = 68, ROW_H = 52;
	constexpr int NUM_X = 496, NUM_W = 56;
	constexpr int NAME_X = 564, NAME_W = 300;
	constexpr int EMPTY_X = 680, EMPTY_Y = 260;
} // namespace UI::ObjectFolderLayout
