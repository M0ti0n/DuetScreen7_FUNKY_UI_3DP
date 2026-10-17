/*
 * test_workshop.cpp
 *
 * Renders the workshop home scene in its main states.
 */

#include "Debug.h"
#include "UI/Core/Navigation.h"
#include "UI/Screens/Home/HomeView.h"
#include "UI/Widgets/Workshop/Workshop.h"
#include "test_utils/UiTestSuite.h"
#include "utils/StorageHelper.h"
#include <cstdlib>
#include <gtest/gtest.h>

/* Runs after UiTestSuite (which selects the classic home) and before the view is built */
struct EnableWorkshopHome
{
	EnableWorkshopHome() { StorageHelper::setData(ID_UI_WORKSHOP_HOME, true); }
};

class TestWorkshop : public UiTestSuite, protected EnableWorkshopHome
{
  protected:
	TestWorkshop()
	{
		ZoneScoped;
		UI::HomeView::setInstance(&view);
		std::srand(1);
		view.show();
		/* Drive the scene from the test, not from the (empty) object model */
		view.getWorkshop().deactivate();
	}

	~TestWorkshop() override
	{
		UI::HomeView::setInstance(nullptr);
		StorageHelper::setData(ID_UI_WORKSHOP_HOME, false);
	}

	static UI::Workshop::State idleState()
	{
		UI::Workshop::State s;
		s.connected = true;
		s.toolName = "T0";
		s.nozzle = 24;
		s.bed = 23;
		s.chamber = 21;
		s.hasNozzle = s.hasBed = s.hasChamber = true;
		return s;
	}

	UI::HomeView view;
};

TEST_F(TestWorkshop, Idle)
{
	view.getWorkshop().update(idleState());
	EXPECT_EQUAL_SCREENSHOT("workshop/idle.png");
}

TEST_F(TestWorkshop, Offline)
{
	UI::Workshop::State s;
	view.getWorkshop().update(s);
	EXPECT_EQUAL_SCREENSHOT("workshop/offline.png");
}

TEST_F(TestWorkshop, Printing)
{
	auto s = idleState();
	s.printing = true;
	s.jobKey = 1;
	s.nozzle = 215;
	s.bed = 60;
	s.timeLeft = 3 * 3600 + 23 * 60;

	s.progress = 0.1f;
	view.getWorkshop().update(s);
	EXPECT_EQUAL_SCREENSHOT("workshop/printing_10.png");

	s.progress = 0.42f;
	view.getWorkshop().update(s);
	EXPECT_EQUAL_SCREENSHOT("workshop/printing_42.png");

	s.progress = 1.0f;
	s.timeLeft = 0;
	view.getWorkshop().update(s);
	EXPECT_EQUAL_SCREENSHOT("workshop/printing_100.png");

	s.paused = true;
	s.progress = 0.42f;
	view.getWorkshop().update(s);
	EXPECT_EQUAL_SCREENSHOT("workshop/paused_42.png");
}

TEST_F(TestWorkshop, OpensPagesAndReturnsHome)
{
	view.getWorkshop().update(idleState());

	view.getWorkshop().open(UI::WorkshopLayout::Target::Motion);
	EXPECT_FALSE(view.getWorkshop().isVisible());
	EXPECT_TRUE(view.getControlView().isVisible());
	EXPECT_EQUAL_SCREENSHOT("workshop/open_motion.png");

	UI::home();
	EXPECT_TRUE(view.getWorkshop().isVisible());
	EXPECT_FALSE(view.getControlView().isVisible());

	view.getWorkshop().open(UI::WorkshopLayout::Target::BedLevelling);
	EXPECT_TRUE(view.getControlView().isVisible());
	UI::home();

	view.getWorkshop().open(UI::WorkshopLayout::Target::PrintStatus);
	EXPECT_TRUE(view.getDashboard().isVisible());
	EXPECT_EQUAL_SCREENSHOT("workshop/open_print_status.png");
	UI::home();
	EXPECT_TRUE(view.getWorkshop().isVisible());
}
