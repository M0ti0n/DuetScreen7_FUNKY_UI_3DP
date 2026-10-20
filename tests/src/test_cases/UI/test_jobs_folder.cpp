/*
 * test_jobs_folder.cpp
 *
 * The jobs folder: the list (sorting, pages, folders), the details page and the
 * START PRINT / DELETE FILE popups. Also the list model (JobsList) on its own.
 */

#include "Debug.h"
#include "UI/Core/Navigation.h"
#include "UI/Screens/Home/HomeView.h"
#include "UI/Widgets/JobsFolder/JobsFolder.h"
#include "UI/Widgets/JobsFolder/JobsList.h"
#include "test_utils/UiTestSuite.h"
#include "utils/StorageHelper.h"
#include <gtest/gtest.h>

using UI::JobsFolder;
namespace JobsList = UI::JobsList;
using JobsList::Entry;
using JobsList::Sort;

namespace
{
	/* "now" for the list dates: 18 Oct 2026, 14:00 local time */
	std::time_t testNow()
	{
		std::tm t{};
		t.tm_year = 2026 - 1900;
		t.tm_mon = 9;
		t.tm_mday = 18;
		t.tm_hour = 14;
		t.tm_isdst = -1;
		return std::mktime(&t);
	}

	Entry file(const char* name, const char* date)
	{
		Entry e;
		e.name = name;
		e.path = std::string("0:/gcodes/") + name;
		e.date = date;
		e.when = JobsList::DateText(date, testNow());
		return e;
	}

	Entry folder(const char* name)
	{
		Entry e;
		e.name = name;
		e.path = std::string("0:/gcodes/") + name;
		e.folder = true;
		return e;
	}

	std::vector<Entry> files()
	{
		return {
			file("calibration_cube_PLA.gcode", "2026-10-18T09:10:00"),
			file("engine_block_PETG.gcode", "2026-10-18T12:02:00"),
			folder("brackets"),
			file("benchy_0p2_PLA.gcode", "2026-10-17T14:00:00"),
			file("first_layer_test_PLA.gcode", "2026-10-16T14:00:00"),
			file("temp_tower_PETG.gcode", "2026-10-15T14:00:00"),
			file("badge_dual_ASA.gcode", "2026-10-13T14:00:00"),
			file("drawer_handle_PLA.gcode", "2026-10-11T14:00:00"),
			file("cable_clips_TPU.gcode", "2026-10-04T14:00:00"),
			file("lamp_shade_vase_mode.gcode", "2026-09-02T14:00:00"),
			file("spool_holder_PETG.gcode", "2026-06-01T14:00:00"),
		};
	}

	std::vector<std::string> names(const std::vector<Entry>& entries)
	{
		std::vector<std::string> out;
		for (const auto& e : entries)
		{
			out.push_back(e.name);
		}
		return out;
	}
} // namespace

/* ---------------- the model ---------------- */

TEST(JobsList, MaterialFromTheFileName)
{
	EXPECT_EQ(JobsList::MaterialOf("engine_block_PETG.gcode"), "PETG");
	EXPECT_EQ(JobsList::MaterialOf("cube_pla.gcode"), "PLA");
	EXPECT_EQ(JobsList::MaterialOf("part_PA.gcode"), "PA");
	EXPECT_EQ(JobsList::MaterialOf("lamp_shade_vase_mode.gcode"), "");
	EXPECT_EQ(JobsList::MaterialOf("nothing.gcode"), "");
}

TEST(JobsList, Sorting)
{
	auto e = files();
	JobsList::SortEntries(e, Sort::Newest, false);
	EXPECT_EQ(e[0].name, "brackets"); /* folders first */
	EXPECT_EQ(e[1].name, "engine_block_PETG.gcode");
	EXPECT_EQ(e.back().name, "spool_holder_PETG.gcode");

	JobsList::SortEntries(e, Sort::Newest, true);
	EXPECT_EQ(e[0].name, "brackets");
	EXPECT_EQ(e[1].name, "spool_holder_PETG.gcode");

	JobsList::SortEntries(e, Sort::Name, true);
	EXPECT_EQ(e[1].name, "badge_dual_ASA.gcode");
	EXPECT_EQ(e.back().name, "temp_tower_PETG.gcode");

	JobsList::SortEntries(e, Sort::Filament, true);
	EXPECT_EQ(e[1].name, "benchy_0p2_PLA.gcode");			/* PLA first, then by name */
	EXPECT_EQ(e.back().name, "lamp_shade_vase_mode.gcode"); /* unknown material last */

	EXPECT_FALSE(JobsList::DefaultAscending(Sort::Newest));
	EXPECT_TRUE(JobsList::DefaultAscending(Sort::Name));
	EXPECT_EQ(JobsList::NextSort(Sort::Newest), Sort::Name);
	EXPECT_EQ(JobsList::NextSort(Sort::Name), Sort::Filament);
	EXPECT_EQ(JobsList::NextSort(Sort::Filament), Sort::Newest);
	EXPECT_STREQ(JobsList::SortName(Sort::Filament), "FILA");
}

