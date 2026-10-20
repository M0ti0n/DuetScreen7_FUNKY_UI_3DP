/*
 * test_workshop.cpp
 *
 * Renders the workshop home scene in its main states.
 */

#include "Debug.h"
#include "UI/Core/Navigation.h"
#include "UI/Screens/Home/HomeView.h"
#include "UI/Widgets/Workshop/MachineActivity.h"
#include "UI/Widgets/Workshop/PrinterStyle.h"
#include "UI/Widgets/Workshop/Workshop.h"
#include "test_utils/UiTestSuite.h"
#include "utils/StorageHelper.h"
#include <cstdlib>
#include <gtest/gtest.h>

/* Runs after UiTestSuite (which selects the classic home) and before the view is built */
struct EnableWorkshopHome
{
	EnableWorkshopHome()
	{
		StorageHelper::setData(ID_UI_WORKSHOP_HOME, true);
		UI::PrinterStyle::SetSetting(UI::PrinterStyle::Setting::Auto);
	}
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
		UI::PrinterStyle::SetSetting(UI::PrinterStyle::Setting::Auto);
	}

	static UI::Workshop::State printingState(float progress)
	{
		auto s = idleState();
		s.printing = true;
		s.jobKey = 1;
		s.nozzle = 215;
		s.bed = 60;
		s.progress = progress;
		return s;
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

TEST_F(TestWorkshop, PrintGrowsInTenSteps)
{
	auto& w = view.getWorkshop();
	w.update(printingState(0.0f));
	EXPECT_EQ(w.printStage(), 0); /* only the red outline */
	EXPECT_EQUAL_SCREENSHOT("workshop/printing_0.png");
	w.update(printingState(0.49f));
	EXPECT_EQ(w.printStage(), 4);
	w.update(printingState(0.5f));
	EXPECT_EQ(w.printStage(), 5);
	w.update(printingState(1.0f));
	EXPECT_EQ(w.printStage(), 10);
	w.update(idleState());
	EXPECT_EQ(w.printStage(), -1);
}

TEST_F(TestWorkshop, DisplayModes)
{
	using Mode = UI::Workshop::Mode;
	auto& w = view.getWorkshop();
	auto s = idleState();
	EXPECT_EQ(UI::Workshop::ModeOf(s), Mode::Idle);

	s.busy = true;
	s.busyText = "G29";
	EXPECT_EQ(UI::Workshop::ModeOf(s), Mode::Busy);
	w.update(s);
	EXPECT_EQUAL_SCREENSHOT("workshop/busy_g29.png");

	s.busyText = "DWC";
	w.update(s);
	EXPECT_EQUAL_SCREENSHOT("workshop/busy_dwc.png");

	s.error = true;
	EXPECT_EQ(UI::Workshop::ModeOf(s), Mode::Error);
	w.update(s);
	EXPECT_EQUAL_SCREENSHOT("workshop/error.png");

	s = idleState();
	s.connected = false;
	EXPECT_EQ(UI::Workshop::ModeOf(s), Mode::Offline);
}

TEST_F(TestWorkshop, PrinterStyles)
{
	using UI::PrinterStyle::Setting;
	using UI::PrinterStyle::Style;
	auto& w = view.getWorkshop();
	EXPECT_EQ(UI::PrinterStyle::Current(), Style::CoreXY); /* AUTO, not a delta */

	UI::PrinterStyle::SetSetting(Setting::Delta);
	w.update(printingState(0.42f));
	EXPECT_EQ(w.style(), Style::Delta);
	EXPECT_TRUE(UI::PrinterStyle::RoundBed());
	EXPECT_EQUAL_SCREENSHOT("workshop/delta_printing_42.png");
	w.update(idleState());
	EXPECT_EQUAL_SCREENSHOT("workshop/delta_idle.png");

	UI::PrinterStyle::SetSetting(Setting::BedSlinger);
	w.update(printingState(0.42f));
	EXPECT_EQ(w.style(), Style::BedSlinger);
	EXPECT_FALSE(UI::PrinterStyle::RoundBed());
	EXPECT_EQUAL_SCREENSHOT("workshop/bedslinger_printing_42.png");
	w.update(idleState());
	EXPECT_EQUAL_SCREENSHOT("workshop/bedslinger_idle.png");

	UI::PrinterStyle::SetSetting(Setting::CoreXY);
	w.update(idleState());
	EXPECT_EQ(w.style(), Style::CoreXY);
}

TEST(MachineActivity, CommandCode)
{
	namespace A = UI::MachineActivity;
	EXPECT_EQ(A::CommandCode("G29 S1"), "G29");
	EXPECT_EQ(A::CommandCode("  g28"), "G28");
	EXPECT_EQ(A::CommandCode("M98 P\"macro.g\""), "MACRO");
	EXPECT_EQ(A::CommandCode("T1"), "T1");
	EXPECT_EQ(A::CommandCode("; comment"), "");
}

TEST(MachineActivity, Sources)
{
	namespace A = UI::MachineActivity;
	A::Reset();
	const uint32_t t0 = A::NowMs();
	A::RecordCommand("M409 K\"move\""); /* status requests don't count */
	EXPECT_EQ(A::LastScreenCommand(t0 - 1000), "");
	A::RecordCommand("G32");
	EXPECT_EQ(A::LastScreenCommand(t0 - 1000), "G32");

	EXPECT_EQ(A::BusySource(), "");
	A::SetChannel(0, "HTTP", "idle");
	A::SetChannel(9, "Daemon", "executing");
	EXPECT_EQ(A::BusySource(), "MACRO");
	A::SetChannel(0, nullptr, "executing");
	EXPECT_EQ(A::BusySource(), "DWC");
	A::Reset();
}
