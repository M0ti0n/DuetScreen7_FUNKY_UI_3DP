/*
 * LaptopSettings.cpp
 */

#include "LaptopSettings.h"
#include "UI/Widgets/FilamentBox/FilamentModel.h"
#include "UI/Widgets/Workshop/PrinterStyle.h"
#include "Configuration.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "Storage.h"
#include "Subscribers/Subscribers.h"
#include "lvgl/lvgl.h"
#include "utils/DisplayHelper.h"
#include "utils/StorageHelper.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <fmt/format.h>
#include <fstream>
#include <vector>

namespace UI::LaptopSettings
{
	namespace
	{
		constexpr StorageKey<unsigned int> ID_FUNKY_SOUND = {"funky:sound", 1u};
		constexpr StorageKey<bool> ID_FUNKY_DRAWER_INDICATOR = {"funky:drawer_indicator", true};

		constexpr unsigned int CLICK_FREQUENCY = 3000, CLICK_MS = 12;

		std::function<void(const std::string&)> s_hook;
		bool s_inited = false;
		unsigned int s_speakerVolumeSet = 0; /* volume last set with amixer (0 = not yet) */
		uint32_t s_lastBeepTime = 0;
		unsigned int s_lastBeepFreq = 0, s_lastBeepMs = 0;

		/* M300 from the receive thread, played by the UI thread */
		std::atomic<int32_t> s_beepDuration{0};
		std::atomic<int32_t> s_beepFrequency{0};
		std::atomic<bool> s_beepPending{false};

		void run(const std::string& cmd)
		{
			if (s_hook)
			{
				s_hook(cmd);
				return;
			}
			/* in the background: the UI never waits for a sound */
			const int rc = std::system((cmd + " >/dev/null 2>&1 &").c_str());
			(void)rc;
		}

		constexpr size_t MAX_TONE_FILES = 16;
		std::deque<std::string> s_toneFiles;

		/* A sine tone as a WAV file in /tmp (made once per tone) */
		std::string toneFile(unsigned int frequency, unsigned int ms)
		{
			const std::string path = fmt::format("/tmp/funky-tone-{:d}-{:d}.wav", frequency, ms);
			if (s_hook)
			{
				return path;
			}
			if (std::ifstream(path).good())
			{
				return path;
			}
			s_toneFiles.push_back(path);
			if (s_toneFiles.size() > MAX_TONE_FILES)
			{
				std::remove(s_toneFiles.front().c_str());
				s_toneFiles.pop_front();
			}
			constexpr uint32_t RATE = 22050;
			const uint32_t n = RATE * ms / 1000;
			std::vector<int16_t> samples(n);
			for (uint32_t i = 0; i < n; ++i)
			{
				const double env = std::min(1.0, static_cast<double>(std::min(i, n - i)) / (RATE * 0.002));
				samples[i] = static_cast<int16_t>(12000.0 * env *
												  std::sin(2.0 * 3.14159265 * static_cast<double>(frequency) *
														   static_cast<double>(i) / static_cast<double>(RATE)));
			}
			std::ofstream f(path, std::ios::binary);
			auto u32 = [&](uint32_t v) { f.write(reinterpret_cast<const char*>(&v), 4); };
			auto u16 = [&](uint16_t v) { f.write(reinterpret_cast<const char*>(&v), 2); };
			f.write("RIFF", 4);
			u32(36 + n * 2);
			f.write("WAVEfmt ", 8);
			u32(16);
			u16(1);
			u16(1);
			u32(RATE);
			u32(RATE * 2);
			u16(2);
			u16(16);
			f.write("data", 4);
			u32(n * 2);
			f.write(reinterpret_cast<const char*>(samples.data()), static_cast<std::streamsize>(n * 2));
			return path;
		}

		bool beepDuration(Comm::JsonDecoder*, const int32_t& val, const size_t[])
		{
			s_beepDuration = val;
			return true;
		}

		bool beepFrequency(Comm::JsonDecoder*, const int32_t& val, const size_t[])
		{
			s_beepFrequency = val;
			if (val > 0 && s_beepDuration > 0)
			{
				s_beepPending = true;
			}
			return true;
		}
	} // namespace

	void Init()
	{
		if (s_inited)
		{
			return;
		}
		s_inited = true;
		/* A click on every touch, wherever it lands */
		for (lv_indev_t* indev = lv_indev_get_next(nullptr); indev != nullptr; indev = lv_indev_get_next(indev))
		{
			if (lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER)
			{
				lv_indev_add_event_cb(indev, [](lv_event_t*) { Click(); }, LV_EVENT_PRESSED, nullptr);
			}
		}
		SubscriberMap::addSubscriber("state:beep:duration", &beepDuration);
		SubscriberMap::addSubscriber("state:beep:frequency", &beepFrequency);
		lv_timer_create(
			[](lv_timer_t*)
			{
				if (s_beepPending.exchange(false))
				{
					Beep(static_cast<unsigned int>(s_beepFrequency.load()),
						 static_cast<unsigned int>(s_beepDuration.load()));
				}
			},
			50,
			nullptr);
	}

	/* ---------------- screen ---------------- */

