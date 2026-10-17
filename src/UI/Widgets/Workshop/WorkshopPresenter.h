/*
 * WorkshopPresenter.h
 *
 * Feeds printer state from the object model into the workshop home scene.
 */

#pragma once

#include "ObjectModel/PrinterStatus.h"
#include "UI/Core/Presenter.h"
#include <string>

namespace UI
{
	class Workshop;

	class WorkshopPresenter : public Presenter<Workshop>
	{
	  public:
		PRESENTER_CONSTRUCTOR(WorkshopPresenter, Workshop);

		void refresh();

	  protected:
		void onInit() override;
		void onActivate() override { refresh(); }
		void onDeactivate() override {}
		void onConnect() override { refresh(); }
		void onDisconnect() override { refresh(); }

	  private:
		void newStatus(const OM::PrinterStatus& status);
		void newJobFileName(const std::string& filename);

		OM::PrinterStatus m_status = OM::PrinterStatus::unknown;
		std::string m_jobFile;
	};
} // namespace UI
