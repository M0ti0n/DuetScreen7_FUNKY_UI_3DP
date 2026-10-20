/*
 * PrinterStyle.cpp
 */

#include "PrinterStyle.h"
#include "ObjectModel/Axis.h"
#include "Storage.h"
#include "utils/StorageHelper.h"

namespace UI::PrinterStyle
{
	namespace
	{
		constexpr StorageKey<unsigned int> ID_FUNKY_PRINTER = {"funky:printer_style", 0u};
	}

	Setting GetSetting()
	{
		const unsigned int v = StorageHelper::getData(ID_FUNKY_PRINTER);
		return v <= static_cast<unsigned int>(Setting::BedSlinger) ? static_cast<Setting>(v) : Setting::Auto;
	}

	void SetSetting(Setting setting)
	{
		StorageHelper::setData(ID_FUNKY_PRINTER, static_cast<unsigned int>(setting));
	}

	const char* SettingName(Setting setting)
	{
		switch (setting)
		{
		case Setting::Auto:
			return "AUTO";
		case Setting::CoreXY:
			return "COREXY";
		case Setting::Delta:
			return "DELTA";
		case Setting::BedSlinger:
			return "BED SLINGER";
		}
		return "";
	}

	Style Current()
	{
		switch (GetSetting())
		{
		case Setting::CoreXY:
			return Style::CoreXY;
		case Setting::Delta:
			return Style::Delta;
		case Setting::BedSlinger:
			return Style::BedSlinger;
		case Setting::Auto:
			break;
		}
		return OM::Move::GetKinematics().IsDelta() ? Style::Delta : Style::CoreXY;
	}
} // namespace UI::PrinterStyle