	unsigned int Brightness()
	{
		return StorageHelper::getData(ID_SYS_BRIGHTNESS_KEY);
	}

	void SetBrightness(unsigned int percent)
	{
		if (s_hook)
		{
			StorageHelper::setData(ID_SYS_BRIGHTNESS_KEY, percent);
			s_hook(fmt::format("brightness {:d}", percent));
			return;
		}
		DisplayHelper::setBrightness(percent); /* stores it too */
	}

	unsigned int DimTimeout()
	{
		return static_cast<unsigned int>(StorageHelper::getData(ID_SCREENSAVER_TIMEOUT).count());
	}

	void SetDimTimeout(unsigned int seconds)
	{
		StorageHelper::setData(ID_SCREENSAVER_TIMEOUT, std::chrono::seconds(seconds));
	}

	bool UpsideDown()
	{
		return StorageHelper::getData(ID_DISPLAY_ROTATION) == DisplayRotation::ROTATION_180;
	}

	void SetUpsideDown(bool upsideDown)
	{
		const DisplayRotation r = upsideDown ? DisplayRotation::ROTATION_180 : DisplayRotation::ROTATION_0;
		if (s_hook)
		{
			StorageHelper::setData(ID_DISPLAY_ROTATION, r);
			s_hook(fmt::format("rotation {:d}", upsideDown ? 180 : 0));
			return;
		}
		DisplayHelper::setRotation(r); /* stores it too */
	}

	/* ---------------- sound ---------------- */

	unsigned int SoundSetting()
	{
		const unsigned int s = StorageHelper::getData(ID_FUNKY_SOUND);
		return s <= 6 ? s : 1;
	}

	void SetSoundSetting(unsigned int value)
	{
		if (value > 6)
		{
			return;
		}
		StorageHelper::setData(ID_FUNKY_SOUND, value);
		s_speakerVolumeSet = 0;
		Click(); /* hear the new setting */
	}

	void Click()
	{
		Beep(CLICK_FREQUENCY, CLICK_MS);
	}

	void Beep(unsigned int frequency, unsigned int ms)
	{
		const unsigned int setting = SoundSetting();
		if (setting == 0 || frequency == 0 || ms == 0)
		{
			return;
		}
		ms = std::min(ms, 3000u);
		/* The printer reports a beep until it ends: the same one again within its time is not new */
		const uint32_t now = lv_tick_get();
		if (frequency == s_lastBeepFreq && ms == s_lastBeepMs && ms > CLICK_MS && now - s_lastBeepTime < ms)
		{
			return;
		}
		s_lastBeepFreq = frequency;
		s_lastBeepMs = ms;
		s_lastBeepTime = now;
		if (setting == 1)
		{
			run(fmt::format("beep -f {:d} -l {:d}", frequency, ms));
			return;
		}
		const unsigned int level = setting - 1; /* 1..5 */
		if (s_speakerVolumeSet != level)
		{
			s_speakerVolumeSet = level;
			run(fmt::format("amixer -q sset 'Headphone' on; amixer -q sset 'DAC volume' 170,170; "
							"amixer -q sset 'Headphone volume' {:d}",
							(level * 7 + 2) / 5));
		}
		run(fmt::format("aplay -q {:s}", toneFile(frequency, ms)));
	}

	/* ---------------- connection ---------------- */

	Link Connection()
	{
		switch (Comm::DUET.GetCommunicationType())
		{
		case Comm::CommunicationType::usb:
			return Link::Usb;
		case Comm::CommunicationType::network:
			return Link::Network;
		case Comm::CommunicationType::uart:
			return Link::Uart;
		}
		return Link::Uart;
	}

	std::string DuetIp()
	{
		return Comm::DUET.GetIPAddress();
	}

	void SetConnection(Link link, const std::string& ip)
	{
		if (s_hook)
		{
			s_hook(fmt::format("connection {:d} {:s}", static_cast<int>(link), ip));
			return;
		}
		if (link == Link::Network && !ip.empty())
		{
			Comm::DUET.SetIPAddress(ip);
		}
		Comm::DUET.SetCommunicationType(link == Link::Usb		  ? Comm::CommunicationType::usb
										: link == Link::Network ? Comm::CommunicationType::network
																: Comm::CommunicationType::uart);
	}

	/* ---------------- personalization ---------------- */

	bool DrawerIndicator()
	{
		return StorageHelper::getData(ID_FUNKY_DRAWER_INDICATOR);
	}

	void SetDrawerIndicator(bool on)
	{
		StorageHelper::setData(ID_FUNKY_DRAWER_INDICATOR, on);
	}

	void FactoryReset()
	{
		SetBrightness(100);
		SetDimTimeout(static_cast<unsigned int>(DEFAULT_SCREEN_TIMEOUT.count()));
		SetSoundSetting(1);
		if (UpsideDown())
		{
			SetUpsideDown(false);
		}
		SetDrawerIndicator(true);
		PrinterStyle::SetSetting(PrinterStyle::Setting::Auto);
		FilamentModel::SetMode(FilamentModel::Mode::Panel);
	}

	void SetTestHook(std::function<void(const std::string&)> hook)
	{
		s_hook = std::move(hook);
	}
} // namespace UI::LaptopSettings
