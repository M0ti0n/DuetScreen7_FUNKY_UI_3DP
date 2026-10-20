/*
 * FilamentBoxPresenter.h
 *
 * Feeds the filament page: the tools (heater temperatures, the filament RRF loaded, the
 * filament sensors), the printer status, the extrusion of the active tool, and the /filaments
 * folders (MACROS mode). Sends the filament G-code.
 */

#pragma once

#include "UI/Core/Presenter.h"
#include <atomic>
#include <functional>
#include <string>
#include <vector>

namespace UI
{
	class FilamentBox;

	class FilamentBoxPresenter : public Presenter<FilamentBox>
	{
	  public:
		PRESENTER_CONSTRUCTOR(FilamentBoxPresenter, FilamentBox);

		void refresh();
		void requestFolders();

		/* G-code, one command a line; tests catch them with the hook */
		static void Send(const std::vector<std::string>& commands);
		static void SetSendHook(std::function<void(const std::string&)> hook);

	  protected:
		void onInit() override;
		void onActivate() override;
		void onConnect() override { refresh(); }
		void onDisconnect() override { refresh(); }

	  private:
		uint32_t m_monitorSeen = 0;
	};
} // namespace UI
