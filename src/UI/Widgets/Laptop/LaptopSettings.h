/*
 * LaptopSettings.h
 *
 * The screen settings of the laptop's SETTINGS page, kept by DuetScreen (StorageHelper,
 * /etc/duetscreen.json). Ported from the DuetScreen7 CNC UI (CncPanel).
 *
 *   BRIGHTNESS        10 / 20 / 40 / 60 / 80 / 100 %   sys:brightness (DuetScreen's backlight)
 *   SOUND             OFF / BUZZER / SPEAKER 1-5       funky:sound
 *   SCREEN DIMMING    OFF / 1 / 5 / 10 MIN             ui:screensaver_timeout (DuetScreen's)
 *   ORIENTATION       NORMAL / UPSIDE DOWN             ui:display_rotation (0 / 180 degrees)
 *   CONNECTION        UART / USB / NETWORK             DuetScreen's own setting
 *   DRAWER INDICATOR  ON / OFF                         funky:drawer_indicator
 *   PRINTER           AUTO / COREXY / DELTA / BED SLINGER  funky:printer_style (PrinterStyle.h)
 *   FILAMENT          PANEL / MACROS                   funky:filament_mode (FilamentModel.h)
 *
 * Sound, as on PanelDue: a short click on every touch, and the printer's M300 beeps. The
 * buzzer is played with `beep`, the speaker with `aplay` (a generated tone) after `amixer`
 * sets its volume; a missing program just stays quiet.
 */

#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace UI::LaptopSettings
{
	void Init(); /* touch click and the printer's beeps; called once from main() */

	unsigned int Brightness();
	void SetBrightness(unsigned int percent);
	unsigned int DimTimeout(); /* seconds, 0 = never */
	void SetDimTimeout(unsigned int seconds);
	bool UpsideDown();
	void SetUpsideDown(bool upsideDown);

	unsigned int SoundSetting(); /* 0 off, 1 buzzer, 2..6 speaker volume 1..5 */
	void SetSoundSetting(unsigned int value);
	void Click();
	void Beep(unsigned int frequency, unsigned int ms);

	enum class Link : uint8_t
	{
		Uart,
		Usb,
		Network,
	};
	Link Connection();
	std::string DuetIp();
	void SetConnection(Link link, const std::string& ip = std::string());

	bool DrawerIndicator();
	void SetDrawerIndicator(bool on);

	/* FACTORY RESET (screen settings; the connection is kept) */
	void FactoryReset();

	/* Tests: commands go to the hook instead (nothing is run, nothing touches the display) */
	void SetTestHook(std::function<void(const std::string&)> hook);
} // namespace UI::LaptopSettings
