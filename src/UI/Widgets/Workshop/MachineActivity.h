/*
 * MachineActivity.h
 *
 * What the printer is busy with, for the BUSY screen of the printer on the workshop scene:
 *
 *   - G-code sent from this screen (laptop console, bed levelling, drawers...) is remembered
 *     (Duet::SendGcode calls RecordCommand), so "G29", "G28", "MACRO" (M98) can be shown;
 *   - otherwise the firmware's input channels (object model `inputs`) tell where the running
 *     command came from: DWC (HTTP, Telnet, SBC), USB, or MACRO (daemon, triggers, queue).
 */

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace UI::MachineActivity
{
	void Init(); /* subscribes to inputs^:name / inputs^:state (once) */

	void RecordCommand(std::string_view gcode);
	/* "G29", "MACRO", ... for the last command sent from this screen; "" if none since 'sinceMs' */
	std::string LastScreenCommand(uint32_t sinceMs);
	uint32_t NowMs();

	/* "DWC", "USB", "MACRO" from the busy input channels; "" when none is known */
	std::string BusySource();

	/* The short text for a command: "G29" from "G29 S1", "MACRO" for M98, "" for nothing useful */
	std::string CommandCode(std::string_view gcode);
	/* Tests */
	void SetChannel(size_t index, const char* name, const char* state);
	void Reset();
} // namespace UI::MachineActivity
