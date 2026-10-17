/*
 * Workshop.h
 *
 * The illustrated workshop home scene: printer on a workbench, drawers that
 * open the control pages, and scene objects that react to the printer state.
 * It is registered as a home screen and covers the whole display; opening any
 * other screen hides it, and the Home button brings it back.
 */

#pragma once

#include "UI/Core/View.h"
#include "UI/Styles/Font.h"
#include "WorkshopLayout.h"
#include "WorkshopPresenter.h"
#include <array>
#include <string>
#include <vector>

namespace UI
{
	class Workshop : public View<WorkshopPresenter>
	{
	  public:
		struct State
		{
			bool connected = false;
			bool printing = false;
			bool paused = false;
			float progress = 0.0f; /* 0..1 */
			uint32_t timeLeft = 0; /* seconds */
			size_t jobKey = 0;	   /* changes when a different job starts */
			std::string toolName;
			float nozzle = 0, bed = 0, chamber = 0;
			bool hasNozzle = false, hasBed = false, hasChamber = false;
		};

		Workshop(const std::string& name, LvObj& parent);
		~Workshop() override;

		void update(const State& state);

		/* Open what a scene object or drawer leads to (also used by tests) */
		void open(WorkshopLayout::Target target);
		void openDrawer(size_t index);

	  protected:
		void onInit() override;
		void onShow() override;

	  private:
		static constexpr size_t DRAWER_COUNT = std::size(WorkshopLayout::DRAWERS);
		static constexpr size_t PART_COUNT = std::size(WorkshopLayout::SPARE_BED_PARTS);

		lv_draw_buf_t* load(const std::string& relPath);
		lv_obj_t* addHotspot(const WorkshopLayout::Rect& r);
		void buildPrinterDisplay();
		void setPartProgress(float progress);
		void animateDrawer(size_t index, bool opening);

		std::vector<lv_draw_buf_t*> m_images; /* decoded once, freed in the destructor */

		lv_obj_t* m_background = nullptr;
		lv_obj_t* m_partClip = nullptr;
		lv_obj_t* m_part = nullptr;
		std::array<lv_obj_t*, PART_COUNT> m_spareParts{};
		std::array<lv_obj_t*, DRAWER_COUNT> m_drawers{};
		std::array<std::array<lv_draw_buf_t*, WorkshopLayout::DRAWER_FRAMES>, DRAWER_COUNT> m_drawerFrames{};
		int m_openDrawer = -1;
		bool m_busy = false;

		/* printer display */
		lv_obj_t* m_display = nullptr;
		lv_obj_t* m_idlePanel = nullptr;
		lv_obj_t* m_printPanel = nullptr;
		lv_obj_t* m_idleTitle = nullptr;
		std::array<lv_obj_t*, 3> m_idleValues{};
		lv_obj_t* m_printTitle = nullptr;
		lv_obj_t* m_printRight = nullptr;
		lv_obj_t* m_percent = nullptr;
		lv_obj_t* m_nozzle = nullptr;
		lv_obj_t* m_timeLeft = nullptr;
		lv_obj_t* m_bar = nullptr;
		FontManager::Font m_fontSmall;
		FontManager::Font m_fontMedium;
		FontManager::Font m_fontLarge;

		size_t m_lastJobKey = 0;
		uint8_t m_partMask = 0;
		State m_state;
	};
} // namespace UI
