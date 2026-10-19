/*
 * JobsList.cpp
 */

#include "JobsList.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <fmt/format.h>

namespace UI::JobsList
{
	static constexpr std::array<const char*, 9> MATERIALS = {
		"PLA", "PETG", "ABS", "ASA", "TPU", "PA", "PC", "PVA", "HIPS"};

	static std::string upper(std::string s)
	{
		for (char& c : s)
		{
			c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
		}
		return s;
	}

	static std::string lower(std::string s)
	{
		for (char& c : s)
		{
			c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		}
		return s;
	}

	std::string MaterialOf(const std::string& name)
	{
		std::string base = name;
		if (const size_t dot = base.rfind('.'); dot != std::string::npos)
		{
			base.resize(dot);
		}
		const size_t us = base.rfind('_');
		if (us == std::string::npos)
		{
			return "";
		}
		const std::string tag = upper(base.substr(us + 1));
		for (const char* m : MATERIALS)
		{
			if (tag == m)
			{
				return tag;
			}
		}
		return "";
	}

	static size_t materialRank(const std::string& name)
	{
		const std::string m = MaterialOf(name);
		for (size_t i = 0; i < MATERIALS.size(); i++)
		{
			if (m == MATERIALS[i])
			{
				return i;
			}
		}
		return MATERIALS.size();
	}

	void SortEntries(std::vector<Entry>& entries, Sort sort, bool ascending)
	{
		auto byName = [](const Entry& a, const Entry& b) { return lower(a.name) < lower(b.name); };
		std::stable_sort(entries.begin(),
						 entries.end(),
						 [&](const Entry& a, const Entry& b)
						 {
							 /* ".." first, then folders, then files */
							 if (a.up != b.up)
							 {
								 return a.up;
							 }
							 if (a.folder != b.folder)
							 {
								 return a.folder;
							 }
							 bool less = false, greater = false;
							 switch (a.folder ? Sort::Name : sort)
							 {
							 case Sort::Newest:
								 less = a.date < b.date;
								 greater = b.date < a.date;
								 break;
							 case Sort::Name:
								 less = byName(a, b);
								 greater = byName(b, a);
								 break;
							 case Sort::Filament:
							 {
								 const size_t ra = materialRank(a.name), rb = materialRank(b.name);
								 less = ra < rb || (ra == rb && byName(a, b));
								 greater = rb < ra || (ra == rb && byName(b, a));
								 break;
							 }
							 }
							 return (ascending || a.folder) ? less : greater;
						 });
	}

	bool DefaultAscending(Sort sort)
	{
		return sort != Sort::Newest;
	}

	const char* SortName(Sort sort)
	{
		switch (sort)
		{
		case Sort::Newest:
			return "NEW";
		case Sort::Name:
			return "A-Z";
		case Sort::Filament:
			return "FILA";
		}
		return "";
	}

	Sort NextSort(Sort sort)
	{
		switch (sort)
		{
		case Sort::Newest:
			return Sort::Name;
		case Sort::Name:
			return Sort::Filament;
		case Sort::Filament:
			return Sort::Newest;
		}
		return Sort::Newest;
	}

	std::string AgeText(const std::string& date, std::time_t now)
	{
		std::tm t{};
		if (std::sscanf(date.c_str(),
						"%d-%d-%dT%d:%d:%d",
						&t.tm_year,
						&t.tm_mon,
						&t.tm_mday,
						&t.tm_hour,
						&t.tm_min,
						&t.tm_sec) < 3)
		{
			return "";
		}
		t.tm_year -= 1900;
		t.tm_mon -= 1;
		t.tm_isdst = -1;
		const std::time_t then = std::mktime(&t);
		if (then == static_cast<std::time_t>(-1))
		{
			return "";
		}
		const long long s = std::max<long long>(0, static_cast<long long>(std::difftime(now, then)));
		if (s < 3600)
		{
			return fmt::format("{:d}m ago", s / 60);
		}
		if (s < 86400)
		{
			return fmt::format("{:d}h ago", s / 3600);
		}
		if (s < 7 * 86400)
		{
			return fmt::format("{:d}d ago", s / 86400);
		}
		if (s < 60 * 86400)
		{
			return fmt::format("{:d}w ago", s / (7 * 86400));
		}
		if (s < 365 * 86400)
		{
			return fmt::format("{:d}mo ago", s / (30 * 86400));
		}
		return fmt::format("{:d}y ago", s / (365 * 86400));
	}
} // namespace UI::JobsList
