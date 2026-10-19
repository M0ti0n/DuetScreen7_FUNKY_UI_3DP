/*
 * ObjectFolder.h
 *
 * The object cancel page, opened from the spare bed on the workshop scene: an open
 * folder on the desk, as the Modern UI OBJECT tab.
 *
 *   left page    the bed between pencil axes, a numbered box for each object
 *                (blue = selected, red = cancelled)
 *   right page   the job name and the objects, 5 per page (number + name)
 *   right side   page up / page / page down cards; HOME sticky note on top
 *   corner       the red STOP sticker (emergency stop)
 *   bottom       the workbench drawers, as on the home scene
 *
 * Tap a name or a box on the bed: select it (the list turns to its page). Tap a number:
 * CANCEL OBJECT popup, check = M486 P<n>. Cancelled objects get a red number, a red box
 * and their name struck through. The selection follows the object being printed.
 */

#pragma once

#include "ObjectFolderLayout.h"
#include "ObjectFolderPresenter.h"
#include "UI/Core/View.h"
#include "UI/Widgets/Laptop/FunkyPopup.h"
#include "UI/Widgets/Laptop/FunkyUi.h"
#include <array>
#include <memory>
#include <string>
#include <vector>

namespace UI
{
	class ObjectFolder : public View<ObjectFolderPresenter>
	{
	  public:
		struct Object
		{
			bool present = false;
			std::string name;
			bool cancelled = false;
			bool hasPosition = false;
			float x = 0, y = 0; /* centre, mm */
		};

		struct State
		{
			bool printing = false;
			std::string job;
			std::vector<Object> objects; /* by object index */
			int current = -1;			 /* object being printed */
			bool hasBed = false;
			float xMin = 0, xMax = 0, yMin = 0, yMax = 0;
		};

		ObjectFolder(const std::string& name, LvObj& parent);
		~ObjectFolder() override;

		void update(const State& state);

		/* Touches (also used by the tests) */
		void pressName(size_t row);
		void pressNumber(size_t row);
		void pressMarker(size_t index);
		void pressPage(int dir);
		void pressHome();
		void pressStop();
		void pressDrawer(size_t index);

		/* State, for the tests */
		int selected() const { return m_selected; }
		size_t page() const { return m_page; }
		size_t pageCount() const;
		Funky::Popup& popup() { return m_popup; }

	  protected:
		void onInit() override;
		void onShow() override;
		void onHide() override;

	  private:
		struct Row
		{
			lv_obj_t* highlight = nullptr;
			lv_obj_t* numBox = nullptr;
			lv_obj_t* num = nullptr;
			lv_obj_t* nameBox = nullptr;
			lv_obj_t* name = nullptr;
			lv_obj_t* strike = nullptr;
			lv_obj_t* numTap = nullptr;
			lv_obj_t* nameTap = nullptr;
			std::unique_ptr<lv_point_precise_t[]> strikePoints;
		};
		struct Marker
		{
			lv_obj_t* box = nullptr;
			lv_obj_t* label = nullptr;
		};

		void setArt(bool loaded);
		void refresh();
		void refreshList();
		void refreshBed();
		void ensureMarkers(size_t count);
		bool objectValid(size_t index) const;
		static void clicked(lv_event_t* e);

		Funky::Images m_images;
		Funky::Fonts m_fonts;
		Funky::Popup m_popup{m_images, m_fonts};

		State m_state;
		int m_selected = -1;
		int m_lastCurrent = -1;
		size_t m_page = 0;
		bool m_built = false;

		lv_obj_t* m_background = nullptr;
		lv_obj_t* m_job = nullptr;
		lv_obj_t* m_pageLabel = nullptr;
		lv_obj_t* m_upShade = nullptr;
		lv_obj_t* m_downShade = nullptr;
		lv_obj_t* m_bedEmpty = nullptr;
		lv_obj_t* m_listEmpty = nullptr;
		std::array<lv_obj_t*, 4> m_bedLines{};
		std::array<std::array<lv_point_precise_t, 2>, 4> m_bedPoints{};
		std::array<Row, ObjectFolderLayout::ROWS> m_rows{};
		std::vector<Marker> m_markers;

		lv_obj_t* m_estop = nullptr;
		lv_obj_t* m_home = nullptr;
		lv_obj_t* m_up = nullptr;
		lv_obj_t* m_down = nullptr;
		std::array<lv_obj_t*, 5> m_drawers{};
	};
} // namespace UI
