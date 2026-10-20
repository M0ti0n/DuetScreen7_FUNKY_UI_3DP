/*
 * FilamentBoxPresenter.cpp
 */

#include "FilamentBoxPresenter.h"
#include "Debug.h"
#include "FilamentBox.h"
#include "FilamentModel.h"
#include "Hardware/Duet.h"
#include "ObjectModel/Axis.h"
#include "ObjectModel/Files.h"
#include "ObjectModel/PrinterStatus.h"
#include "ObjectModel/Tool.h"
#include "Subscribers/Subscribers.h"
#include <array>
#include <cstring>
#include <mutex>

namespace UI
{
	namespace
	{
		/* sensors.filamentMonitors[<extruder>].status: DuetScreen does not keep it, so it is kept here */
		constexpr size_t MONITORS = FilamentModel::MAX_SLOTS;
		std::mutex s_monitorLock;
		std::array<bool, MONITORS> s_noFilament{};
		std::atomic<uint32_t> s_monitorGeneration{0};
		std::function<void(const std::string&)> s_sendHook;

		bool onMonitorStatus(Comm::JsonDecoder*, const char* data, const size_t indices[])
		{
			const size_t i = indices[0];
			if (i < MONITORS)
			{
				std::lock_guard<std::mutex> lock(s_monitorLock);
				const bool no = std::strcmp(data, "noFilament") == 0;
				if (s_noFilament[i] != no)
				{
					s_noFilament[i] = no;
					s_monitorGeneration++;
				}
			}
			return true;
		}
	} // namespace

	void FilamentBoxPresenter::Send(const std::vector<std::string>& commands)
	{
		for (const std::string& c : commands)
		{
			LOG_INFO("Filament: {:s}", c);
			if (s_sendHook)
			{
				s_sendHook(c);
			}
			else
			{
				Comm::DUET.SendGcode(c + "\n");
			}
		}
	}

	void FilamentBoxPresenter::SetSendHook(std::function<void(const std::string&)> hook)
	{
		s_sendHook = std::move(hook);
	}

	void FilamentBoxPresenter::onInit()
	{
		ZoneScoped;
		static bool subscribed = false;
		if (!subscribed)
		{
			subscribed = true;
			SubscriberMap::addSubscriber("sensors:filamentMonitors^:status", onMonitorStatus);
		}
		registerEventListener<EventType::Status>([this](const OM::PrinterStatus&) { refresh(); });
		registerEventListener<EventType::ToolData>([this]() { refresh(); });
		registerEventListener<EventType::CurrentTool>([this]() { refresh(); });
		registerEventListener<EventType::HeaterData>([this]() { refresh(); });
		registerEventListener<EventType::ToolHeaterData>([this](size_t) { refresh(); });
		registerEventListener<EventType::ExtruderData>([this]() { refresh(); });
		registerEventListener<EventType::CurrentMoveExtrusionSpeed>([this]() { refresh(); });
		registerEventListener<EventType::Filaments>([this](const std::vector<std::string>&) { refresh(); });
		registerEventListener<EventType::Tick>(
			[this]()
			{
				if (s_monitorGeneration.load() != m_monitorSeen)
				{
					refresh();
				}
			});
	}

	void FilamentBoxPresenter::onActivate()
	{
		if (FilamentModel::GetMode() == FilamentModel::Mode::Macros)
		{
			requestFolders();
		}
		refresh();
	}

	void FilamentBoxPresenter::requestFolders()
	{
		if (OM::IsConnected())
		{
			OM::FileSystem::RequestFilaments();
		}
	}

	void FilamentBoxPresenter::refresh()
	{
		ZoneScoped;
		FilamentBox* view = getView();
		if (view == nullptr)
		{
			return;
		}
		m_monitorSeen = s_monitorGeneration.load();
		std::array<bool, MONITORS> noFilament{};
		{
			std::lock_guard<std::mutex> lock(s_monitorLock);
			noFilament = s_noFilament;
		}

		FilamentBox::State s;
		{
			MODEL_LOCK();
			const OM::PrinterStatus status = OM::GetStatus();
			s.connected = OM::IsConnected();
			s.printing = status == OM::PrinterStatus::processing || status == OM::PrinterStatus::pausing ||
						 status == OM::PrinterStatus::resuming || status == OM::PrinterStatus::simulating;
			s.paused = status == OM::PrinterStatus::paused;
			auto current = OM::GetCurrentTool();
			s.currentTool = current != nullptr ? static_cast<int>(current->index) : -1;
			const float rate = OM::Move::GetExtrusionRate();
			s.folders = OM::FileSystem::GetFilamentList();

			OM::IterateToolsWhile(
				[&](OM::ToolPtr tool, size_t)
				{
					if (tool == nullptr || s.slots.size() >= FilamentModel::MAX_SLOTS)
					{
						return true;
					}
					FilamentBox::Slot slot;
					slot.tool = static_cast<int>(tool->index);
					if (auto th = tool->heaters[0]; th != nullptr && th->heater != nullptr)
					{
						slot.hasTemp = true;
						slot.temp = th->heater->current; /* tools sharing a heater show the same */
					}
					if (auto ext = tool->extruders[0]; ext != nullptr)
					{
						slot.material = ext->filamentName.c_str();
						slot.runout = ext->index < MONITORS && noFilament[ext->index];
					}
					if (slot.tool == s.currentTool)
					{
						slot.activity = rate > 0.01f	? FilamentBox::Activity::Extruding
										: rate < -0.01f ? FilamentBox::Activity::Retracting
														: FilamentBox::Activity::Idle;
					}
					s.slots.push_back(std::move(slot));
					return true;
				});
		}
		view->update(s);
	}
} // namespace UI
