/*
 * Workshop.h
 *
 * The illustrated workshop home scene: printer on a workbench, drawers that
 * open the control pages, and scene objects that react to the printer state.
 * It is registered as a home screen and covers the whole display; opening any
 * other screen hides it, and the Home button brings it back.
 */

#pragma once

#include "PrinterStyle.h"
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
		/* What the printer's little screen shows */
		enum class Mode : uint8_t
		{
			Offline,  /* dark, NO LINK */
			Idle,	  /* blue: READY + nozzle / bed */
			Busy,	  /* amber: BUSY + G29 / DWC / MACRO */
			Printing, /* green: PRINTING (PAUSED) + % */
			Error,	  /* red: a big warning triangle */
		};

		struct State
		{
			bool connected = false;
			bool printing = false;
			bool paused = false;
			bool busy = false;	/* running something that is not a job */
			bool error = false; /* halted (emergency stop) or a heater fault */
			std::string busyText;
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
		static Mode ModeOf(const State& state);
		/* The printer style changed (settings): swap the printer and the beds */
		void refreshStyle();

		/* For the tests */
		PrinterStyle::Style style() const { return m_style; }
		int printStage() const { return m_stage; }

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
		lv_obj_t* layer(const WorkshopLayout::LayerDef& def);
		void setLayer(lv_obj_t* obj, const WorkshopLayout::LayerDef& def);
		void buildPrinterDisplay();
		void placePrinterDisplay();
		void setPartStage(int stage);
		void showDisplay(const State& s);
		void animateDrawer(size_t index, bool opening);

		std::vector<std::pair<std::string, lv_draw_buf_t*>> m_images; /* decoded once, freed in the destructor */

		lv_obj_t* m_background = nullptr;
		lv_obj_t* m_wallBed = nullptr;
		lv_obj_t* m_printer = nullptr;
		lv_obj_t* m_spareBed = nullptr;
		lv_obj_t* m_folder = nullptr;
		/* the print on the printer's bed: faint red still to print, the printed part, red outline */
		lv_obj_t* m_ghostClip = nullptr;
		lv_obj_t* m_ghost = nullptr;
		lv_obj_t* m_partClip = nullptr;
		lv_obj_t* m_part = nullptr;
		lv_obj_t* m_outline = nullptr;
		PrinterStyle::Style m_style = PrinterStyle::Style::CoreXY;
		bool m_styleSet = false;
		int m_stage = -1;
		std::array<lv_obj_t*, PART_COUNT> m_spareParts{};
		static constexpr size_t HOTSPOT_COUNT = std::size(WorkshopLayout::HOTSPOTS);

		/* Raw LVGL objects must not use lv_obj user data: DuetScreen stores the
		 * LvObj wrapper pointer there. Tap targets are looked up in these arrays. */
		std::array<lv_obj_t*, DRAWER_COUNT> m_drawerTaps{};
		std::array<lv_obj_t*, HOTSPOT_COUNT> m_spotTaps{};
		static void tapEvent(lv_event_t* e);

		std::array<lv_obj_t*, DRAWER_COUNT> m_drawers{};
		std::array<std::array<lv_draw_buf_t*, WorkshopLayout::DRAWER_FRAMES>, DRAWER_COUNT> m_drawerFrames{};
		int m_openDrawer = -1;
		bool m_busy = false;

		/* printer display: a tinted screen with two rows, or the warning triangle */
		lv_obj_t* m_display = nullptr;
		lv_obj_t* m_title = nullptr;
		lv_obj_t* m_value = nullptr;
		lv_obj_t* m_triangle = nullptr;
		lv_obj_t* m_bang = nullptr;
		std::array<lv_point_precise_t, 4> m_trianglePoints{};
		FontManager::Font m_fontSmall;
		FontManager::Font m_fontLarge;

		size_t m_lastJobKey = 0;
		uint8_t m_partMask = 0;
		State m_state;
	};
} // namespace UI
