/*
 * WorkshopPresenter.cpp
 */

#include "WorkshopPresenter.h"
#include "Debug.h"
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