TEST(JobsList, Date)
{
	const std::time_t now = testNow();
	EXPECT_EQ(JobsList::DateText("2026-10-18T09:05:00", now), "09:05");
	EXPECT_EQ(JobsList::DateText("2026-10-17T23:59:00", now), "17 Oct");
	EXPECT_EQ(JobsList::DateText("2026-01-02T10:00:00", now), "2 Jan");
	EXPECT_EQ(JobsList::DateText("2025-12-31T10:00:00", now), "31 Dec 25");
	EXPECT_EQ(JobsList::DateText("rubbish", now), "");
}

TEST(JobsList, DetailsText)
{
	EXPECT_EQ(JobsFolder::FormatPrintTime(0), "-");
	EXPECT_EQ(JobsFolder::FormatPrintTime(600), "10m");
	EXPECT_EQ(JobsFolder::FormatPrintTime(8040), "2h 14m");
	EXPECT_EQ(JobsFolder::FormatSaved("2026-10-18T14:02:00"), "18 Oct 2026, 14:02");
	EXPECT_EQ(JobsFolder::FormatSaved(""), "-");
}

/* ---------------- the folder ---------------- */

struct EnableWorkshopHomeForJobs
{
	EnableWorkshopHomeForJobs() { StorageHelper::setData(ID_UI_WORKSHOP_HOME, true); }
};

class TestJobsFolder : public UiTestSuite, protected EnableWorkshopHomeForJobs
{
  protected:
	TestJobsFolder()
	{
		ZoneScoped;
		UI::HomeView::setInstance(&view);
		view.show();
		view.getWorkshop().deactivate();
	}

	~TestJobsFolder() override
	{
		UI::HomeView::setInstance(nullptr);
		StorageHelper::setData(ID_UI_WORKSHOP_HOME, false);
	}

	JobsFolder& jobs() { return view.getJobsFolder(); }

	void open()
	{
		view.getWorkshop().open(UI::WorkshopLayout::Target::Jobs);
		jobs().deactivate(); /* driven by the test */
		jobs().setSort(Sort::Newest, false);
		jobs().setFiles("", files());
	}

	static JobsFolder::Details engineBlock()
	{
		JobsFolder::Details d;
		d.valid = true;
		d.path = "0:/gcodes/engine_block_PETG.gcode";
		d.name = "engine_block_PETG.gcode";
		d.infoKnown = true;
		d.printTime = 8040;
		d.filamentMm = 5620;
		d.layerHeight = 0.2f;
		d.height = 42;
		d.slicer = "OrcaSlicer 2.3.1";
		d.saved = "2026-10-18T14:02:00";
		return d;
	}

	UI::HomeView view;
};

TEST_F(TestJobsFolder, OpensFromTheRackAndGoesHome)
{
	open();
	EXPECT_TRUE(jobs().isVisible());
	EXPECT_FALSE(view.getWorkshop().isVisible());
	jobs().pressHome();
	EXPECT_FALSE(jobs().isVisible());
	EXPECT_TRUE(view.getWorkshop().isVisible());
}

TEST_F(TestJobsFolder, NewestFirst)
{
	open();
	EXPECT_EQ(jobs().pageCount(), 2u);
	EXPECT_EQ(jobs().entries()[1].name, "engine_block_PETG.gcode");
	EXPECT_EQUAL_SCREENSHOT("jobs/newest.png");

	jobs().pressPage(1);
	EXPECT_EQ(jobs().page(), 1u);
	EXPECT_EQUAL_SCREENSHOT("jobs/newest_page2.png");
	jobs().pressPage(1);
	EXPECT_EQ(jobs().page(), 1u);
	jobs().pressPage(-1);
	EXPECT_EQ(jobs().page(), 0u);
}

TEST_F(TestJobsFolder, SortCycles)
{
	open();
	jobs().pressSort();
	EXPECT_EQ(jobs().sort(), Sort::Name);
	EXPECT_TRUE(jobs().ascending());
	EXPECT_EQ(jobs().entries()[1].name, "badge_dual_ASA.gcode");
	EXPECT_EQUAL_SCREENSHOT("jobs/name.png");

	jobs().pressDirection(false);
	EXPECT_FALSE(jobs().ascending());
	EXPECT_EQ(jobs().entries()[1].name, "temp_tower_PETG.gcode");

	jobs().pressSort();
	EXPECT_EQ(jobs().sort(), Sort::Filament);
	EXPECT_TRUE(jobs().ascending());
	EXPECT_EQUAL_SCREENSHOT("jobs/filament.png");

	jobs().pressSort();
	EXPECT_EQ(jobs().sort(), Sort::Newest);
	EXPECT_FALSE(jobs().ascending());
}

