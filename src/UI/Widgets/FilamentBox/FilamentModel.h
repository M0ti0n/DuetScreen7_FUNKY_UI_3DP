/*
 * FilamentModel.h
 *
 * The filament page's logic without LVGL (the tests use it directly), as the Modern UI
 * FILAMENT page:
 *
 *   PANEL mode (default)  information only: the screen remembers each slot's material and
 *                         colour; the material row picks both; the bottom button is EXTRUDE
 *                         (opens the extruder controls).
 *   MACROS mode           RRF's filament system: the material is what M701 loaded; the material
 *                         row picks the colour only; the bottom button is CHANGE / LOAD:
 *                         material popup -> colour popup -> T<n> / M702 / M701 S"<name>".
 *
 * One slot per tool, 4 slots a page (one MMU box). Colours are kept by the screen in both modes.
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace UI::FilamentModel
{
	constexpr size_t SLOTS_PER_PAGE = 4;
	constexpr size_t MAX_SLOTS = 16;
	constexpr int NONE = -1;

	constexpr std::array<const char*, 24> MATERIALS = {
		"PLA",	   "PETG",	  "ABS",	"ASA",	 "TPU", "PA",	 "PLA+", "ASA+", "ABS+", "PLA R", "PETG R", "TPU L",
		"PETG-CF", "PETG-GF", "ABS-GF", "PA-CF", "PC",	"PC-CF", "PCTG", "TPE",	 "PP",	 "PVB",	  "HIPS",	"PVA"};
	/* 64 colours, 8 a popup page (Modern UI's palette); 15 = natural / clear */
	constexpr std::array<uint32_t, 64> PALETTE = {
		0x000000, 0xF5F5F5, 0x808080, 0xC0C0C4, 0xD62828, 0xF07814, 0xFAD71E, 0xD4AF37, 0x28A03C, 0x145A28, 0x6EB4E6,
		0x1E50C8, 0x783CAA, 0xF082B4, 0x784B28, 0x1C222B, 0x28282C, 0x46464A, 0x69696E, 0xA0A0A5, 0xD2D2D4, 0x828C96,
		0xF0EBD7, 0xD7C3A0, 0x78141E, 0xAA1428, 0xFF6E5A, 0xFA9682, 0xC81E8C, 0xFF3C96, 0xFFBED2, 0xBE5A6E, 0xDC500A,
		0xFFB482, 0xFFAA00, 0xFFF05A, 0xC8A028, 0xC8AA78, 0x502D19, 0xB87333, 0xAAE628, 0x5ABE3C, 0x6E7828, 0x96E6BE,
		0x1E4628, 0x008282, 0x78825A, 0x32AA82, 0x141E50, 0x325ADC, 0x82C8F0, 0x00C8E6, 0x3CD2C8, 0x466E96, 0x0046AA,
		0xBEE1F5, 0x9650DC, 0xBEA0E6, 0x6E285A, 0x3C2878, 0xD2AADC, 0xC8FF50, 0x505A28, 0xFFFFC8};
	constexpr int NATURAL = 15;

	enum class Mode : uint8_t
	{
		Panel,
		Macros,
	};
	Mode GetMode();
	void SetMode(Mode mode);
	const char* ModeName(Mode mode); /* PANEL / MACROS */

	/* Kept per tool number by the screen */
	int SlotMaterial(size_t tool); /* index in MATERIALS or NONE (PANEL mode) */
	void SetSlotMaterial(size_t tool, int material);
	int SlotColour(size_t tool); /* index in PALETTE or NONE */
	void SetSlotColour(size_t tool, int colour);
	/* The colour a spool of this palette entry is painted with (natural: a light clear tone) */
	uint32_t SpoolColour(int colour);

	int FindMaterial(const std::string& name); /* index in MATERIALS, or NONE */

	/* MACROS mode: the materials there are folders for (built-in order first, then the others) */
	std::vector<std::string> MacroMaterials(const std::vector<std::string>& folders);

	size_t PageCount(size_t tools);

	/* G-code for CHANGE / LOAD (MACROS mode): select the tool, unload what is in it, load */
	std::vector<std::string> ChangeCommands(int tool, int currentTool, bool loaded, const std::string& material);
	std::vector<std::string> UnloadCommands(int tool, int currentTool);

} // namespace UI::FilamentModel
