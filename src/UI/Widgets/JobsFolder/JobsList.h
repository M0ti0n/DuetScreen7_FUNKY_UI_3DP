/*
 * JobsList.h
 *
 * The job list of the jobs folder, without LVGL (the tests use it directly): sorting,
 * the material read from the file name and the age text.
 *
 *   NEW   by date; down = newest on top (default)
 *   A-Z   by name; up = A on top (default)
 *   FILA  by material, then name; up = PLA group first (default). The material is the last
 *         "_" part of the name, as the slicers' output name template writes it, e.g. OrcaSlicer
 *         {input_filename_base}_{filament_type[initial_tool]}.gcode -> part_PETG.gcode.
 *         Files without a known material go last.
 * Folders always come first.
 */

#pragma once

#include <cstdint>
#include <ctime>
#include <string>
#include <vector>

namespace UI::JobsList
{
	enum class Sort : uint8_t
	{
		Newest,
		Name,
		Filament,
	};

	struct Entry
	{
		std::string name;
		std::string path;
		bool folder = false;
		bool up = false;  /* the ".." row back to the parent folder */
		std::string date; /* "2026-10-18T14:02:00" */
		std::string age;  /* "2h ago" */
	};

	std::string MaterialOf(const std::string& name); /* "" when not known */
	void SortEntries(std::vector<Entry>& entries, Sort sort, bool ascending);
	bool DefaultAscending(Sort sort);
	const char* SortName(Sort sort);
	Sort NextSort(Sort sort);
	/* "2h ago" from a "YYYY-MM-DDTHH:MM:SS" date (local time) */
	std::string AgeText(const std::string& date, std::time_t now);
} // namespace UI::JobsList
