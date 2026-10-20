/*
 * MachineActivity.cpp
 */

#include "MachineActivity.h"
#include "Subscribers/Subscribers.h"
#include <array>
#include <cctype>
#include <chrono>
#include <cstring>
#include <mutex>

namespace UI::MachineActivity
{
	namespace
	{
		constexpr size_t CHANNELS = 16;
		struct Channel
		{
			std::string name;
			bool executing = false;
		};
		std::mutex s_lock;
		std::array<Channel, CHANNELS> s_channels;
		std::string s_lastCommand;
		uint32_t s_lastCommandMs = 0;
		bool s_hasCommand = false;

		bool onName(Comm::JsonDecoder*, const char* data, const size_t indices[])
		{
			SetChannel(indices[0], data, nullptr);
			return true;
		}

		bool onState(Comm::JsonDecoder*, const char* data, const size_t indices[])
		{
			SetChannel(indices[0], nullptr, data);
			return true;
		}
	} // namespace

	void Init()
	{
		static bool done = false;
		if (done)
		{
			return;
		}
		done = true;
		SubscriberMap::addSubscriber("inputs^:name", onName);
		SubscriberMap::addSubscriber("inputs^:state", onState);
	}

	uint32_t NowMs()
	{
		using namespace std::chrono;
		return static_cast<uint32_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
	}

	std::string CommandCode(std::string_view gcode)
	{
		size_t i = 0;
		while (i < gcode.size() && std::isspace(static_cast<unsigned char>(gcode[i])))
		{
			i++;
		}
		std::string code;
		if (i < gcode.size())
		{
			const char letter = static_cast<char>(std::toupper(static_cast<unsigned char>(gcode[i])));
			if (letter != 'G' && letter != 'M' && letter != 'T')
			{
				return "";
			}
			code += letter;
			i++;
			while (i < gcode.size() && (std::isdigit(static_cast<unsigned char>(gcode[i])) || gcode[i] == '.'))
			{
				code += gcode[i++];
			}
		}
		if (code.size() < 2 && !(code == "T"))
		{
			return "";
		}
		if (code == "M98")
		{
			return "MACRO";
		}
		return code;
	}

	void RecordCommand(std::string_view gcode)
	{
		std::string code = CommandCode(gcode);
		if (code.empty() || code == "M408" || code == "M409" || code == "M36" || code == "M20" || code == "M122")
		{
			return; /* status / file requests, not something the printer gets busy with */
		}
		std::lock_guard<std::mutex> lock(s_lock);
		s_lastCommand = std::move(code);
		s_lastCommandMs = NowMs();
		s_hasCommand = true;
	}

	std::string LastScreenCommand(uint32_t sinceMs)
	{
		std::lock_guard<std::mutex> lock(s_lock);
		if (!s_hasCommand || static_cast<int32_t>(s_lastCommandMs - sinceMs) < 0)
		{
			return "";
		}
		return s_lastCommand;
	}

	std::string BusySource()
	{
		std::lock_guard<std::mutex> lock(s_lock);
		bool dwc = false, usb = false, macro = false;
		for (const Channel& c : s_channels)
		{
			if (!c.executing)
			{
				continue;
			}
			if (c.name == "HTTP" || c.name == "Telnet" || c.name == "SBC")
				dwc = true;
			else if (c.name.starts_with("USB")) /* USB, USB2 (RRF 3.6.2+) */
				usb = true;
			else if (c.name == "Daemon" || c.name == "Trigger" || c.name == "Queue" || c.name == "Queue2" ||
					 c.name == "Autopause")
				macro = true;
		}
		return dwc ? "DWC" : usb ? "USB" : macro ? "MACRO" : "";
	}

	void SetChannel(size_t index, const char* name, const char* state)
	{
		if (index >= CHANNELS)
		{
			return;
		}
		std::lock_guard<std::mutex> lock(s_lock);
		if (name != nullptr)
		{
			s_channels[index].name = name;
		}
		if (state != nullptr)
		{
			/* RRF 3.6.2+ reports "unused" for channels not used since start-up */
			s_channels[index].executing =
				state[0] != '\0' && std::strcmp(state, "idle") != 0 && std::strcmp(state, "unused") != 0;
		}
	}

	void Reset()
	{
		std::lock_guard<std::mutex> lock(s_lock);
		s_channels = {};
		s_lastCommand.clear();
		s_hasCommand = false;
	}
} // namespace UI::MachineActivity
