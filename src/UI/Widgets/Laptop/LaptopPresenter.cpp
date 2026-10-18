/*
 * LaptopPresenter.cpp
 */

#include "LaptopPresenter.h"
#include "Debug.h"
#include "Laptop.h"
#include "Subscribers/ResponseSubscribers.h"
#include <filesystem>

namespace UI
{
	void LaptopPresenter::onInit()
	{
		ZoneScoped;
		registerEventListener<EventType::Response>(this, &LaptopPresenter::newResponse);
		registerEventListener<EventType::Status>(this, &LaptopPresenter::newStatus);
		registerEventListener<EventType::JobFileName>(this, &LaptopPresenter::newJobFileName);
	}

	void LaptopPresenter::onConnect()
	{
		if (Laptop* view = getView(); view != nullptr)
		{
			view->info("Connected to the printer");
		}
	}

	void LaptopPresenter::onDisconnect()
	{
		if (Laptop* view = getView(); view != nullptr)
		{
			view->info("Disconnected from the printer");
		}
	}

	void LaptopPresenter::newResponse(ResponseType type, const std::string& text)
	{
		ZoneScoped;
		if (Laptop* view = getView(); view != nullptr)
		{
			view->response(type == ResponseType::ERROR, text);
		}
	}

	void LaptopPresenter::newJobFileName(const std::string& filename)
	{
		if (!filename.empty())
		{
			m_jobFile = filename;
		}
	}

	void LaptopPresenter::newStatus(const OM::PrinterStatus& status)
	{
		ZoneScoped;
		const OM::PrinterStatus old = m_status;
		m_status = status;
		Laptop* view = getView();
		if (view == nullptr || old == status)
		{
			return;
		}
		const bool wasPrinting = OM::IsPrintingStatus(old);
		const bool printing = OM::IsPrintingStatus(status);
		view->setJobActive(printing);
		if (old == OM::PrinterStatus::unknown)
		{
			return; /* first status after start-up: no event */
		}
		const std::string file = std::filesystem::path(m_jobFile).filename().string();
		if (printing && !wasPrinting)
		{
			view->info(file.empty() ? std::string("Print started") : "Print started: " + file);
		}
		else if (!printing && wasPrinting)
		{
			view->info(status == OM::PrinterStatus::cancelling ? std::string("Print cancelled")
														  : std::string("Print finished"));
		}
		else if (status == OM::PrinterStatus::paused && old != OM::PrinterStatus::paused)
		{
			view->info("Print paused");
		}
	}
} // namespace UI
