/*
 * FilamentBox.h
 *
 * The filament page, opened from the spool pile on the workshop scene: an MMU box on the desk
 * with four bays, one tool per bay (as the Modern UI FILAMENT page, see FilamentModel.h).
 *
 *   bay           the spool in the slot's colour; an empty spool when nothing is loaded or
 *                 the filament sensor reports no filament; no spool for bays without a tool
 *   screen        top: the tool and its nozzle temperature (the same on every screen when
 *                 the tools share one heater: MMU / single nozzle)
 *                 middle: extruding / retracting / idle, the material; tap: material + colour
 *                 (PANEL) or colour (MACROS)
 *                 bottom: EXTRUDE (PANEL) or CHANGE / LOAD (MACROS)
 *                 no filament: red, a warning in the middle row
 *                 no tool: "SPACE FOR UPGRADES :)"
 *   more than 4   the "< 1 / 2 >" plate on the top rail and the NEXT / BACK wooden arrows
 *   tools         hanging under the desk turn the pages
 *   wall          the fire bell (emergency stop) and the house picture (home); drawers below
 */

#pragma once

#include "FilamentBoxPresenter.h"
#include "FilamentLayout.h"
#include "FilamentModel.h"
#include "UI/Core/View.h"
#include "UI/Widgets/Laptop/FunkyPopup.h"
#include "UI/Widgets/Laptop/FunkyUi.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace UI
{
	class FilamentBox : public View<FilamentBoxPresenter>
	{
	  public:
		enum class Activity : uint8_t
		{
			Idle,
			Extruding,
			Retracting,
		};

		struct Slot
		{
			int tool = 0;
			bool hasTemp = false;
			float temp = 0;
			std::string material; /* MACROS mode: what RRF loaded ("" = nothing) */
			bool runout = false;  /* the filament sensor reports no filament */
			Activity activity = Activity::Idle;
		};

		struct State
		{
			bool connected = false;
			std::vector<Slot> slots; /* one per tool, in object-model order */
			int currentTool = -1;
			bool printing = false; /* printing, pausing, resuming, simulating */
			bool paused = false;
			std::vector<std::string> folders; /* /filaments (MACROS mode) */
		};

		FilamentBox(const std::string& name, LvObj& parent);
		~FilamentBox() override;

		void update(const State& state);

		/* Touches (also used by the tests) */
		void pressMaterial(size_t bay);
		void pressButton(size_t bay);
		void pressPage(int dir); /* +1 next, -1 back */
		void pressHome();
		void pressStop();
		void pressDrawer(size_t index);

		/* State, for the tests */
		size_t page() const { return m_page; }
		size_t pageCount() const;
		Funky::Popup& popup() { return m_popup; }
		bool loaded(const Slot& slot) const;
		std::string materialOf(const Slot& slot) const; /* as shown, "" = none */

	  protected:
		void onInit() override;
		void onShow() override;
		void onHide() override;

	  private:
		struct Bay
		{
			lv_obj_t* empty = nullptr;
			lv_obj_t* filament = nullptr;
			lv_obj_t* shade = nullptr;
			lv_obj_t* spool = nullptr;
			lv_obj_t* glow = nullptr;
			lv_obj_t* screen = nullptr; /* holds everything on the screen */
			lv_obj_t* red = nullptr;
			lv_obj_t* thermo = nullptr;
			lv_obj_t* tool = nullptr;
			lv_obj_t* temp = nullptr;
			lv_obj_t* materialBox = nullptr;
			lv_obj_t* activity = nullptr;
			lv_obj_t* material = nullptr;
			lv_obj_t* chevron = nullptr;
			lv_obj_t* buttonBox = nullptr;
			lv_obj_t* buttonIcon = nullptr;
			lv_obj_t* buttonText = nullptr;
			lv_obj_t* upgrades = nullptr;
		};

		void setArt(bool loaded);
		void refresh();
		void refreshBay(size_t bay);
		const Slot* slotOf(size_t bay) const;
		void startChange(const Slot& slot);
		void pickColour(int tool,
						const std::string& material,
						std::function<void(int)> done,
						std::function<void()> cancelled);
		void alert(const std::string& text);
		void send(const std::vector<std::string>& commands);
		static void clicked(lv_event_t* e);

		Funky::Images m_images;
		Funky::Fonts m_fonts;
		Funky::Popup m_popup{m_images, m_fonts};

		State m_state;
		size_t m_page = 0;
		bool m_built = false;

		lv_obj_t* m_background = nullptr;
		std::array<Bay, 4> m_bays{};
		lv_obj_t* m_plate = nullptr;
		lv_obj_t* m_plateText = nullptr;
		lv_obj_t* m_plateBack = nullptr;
		lv_obj_t* m_plateNext = nullptr;
		lv_obj_t* m_arrowNext = nullptr;
		lv_obj_t* m_arrowBack = nullptr;
		lv_obj_t* m_arrowNextTap = nullptr;
		lv_obj_t* m_arrowBackTap = nullptr;
		lv_obj_t* m_estop = nullptr;
		lv_obj_t* m_home = nullptr;
		std::array<lv_obj_t*, 5> m_drawers{};
	};
} // namespace UI
