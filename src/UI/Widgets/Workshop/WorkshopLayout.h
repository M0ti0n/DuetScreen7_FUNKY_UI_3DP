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
		Tools,		   // temperature / tools
		Motion,		   // move
		Extruder,	   // extruder controls (temperature tab)
		Tune,		   // fine tune (dashboard status tab)
		Macros,		   // macro files
		Filament,	   // spool pile
		BedLevelling,  // wall bed with the "LEVEL !!!" note
		Console,	   // laptop
		PrintStatus,   // printer
		Jobs,		   // job folder rack
		ObjectCancel,  // spare bed on the table
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

	/* Print on the printer's bed, revealed from the bed upward with progress */
	constexpr LayerDef PRINT_PART = {"layers/print_part.png", 576, 223};
	constexpr int PRINT_PART_W = 197;
	constexpr int PRINT_PART_H = 127;

	/* Finished parts on the spare bed (random 1-4 per job) */
	constexpr LayerDef SPARE_BED_PARTS[] = {
		{"layers/bed_boat.png", 869, 418},
		{"layers/bed_spinner.png", 921, 427},
		{"layers/bed_bar.png", 941, 437},
		{"layers/bed_cube.png", 965, 419},
	};

	/* The printer's own little display. Quad on the scene:
	 * TL(593,380) TR(711,383) BR(702,439) BL(585,435) */
	constexpr Rect PRINTER_DISPLAY = {589, 382, 118, 54};
	constexpr int PRINTER_DISPLAY_ROTATION = 15; /* 0.1 degree units, clockwise */
} // namespace UI::WorkshopLayout
