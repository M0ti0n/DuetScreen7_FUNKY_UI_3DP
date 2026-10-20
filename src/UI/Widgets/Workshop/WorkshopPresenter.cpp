/*
 * WorkshopPresenter.cpp
 */

#include "WorkshopPresenter.h"
#include "Debug.h"
#include "MachineActivity.h"
#include "ObjectModel/BedOrChamber.h"
#include "ObjectModel/Heat.h"
#include "ObjectModel/Job.h"
#include "ObjectModel/Tool.h"
#include "Workshop.h"
#include <functional>

namespace UI
{
	void WorkshopPresenter::onInit()
	{
		ZoneScoped;
		MachineActivity::Init();
		registerEventListener<EventType::Status>(this, &WorkshopPresenter::newStatus);
		registerEventListener<EventType::JobFileName>(this, &WorkshopPresenter::newJobFileName);
		registerEventListener<EventType::HeaterData>([this]() { refresh(); });
		registerEventListener<EventType::BedHeaterData>([this]() { refresh(); });
		registerEventListener<EventType::ChamberHeaterData>([this]() { refresh(); });
		registerEventListener<EventType::ToolHeaterData>([this](size_t) { refresh(); });
		registerEventListener<EventType::CurrentTool>([this]() { refresh(); });
		registerEventListener<EventType::JobFileProgress>([this]() { refresh(); });
		registerEventListener<EventType::JobTimeLeft>([this]() { refresh(); });
	}

	void WorkshopPresenter::newStatus(const OM::PrinterStatus& status)
	{
		ZoneScoped;
		m_status = status;
		refresh();
	}

	void WorkshopPresenter::newJobFileName(const std::string& filename)
	{
		ZoneScoped;
		if (!filename.empty())
		{
			m_jobFile = filename;
		}
		refresh();
	}

	void WorkshopPresenter::refresh()
	{
		ZoneScoped;
		Workshop* view = getView();
		if (view == nullptr)
		{
			return;
		}

		Workshop::State s;
		{
			MODEL_LOCK();
			const OM::PrinterStatus status = OM::GetStatus();
			s.connected = OM::IsConnected();
			s.printing = OM::IsPrintingStatus(status);
			s.paused = status == OM::PrinterStatus::paused || status == OM::PrinterStatus::pausing;
			const bool jobLoaded = !OM::GetJobName().empty();
			if ((status == OM::PrinterStatus::changingTool && jobLoaded) || status == OM::PrinterStatus::cancelling)
			{
				s.printing = true; /* a tool change inside the job; a job being cancelled */
			}
			const bool offline = status == OM::PrinterStatus::starting || status == OM::PrinterStatus::updating ||
								 status == OM::PrinterStatus::off || status == OM::PrinterStatus::connecting ||
								 status == OM::PrinterStatus::disconnected;
			s.connected = s.connected && !offline;

			/* ERROR: emergency stop, or a heater fault */
			s.error = status == OM::PrinterStatus::halted;
			OM::Heat::IterateHeatersWhile(
				[&s](OM::Heat::HeaterPtr h, size_t)
				{
					if (h != nullptr && h->status == OM::Heat::HeaterStatus::fault)
					{
						s.error = true;
					}
					return !s.error;
				});

			/* BUSY: G28 / G29 / a macro..., not a job */
			const bool busyNow =
				!s.printing && (status == OM::PrinterStatus::busy || status == OM::PrinterStatus::changingTool);
			const uint32_t now = MachineActivity::NowMs();
			if (!busyNow)
			{
				m_busyPending = false;
			}
			else if (!m_busyPending)
			{
				m_busyPending = true;
				m_busySince = now;
			}
			s.busy = busyNow && now - m_busySince >= 1000;
			if (busyNow && !s.busy && m_busyTimer == nullptr)
			{
				m_busyTimer = lv_timer_create(
					[](lv_timer_t* t)
					{
						auto* self = static_cast<WorkshopPresenter*>(lv_timer_get_user_data(t));
						self->m_busyTimer = nullptr;
						self->refresh();
					},
					1050,
					this);
				lv_timer_set_repeat_count(m_busyTimer, 1);
			}
			if (s.busy)
			{
				/* sent from this screen (just before it got busy), else where the command came from */
				s.busyText = MachineActivity::LastScreenCommand(m_busySince - 3000);
				if (s.busyText.empty())
				{
					s.busyText = MachineActivity::BusySource();
				}
			}

			const uint32_t size = OM::GetFileSize();
			s.progress = size > 0 ? static_cast<float>(OM::GetFilePosition()) / static_cast<float>(size) : 0.0f;
			s.timeLeft = OM::GetPrintRemaining(OM::RemainingTimeType::AUTO);
			s.jobKey = std::hash<std::string>{}(m_jobFile.empty() ? OM::GetJobName() : m_jobFile);

			auto tool = OM::GetCurrentTool();
			if (tool == nullptr)
			{
				tool = OM::GetToolBySlot(0);
			}
			if (tool != nullptr)
			{
				s.toolName = tool->GetName();
				if (auto th = tool->GetHeater(0); th != nullptr && th->heater != nullptr)
				{
					s.nozzle = th->heater->current;
					s.hasNozzle = true;
				}
			}
			if (auto bed = OM::GetFirstBed(); bed != nullptr)
			{
				s.bed = bed->GetCurrentTemp();
				s.hasBed = true;
			}
			if (OM::GetChamberCount() > 0)
			{
				if (auto chamber = OM::GetChamberBySlot(0); chamber != nullptr)
				{
					s.chamber = chamber->GetCurrentTemp();
					s.hasChamber = true;
				}
			}
		}
		view->update(s);
	}
} // namespace UI
