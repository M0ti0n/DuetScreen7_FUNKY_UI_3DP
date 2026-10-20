/*
 * WorkshopLayout.h
 *
 * Positions on the 1024 x 600 workshop home scene (source: art/layout.json).
 * Tap zones were marked by hand on the idle scene.
 */

#pragma once

#include <cstdint>

namespace UI::WorkshopLayout
{
	enum class Target : uint8_t
	{
		Tools,		  // temperature / tools
		Motion,		  // move
		Extruder,	  // extruder controls (temperature tab)
		Tune,		  // fine tune (dashboard status tab)
		Macros,		  // macro files
		Filament,	  // spool pile
		BedLevelling, // wall bed with the "LEVEL !!!" note
		Console,	  // laptop
		PrintStatus,  // printer
		Jobs,		  // job folder rack
		ObjectCancel, // spare bed on the table
	};

	struct Rect
	{
		int16_t x, y, w, h;
	};

	struct DrawerDef
	{
		const char* id;
		Target target;
		Rect tap;
		Rect sprite;
	};

	struct HotspotDef
	{
		const char* id;
		Target target;
		Rect tap;
	};

	struct LayerDef
	{
		const char* file;
		int16_t x, y;
	};

	constexpr int SCREEN_W = 1024;
	constexpr int SCREEN_H = 600;

	constexpr int DRAWER_FRAMES = 7;
	constexpr uint32_t DRAWER_ANIM_MS = 260;

	constexpr DrawerDef DRAWERS[] = {
		{"tools", Target::Tools, {100, 509, 167, 86}, {93, 512, 180, 88}},
		{"motion", Target::Motion, {274, 507, 155, 90}, {258, 512, 180, 88}},
		{"extruder", Target::Extruder, {434, 504, 158, 93}, {423, 512, 180, 88}},
		{"tune", Target::Tune, {595, 505, 164, 91}, {587, 512, 181, 88}},
		{"macros", Target::Macros, {761, 505, 167, 89}, {752, 512, 181, 88}},
	};

	constexpr HotspotDef HOTSPOTS[] = {
		{"filament", Target::Filament, {1, 134, 185, 305}},
		{"bed_levelling", Target::BedLevelling, {250, 13, 208, 181}},
		{"console", Target::Console, {218, 216, 275, 267}},
		{"print_status", Target::PrintStatus, {507, 71, 293, 385}},
		{"jobs", Target::Jobs, {884, 24, 140, 354}},
		{"object_cancel", Target::ObjectCancel, {808, 395, 212, 101}},
	};

	/* Fire alarm = emergency stop (bell drawn 20 % bigger, tap zone grown with it) */
	constexpr Rect ESTOP = {0, 0, 150, 130};

	/* Scene layers (art/printers/bake.py): the base has no printer and no beds */
	constexpr const char* BASE = "home_base.png";
	constexpr LayerDef WALL_BED_SQUARE = {"scene/wall_bed_square.png", 257, 21};
	constexpr LayerDef WALL_BED_ROUND = {"scene/wall_bed_round.png", 285, 21};
	constexpr LayerDef SPARE_BED_SQUARE = {"scene/spare_bed_square.png", 796, 381};
	constexpr LayerDef SPARE_BED_ROUND = {"scene/spare_bed_round.png", 804, 387};
	constexpr LayerDef FOLDER_SQUARE = {
		"layers/folder_square.png", 832, 426}; /* under the finished parts while printing */
	constexpr LayerDef FOLDER_ROUND = {"layers/folder_round.png", 852, 431};

	/* Printer stickers (PrinterStyle::Style order). part: the print on its bed
	 * (printers/<id>_part / _ghost / _outline.png); display: the glass the app draws on. */
	struct PrinterDef
	{
		const char* id;
		LayerDef printer;
		Rect part;
		Rect display;			 /* where the text goes */
		int16_t displayRotation; /* 0.1 degree units, clockwise */
		LayerDef glass;			 /* the screen's glass, white: the app tints it */
	};
	constexpr PrinterDef PRINTERS[] = {
		{"corexy",
		 {"printers/corexy.png", 490, 50},
		 {576, 223, 197, 127},
		 {589, 382, 118, 54},
		 15,
		 {"printers/corexy_glass.png", 583, 377}},
		{"delta",
		 {"printers/delta.png", 489, 28},
		 {629, 244, 115, 74},
		 {615, 371, 123, 66},
		 0,
		 {"printers/delta_glass.png", 604, 367}},
		{"bedslinger",
		 {"printers/bedslinger.png", 504, 91},
		 {604, 272, 130, 84},
		 {567, 386, 118, 54},
		 15,
		 {"printers/bedslinger_glass.png", 560, 380}},
	};
	constexpr int PRINT_STAGES = 10; /* the print grows in steps of 10 % */

	/* Finished parts on the spare bed (random 1-4 per job) */
	constexpr LayerDef SPARE_BED_PARTS[] = {
		{"layers/bed_boat.png", 869, 418},
		{"layers/bed_spinner.png", 921, 427},
		{"layers/bed_bar.png", 941, 437},
		{"layers/bed_cube.png", 965, 419},
	};

} // namespace UI::WorkshopLayout
