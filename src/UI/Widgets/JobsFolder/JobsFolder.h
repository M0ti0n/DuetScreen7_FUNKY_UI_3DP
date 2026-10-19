/*
 * JobsFolder.h
 *
 * The jobs folder, opened from the job folder rack on the workshop scene: an open folder on
 * the desk, laid out like the object cancel folder.
 *
 *   left page    the selected job: its thumbnail as a photo taped on the page, the file name,
 *                print time, filament, layer height, height, slicer and when it was saved,
 *                and a red PRINT stamp (START PRINT popup: delete / cancel / start)
 *   right page   "JOBS:" with the sort direction arrows and "page/pages", then the files of
 *                the folder, 8 a page, with their age (or their material when sorted by FILA)
 *   right side   page up / SORT (NEW, A-Z, FILA) / page down cards; HOME sticky note on top
 *   corner       the red STOP sticker (emergency stop)
 *   bottom       the workbench drawers, as on the home scene
 *
 * Folders are listed first; tap one to open it, ".." to go back up.
 */

#pragma once

#include "JobsFolderLayout.h"
#include "JobsFolderPresenter.h"
#include "JobsList.h"
#include "UI/Core/View.h"
#include "UI/Widgets/Laptop/FunkyPopup.h"
#include "UI/Widgets/Laptop/FunkyUi.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace UI
{
	class JobsFolder : public View<JobsFolderPresenter>
	{
	  public:
		struct Details
		{
			bool valid = false; /* a file is selected */
			std::string path;
			std::string name;
			bool infoKnown = false; /* the file info (M36) has arrived */
			uint32_t printTime = 0; /* s */
			float filamentMm = 0;
			float layerHeight = 0;
			float height = 0;
			std::string slicer;
			std::string saved;	   /* "2026-10-18T14:02:00" */
			std::string thumbnail; /* cached thumbnail file, "" = none */
		};

		JobsFolder(const std::string& name, LvObj& parent);
		~JobsFolder() override;

		void setFiles(const std::string& folder, const std::vector<JobsList::Entry>& entries);
		void setDetails(const Details& details);

		/* Touches (also used by the tests) */
		void pressRow(size_t row);
		void pressPrint();
		void pressSort();
		void pressDirection(bool ascending);
		void pressPage(int dir);
		void pressHome();
		void pressStop();
		void pressDrawer(size_t index);

		/* State, for the tests */
		void setSort(JobsList::Sort sort, bool ascending);
		JobsList::Sort sort() const { return m_sort; }
		bool ascending() const { return m_ascending; }
		size_t page() const { return m_page; }
		size_t pageCount() const;
		const std::string& selectedPath() const { return m_details.path; }
		const std::vector<JobsList::Entry>& entries() const { return m_entries; }
		Funky::Popup& popup() { return m_popup; }

		/* Detail values as shown, for the tests */
		static std::string FormatPrintTime(uint32_t seconds);
		static std::string FormatSaved(const std::string& date);

	  protected:
		void onInit() override;
		void onShow() override;
		void onHide() override;

	  private:
		struct Row
		{
			lv_obj_t* box = nullptr;
			lv_obj_t* icon = nullptr;
			lv_obj_t* name = nullptr;
			lv_obj_t* age = nullptr;
			lv_obj_t* tag = nullptr;
			lv_obj_t* tagText = nullptr;
			lv_obj_t* tap = nullptr;
		};

		void setArt(bool loaded);
		void resort();
		void refresh();
		void refreshList();
		void refreshDetails();
		void confirmDelete(const std::string& path, const std::string& name);
		bool presenterActive();
		static void clicked(lv_event_t* e);

		Funky::Images m_images;
		Funky::Fonts m_fonts;
		Funky::Popup m_popup{m_images, m_fonts};

		std::string m_folder;
		std::vector<JobsList::Entry> m_entries; /* sorted */
		JobsList::Sort m_sort = JobsList::Sort::Newest;
		bool m_ascending = false;
		size_t m_page = 0;
		Details m_details;
		bool m_built = false;

		lv_obj_t* m_background = nullptr;
		/* left page */
		lv_obj_t* m_thumbBack = nullptr;
		lv_obj_t* m_thumb = nullptr;
		lv_obj_t* m_photo = nullptr;
		lv_obj_t* m_fileName = nullptr;
		lv_obj_t* m_divider = nullptr;
		std::array<lv_point_precise_t, 2> m_dividerPoints{};
		std::array<lv_obj_t*, 6> m_infoKeys{};
		std::array<lv_obj_t*, 6> m_infoValues{};
		lv_obj_t* m_stamp = nullptr;
		lv_obj_t* m_pick = nullptr;
		/* right page */
		lv_obj_t* m_pagesLabel = nullptr;
		lv_obj_t* m_sortUp = nullptr;
		lv_obj_t* m_sortDown = nullptr;
		lv_obj_t* m_listEmpty = nullptr;
		std::array<Row, JobsFolderLayout::ROWS> m_rows{};
		/* side */
		lv_obj_t* m_sortValue = nullptr;
		lv_obj_t* m_upShade = nullptr;
		lv_obj_t* m_downShade = nullptr;
		lv_obj_t* m_up = nullptr;
		lv_obj_t* m_sortTap = nullptr;
		lv_obj_t* m_down = nullptr;
		lv_obj_t* m_estop = nullptr;
		lv_obj_t* m_home = nullptr;
		lv_obj_t* m_stampTap = nullptr;
		lv_obj_t* m_sortUpTap = nullptr;
		lv_obj_t* m_sortDownTap = nullptr;
		std::array<lv_obj_t*, 5> m_drawers{};
	};
} // namespace UI
