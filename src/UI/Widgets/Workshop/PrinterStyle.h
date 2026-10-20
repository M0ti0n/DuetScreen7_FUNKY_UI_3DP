/*
 * PrinterStyle.h
 *
 * Which printer stands on the workshop desk (laptop SETTINGS > PERSONALIZATION > PRINTER):
 * AUTO picks the delta for delta kinematics and the CoreXY otherwise. The delta also brings
 * round beds (wall, spare bed, the object cancel page).
 */

#pragma once

#include <cstdint>

namespace UI::PrinterStyle
{
	enum class Style : uint8_t
	{
		CoreXY,
		Delta,
		BedSlinger,
	};

	enum class Setting : uint8_t
	{
		Auto,
		CoreXY,
		Delta,
		BedSlinger,
	};

	Setting GetSetting();
	void SetSetting(Setting setting);
	const char* SettingName(Setting setting); /* AUTO / COREXY / DELTA / BED SLINGER */

	/* The style in use: the setting, or the printer's kinematics for AUTO */
	Style Current();
	inline bool RoundBed()
	{
		return Current() == Style::Delta;
	}
} // namespace UI::PrinterStyle