TEST_F(TestJobsFolder, Details)
{
	open();
	jobs().pressRow(1); /* engine_block_PETG, loading */
	EXPECT_EQ(jobs().selectedPath(), "0:/gcodes/engine_block_PETG.gcode");
	EXPECT_EQUAL_SCREENSHOT("jobs/details_loading.png");

	jobs().setDetails(engineBlock());
	EXPECT_EQUAL_SCREENSHOT("jobs/details.png");

	/* A slicer thumbnail (transparent background) on the old-photo grey */
	auto withThumb = engineBlock();
	withThumb.thumbnail = "tests/fixtures/thumbnail_part.png";
	jobs().setDetails(withThumb);
	EXPECT_EQUAL_SCREENSHOT("jobs/details_thumbnail.png");

	/* Gone from the list: nothing selected */
	auto list = files();
	list.erase(list.begin() + 1);
	jobs().setFiles("", list);
	EXPECT_EQ(jobs().selectedPath(), "");
}

TEST_F(TestJobsFolder, PrintAsksFirst)
{
	open();
	jobs().pressPrint(); /* nothing selected */
	EXPECT_FALSE(jobs().popup().isOpen());

	jobs().setDetails(engineBlock());
	jobs().pressPrint();
	ASSERT_TRUE(jobs().popup().isOpen());
	EXPECT_EQ(jobs().popup().title(), "START PRINT");
	EXPECT_TRUE(jobs().popup().hasTrash());
	EXPECT_EQUAL_SCREENSHOT("jobs/start_popup.png");
	jobs().popup().pressCancel();
	EXPECT_FALSE(jobs().popup().isOpen());

	/* The bin asks again before deleting */
	jobs().pressPrint();
	jobs().popup().pressTrash();
	ASSERT_TRUE(jobs().popup().isOpen());
	EXPECT_EQ(jobs().popup().title(), "DELETE FILE");
	EXPECT_FALSE(jobs().popup().hasTrash());
	EXPECT_EQUAL_SCREENSHOT("jobs/delete_popup.png");
	jobs().popup().pressCancel();
	EXPECT_FALSE(jobs().popup().isOpen());
}

TEST_F(TestJobsFolder, Empty)
{
	open();
	jobs().setFiles("", {});
	EXPECT_EQ(jobs().pageCount(), 1u);
	EXPECT_EQUAL_SCREENSHOT("jobs/empty.png");
}

TEST_F(TestJobsFolder, SubFolder)
{
	open();
	Entry up;
	up.name = "..";
	up.folder = true;
	up.up = true;
	jobs().setFiles("brackets/",
					{up,
					 file("shelf_bracket_PETG.gcode", "2026-10-18T10:00:00"),
					 file("wall_hook_PLA.gcode", "2026-10-12T10:00:00")});
	EXPECT_EQ(jobs().entries()[0].name, "..");
	EXPECT_EQUAL_SCREENSHOT("jobs/subfolder.png");
}

TEST_F(TestJobsFolder, PrintInProgress)
{
	open();
	EXPECT_TRUE(JobsFolder::SamePath("0:/gcodes/a.gcode", "/gcodes/a.gcode"));
	EXPECT_FALSE(JobsFolder::SamePath("0:/gcodes/a.gcode", "0:/gcodes/b.gcode"));

	jobs().setPrinting(true, false, "0:/gcodes/engine_block_PETG.gcode");
	JobsFolder::Details other = engineBlock();
	other.path = "0:/gcodes/calibration_cube_PLA.gcode";
	other.name = "calibration_cube_PLA.gcode";
	jobs().setDetails(other);
	EXPECT_EQUAL_SCREENSHOT("jobs/printing.png");

	/* Another file: no check, the bin still deletes (after asking) */
	jobs().pressPrint();
	ASSERT_TRUE(jobs().popup().isOpen());
	EXPECT_EQ(jobs().popup().title(), "PRINT IN PROGRESS");
	EXPECT_TRUE(jobs().popup().hasTrash());
	EXPECT_EQUAL_SCREENSHOT("jobs/busy_popup.png");
	jobs().popup().pressOk(); /* no check button: nothing happens */
	EXPECT_TRUE(jobs().popup().isOpen());
	jobs().popup().pressTrash();
	ASSERT_TRUE(jobs().popup().isOpen());
	EXPECT_EQ(jobs().popup().title(), "DELETE FILE");
	jobs().popup().pressCancel();

	/* The file being printed: X only */
	jobs().setDetails(engineBlock());
	jobs().pressPrint();
	ASSERT_TRUE(jobs().popup().isOpen());
	EXPECT_EQ(jobs().popup().title(), "PRINT IN PROGRESS");
	EXPECT_FALSE(jobs().popup().hasTrash());
	EXPECT_EQUAL_SCREENSHOT("jobs/busy_same_popup.png");
	jobs().popup().pressCancel();

	/* Finished: START PRINT again */
	jobs().setPrinting(false, false, "");
	jobs().pressPrint();
	EXPECT_EQ(jobs().popup().title(), "START PRINT");
	jobs().popup().pressCancel();
}
