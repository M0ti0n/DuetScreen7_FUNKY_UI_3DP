/*
 * WorkshopPresenter.h
 *
 * Feeds printer state from the object model into the workshop home scene.
 */

#pragma once

#include "ObjectModel/PrinterStatus.h"
#include "UI/Core/Presenter.h"
#include "lvgl/lvgl.h"
#include <cstdint>
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
		/* BUSY shows only after a second (short moves would make the screen blink) */
		bool m_busyPending = false;
		uint32_t m_busySince = 0;
		lv_timer_t* m_busyTimer = nullptr;
	};
} // namespace UI
