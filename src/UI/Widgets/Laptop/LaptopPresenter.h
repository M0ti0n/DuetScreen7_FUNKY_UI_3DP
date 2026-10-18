/*
 * LaptopPresenter.h
 *
 * Feeds the laptop: every machine reply (console log and alerts), job and connection
 * events (INFO alerts), and whether a job runs (the connection can't change then).
 * It stays active while the page is hidden, so nothing is missed.
 */

#pragma once

#include "ObjectModel/PrinterStatus.h"
#include "UI/Core/Presenter.h"
#include <string>

enum class ResponseType;

namespace UI
{
	class Laptop;

	class LaptopPresenter : public Presenter<Laptop>
	{
	  public:
		PRESENTER_CONSTRUCTOR(LaptopPresenter, Laptop);

	  protected:
		void onInit() override;
		void onConnect() override;
		void onDisconnect() override;

	  private:
		void newResponse(ResponseType type, const std::string& text);
		void newStatus(const OM::PrinterStatus& status);
		void newJobFileName(const std::string& filename);

		OM::PrinterStatus m_status = OM::PrinterStatus::unknown;
		std::string m_jobFile;
	};
} // namespace UI
