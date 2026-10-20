/*
 * FilamentModel.cpp
 */

#include "FilamentModel.h"
#include "Storage.h"
#include "utils/StorageHelper.h"
#include <algorithm>
#include <fmt/format.h>

namespace UI::FilamentModel
{
	namespace
	{
		constexpr StorageKey<unsigned int> ID_FUNKY_FILAMENT_MODE = {"funky:filament_mode", 0u};
		constexpr unsigned int UNSET = 0xFFu;

		StorageKeyRunTime<unsigned int> key(const char* what, size_t tool)
		{
			return StorageKeyRunTime<unsigned int>{fmt::format("funky:filament_{:s}{:d}", what, tool), UNSET};
		}

		int read(const char* what, size_t tool, size_t count)
		{
			if (tool >= MAX_SLOTS)
			{
				return NONE;
			}
			const unsigned int v = StorageHelper::getData(key(what, tool));
			return v < count ? static_cast<int>(v) : NONE;
		}

		void write(const char* what, size_t tool, int value)
		{
			if (tool < MAX_SLOTS)
			{
				StorageHelper::setData(key(what, tool), value < 0 ? UNSET : static_cast<unsigned int>(value));
			}
		}
	} // namespace

	Mode GetMode()
	{
		return StorageHelper::getData(ID_FUNKY_FILAMENT_MODE) == 1u ? Mode::Macros : Mode::Panel;
	}

	void SetMode(Mode mode)
	{
		StorageHelper::setData(ID_FUNKY_FILAMENT_MODE, mode == Mode::Macros ? 1u : 0u);
	}

	const char* ModeName(Mode mode)
	{
		return mode == Mode::Macros ? "MACROS" : "PANEL";
	}

	int SlotMaterial(size_t tool)
	{
		return read("material", tool, MATERIALS.size());
	}

	void SetSlotMaterial(size_t tool, int material)
	{
		write("material", tool, material);
	}

	int SlotColour(size_t tool)
	{
		return read("colour", tool, PALETTE.size());
	}

	void SetSlotColour(size_t tool, int colour)
	{
		write("colour", tool, colour);
	}

	uint32_t SpoolColour(int colour)
	{
		if (colour < 0 || static_cast<size_t>(colour) >= PALETTE.size())
		{
			return 0xC8C8C4; /* no colour chosen: light grey */
		}
		return colour == NATURAL ? 0xE6E2D6 : PALETTE[static_cast<size_t>(colour)];
	}

	int FindMaterial(const std::string& name)
	{
		for (size_t i = 0; i < MATERIALS.size(); i++)
		{
			if (name == MATERIALS[i])
			{
				return static_cast<int>(i);
			}
		}
		return NONE;
	}

	std::vector<std::string> MacroMaterials(const std::vector<std::string>& folders)
	{
		std::vector<std::string> out;
		for (const char* m : MATERIALS)
		{
			if (std::find(folders.begin(), folders.end(), m) != folders.end())
			{
				out.emplace_back(m);
			}
		}
		for (const std::string& f : folders)
		{
			if (FindMaterial(f) == NONE)
			{
				out.push_back(f);
			}
		}
		return out;
	}

	size_t PageCount(size_t tools)
	{
		return tools == 0 ? 1 : (tools + SLOTS_PER_PAGE - 1) / SLOTS_PER_PAGE;
	}

	std::vector<std::string> ChangeCommands(int tool, int currentTool, bool loaded, const std::string& material)
	{
		std::vector<std::string> out;
		if (tool != currentTool)
		{
			out.push_back(fmt::format("T{:d}", tool));
		}
		if (loaded)
		{
			out.emplace_back("M702");
		}
		out.push_back(fmt::format("M701 S\"{:s}\"", material));
		return out;
	}

	std::vector<std::string> UnloadCommands(int tool, int currentTool)
	{
		std::vector<std::string> out;
		if (tool != currentTool)
		{
			out.push_back(fmt::format("T{:d}", tool));
		}
		out.emplace_back("M702");
		return out;
	}
} // namespace UI::FilamentModel
