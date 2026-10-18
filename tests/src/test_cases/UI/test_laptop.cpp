/*
 * test_laptop.cpp
 *
 * The laptop page: console with the keyboard, alerts, settings and their popups.
 */

#include "Debug.h"
#include "Storage.h"
#include "UI/Core/Navigation.h"
#include "UI/Screens/Home/HomeView.h"
#include "UI/Widgets/Laptop/Laptop.h"
#include "UI/Widgets/Laptop/LaptopSettings.h"
#include "test_utils/UiTestSuite.h"
#include "utils/StorageHelper.h"
#include <chrono>
#include <gtest/gtest.h>
#include <string>
#include <vector>

using UI::Laptop;
using UI::LaptopLayout::KeyKind;
using UI::LaptopLog::AlertList;

/* Runs after UiTestSuite (which selects the classic home) and before the view is built */
struct EnableWorkshopHomeForLaptop
{
	EnableWorkshopHomeForLaptop()
	{
		StorageHelper::setData(ID_UI_WORKSHOP_HOME, true);
		/* Known settings for the screenshots */
		StorageHelper::setData(ID_SYS_BRIGHTNESS_KEY, 80u);
		StorageHelper::setData(ID_SCREENSAVER_TIMEOUT, std::chrono::seconds(300));
		StorageHelper::setData(ID_DISPLAY_ROTATION, DisplayRotation::ROTATION_0);
	}
};

class TestLaptop : public UiTestSuite, protected EnableWorkshopHomeForLaptop
{
  protected:
	TestLaptop()
	{
		ZoneScoped;
		UI::HomeView::setInstance(&view);
		view.show();
		view.getWorkshop().deactivate();
		UI::LaptopSettings::SetTestHook([this](const std::string& cmd) { commands.push_back(cmd); });
		UI::LaptopSettings::SetSoundSetting(3);
		UI::LaptopSettings::SetDrawerIndicator(true);
		commands.clear();
		laptop().setClock([this] { return clock; });
	}

	~TestLaptop() override
	{
		UI::LaptopSettings::SetTestHook(nullptr);
		UI::HomeView::setInstance(nullptr);
		StorageHelper::setData(ID_UI_WORKSHOP_HOME, false);
	}

	Laptop& laptop() { return view.getLaptop(); }

	void open()
	{
		view.getWorkshop().open(UI::WorkshopLayout::Target::Console);
		laptop().deactivate(); /* driven by the test */
	}

	/* A log that looks like the mock-up: messages over the last 42 minutes */
	void fillConsole()
	{
		clock = 1000;
		laptop().response(false, "PanelDue connected to printer");
		clock = 2000;
		laptop().response(false, "Tool 0 active");
		clock = 40000;
		laptop().response(false, "Heater 1 set to active temperature 215C");
		clock = 76000;
		laptop().response(false, "Bed heater set to 60C");
		clock = 105000;
		laptop().response(false, "Warning: Driver 2 warning: phase A may be disconnected");
		laptop().consoleLog().addCommand("G28", 170000);
		laptop().consoleLog().addCommand("G1 X125 Y87.5 Z12.4 F12000", 187000);
		clock = 190000;
		laptop().response(true, "Error: G0/G1: target position outside machine limits, please home the printer first");
		laptop().consoleLog().addCommand("M32 \"calibration_cube.gcode\"", 223000);
		clock = 224000;
		laptop().response(false, "Print started");
		clock = 2531000;
	}

	void fillAlerts()
	{
		clock = 1000;
		laptop().info("Connected to the printer");
		clock = 60000;
		laptop().response(true, "Error: Z probe was not triggered during probing move");
		clock = 70000;
		laptop().info("Print started: calibration_cube.gcode");
		clock = 90000;
		laptop().response(false, "Warning: Driver 2 warning: phase A may be disconnected");
		clock = 95000;
		laptop().info("Print paused");
		clock = 600000;
		laptop().response(false, "Warning: Filament runout on T0");
		clock = 1000000;
		laptop().response(true, "Error: Heater 1 fault: temperature rising too slowly. Expected 1.80C/sec, measured "
								"0.42C/sec. Heaters were switched off.");
		clock = 1041000;
	}

	UI::HomeView view;
	uint32_t clock = 0;
	std::vector<std::string> commands;
};

TEST_F(TestLaptop, OpensFromWorkshopAndGoesHome)
{
	open();
	EXPECT_TRUE(laptop().isVisible());
	EXPECT_FALSE(view.getWorkshop().isVisible());
	EXPECT_EQ(laptop().tab(), Laptop::Tab::Console);

	laptop().pressHome();
	EXPECT_FALSE(laptop().isVisible());
	EXPECT_TRUE(view.getWorkshop().isVisible());
}

TEST_F(TestLaptop, Console)
{
	open();
	fillConsole();
	laptop().showTab(Laptop::Tab::Console);
	EXPECT_EQUAL_SCREENSHOT("laptop/console.png");
}

TEST_F(TestLaptop, KeyboardTypesAndSends)
{
	open();
	fillConsole();
	laptop().openKeyboard();
	EXPECT_TRUE(laptop().keyboardOpen());
	laptop().type("G1 X125 Y87.5 F6000");
	EXPECT_EQ(laptop().inputText(), "G1 X125 Y87.5 F6000");
	EXPECT_EQUAL_SCREENSHOT("laptop/keyboard.png");

	laptop().pressKey(KeyKind::Backspace);
	EXPECT_EQ(laptop().inputText(), "G1 X125 Y87.5 F600");

	/* Shift: lower case and symbols */
	laptop().pressKey(KeyKind::Shift);
	EXPECT_TRUE(laptop().shift());
	EXPECT_EQUAL_SCREENSHOT("laptop/keyboard_shift.png");
	laptop().pressKey(KeyKind::Shift);

	/* ENTER sends, logs and keeps the keyboard open */
	laptop().pressKey(KeyKind::Enter);
	EXPECT_EQ(laptop().inputText(), "");
	EXPECT_TRUE(laptop().keyboardOpen());
	ASSERT_FALSE(laptop().consoleLog().history().empty());
	EXPECT_EQ(laptop().consoleLog().history().front(), "G1 X125 Y87.5 F600");
	EXPECT_EQ(laptop().consoleLog().lines().back().text, "G1 X125 Y87.5 F600");

	/* History */
	laptop().type("M1");
	laptop().pressKey(KeyKind::Up);
	EXPECT_EQ(laptop().inputText(), "G1 X125 Y87.5 F600");
	laptop().pressKey(KeyKind::Up);
	EXPECT_EQ(laptop().inputText(), "M32 \"calibration_cube.gcode\"");
	laptop().pressKey(KeyKind::Down);
	laptop().pressKey(KeyKind::Down);
	EXPECT_EQ(laptop().inputText(), "M1");

	/* Typing symbols switches shift as needed */
	laptop().pressKey(KeyKind::Backspace);
	laptop().pressKey(KeyKind::Backspace);
	laptop().type("M98 P\"homeall.g\"");
	EXPECT_EQ(laptop().inputText(), "M98 P\"homeall.g\"");

	laptop().pressKey(KeyKind::Hide);
	EXPECT_FALSE(laptop().keyboardOpen());
	EXPECT_EQ(laptop().inputText(), "M98 P\"homeall.g\""); /* the unsent text stays */
}

TEST_F(TestLaptop, Alerts)
{
	open();
	fillAlerts();
	laptop().showTab(Laptop::Tab::Alerts);
	AlertList& alerts = laptop().alertList();
	EXPECT_EQ(alerts.count(), 7u);
	EXPECT_TRUE(alerts.anyUnread());
	EXPECT_EQUAL_SCREENSHOT("laptop/alerts.png");

	/* Filter ALL -> ERR -> WRN -> INFO -> ALL */
	laptop().pressAlertFilter();
	EXPECT_EQ(alerts.filter(), AlertList::Filter::Error);
	EXPECT_EQ(alerts.rowIndex(0), 0);
	EXPECT_EQ(alerts.rowIndex(1), 5);
	EXPECT_EQ(alerts.rowIndex(2), -1);
	EXPECT_EQUAL_SCREENSHOT("laptop/alerts_err.png");
	laptop().pressAlertFilter();
	laptop().pressAlertFilter();
	laptop().pressAlertFilter();
	EXPECT_EQ(alerts.filter(), AlertList::Filter::All);

	/* Tap a row: the whole text; the row is read */
	laptop().pressAlertRow(0);
	EXPECT_TRUE(laptop().popup().isOpen());
	EXPECT_EQ(laptop().popup().title(), "ERROR");
	EXPECT_FALSE(alerts.entry(0).unread);
	EXPECT_EQUAL_SCREENSHOT("laptop/alert_error_popup.png");
	laptop().popup().pressCancel();
	EXPECT_FALSE(laptop().popup().isOpen());
}

TEST_F(TestLaptop, AlertsKeep21)
{
	open();
	for (uint32_t i = 0; i < 25; i++)
	{
		clock = i * 1000;
		laptop().response(false, "Warning: number " + std::to_string(i));
	}
	AlertList& alerts = laptop().alertList();
	EXPECT_EQ(alerts.count(), 21u);
	EXPECT_EQ(alerts.entry(0).text, "number 24");
	EXPECT_EQ(alerts.entry(20).text, "number 4");
	EXPECT_EQ(alerts.pageCount(), 3u);
	laptop().pressAlertPage(1);
	laptop().pressAlertPage(1);
	laptop().pressAlertPage(1);
	EXPECT_EQ(alerts.page(), 2u);

	/* The same again only refreshes the newest */
	laptop().response(false, "Warning: number 24");
	EXPECT_EQ(alerts.count(), 21u);
}

TEST_F(TestLaptop, Settings)
{
	open();
	laptop().showTab(Laptop::Tab::Settings);
	EXPECT_EQUAL_SCREENSHOT("laptop/settings_system.png");

	/* BRIGHTNESS: presets, check applies */
	laptop().pressSetting(0);
	ASSERT_TRUE(laptop().popup().isOpen());
	EXPECT_EQ(laptop().popup().title(), "BRIGHTNESS");
	EXPECT_EQ(laptop().popup().choiceCount(), 6u);
	EXPECT_EQ(laptop().popup().pending(), 4u); /* 80 % */
	EXPECT_EQUAL_SCREENSHOT("laptop/popup_brightness.png");
	laptop().popup().pressChoice(1);
	laptop().popup().pressOk();
	EXPECT_FALSE(laptop().popup().isOpen());
	EXPECT_EQ(UI::LaptopSettings::Brightness(), 20u);

	/* SOUND: OFF / BUZZER / SPEAKER 1-5 */
	laptop().pressSetting(1);
	EXPECT_EQ(laptop().popup().choiceCount(), 7u);
	EXPECT_EQ(laptop().popup().pending(), 3u);
	EXPECT_EQUAL_SCREENSHOT("laptop/popup_sound.png");
	laptop().popup().pressChoice(0);
	laptop().popup().pressOk();
	EXPECT_EQ(UI::LaptopSettings::SoundSetting(), 0u);

	/* X closes without a change */
	laptop().pressSetting(2);
	laptop().popup().pressChoice(0);
	laptop().popup().pressCancel();
	EXPECT_EQ(UI::LaptopSettings::DimTimeout(), 300u);

	/* FACTORY RESET asks first */
	laptop().pressSetting(8);
	EXPECT_EQ(laptop().popup().title(), "ALERT !");
	EXPECT_EQUAL_SCREENSHOT("laptop/popup_factory_reset.png");
	laptop().popup().pressOk();
	EXPECT_EQ(UI::LaptopSettings::Brightness(), 100u);
	EXPECT_EQ(UI::LaptopSettings::SoundSetting(), 1u);

	/* Page 2 */
	laptop().pressSettingsPage(1);
	EXPECT_EQ(laptop().settingsPage(), 1u);
	EXPECT_EQUAL_SCREENSHOT("laptop/settings_personalization.png");
	laptop().pressSetting(0);
	laptop().popup().pressChoice(1);
	laptop().popup().pressOk();
	EXPECT_FALSE(UI::LaptopSettings::DrawerIndicator());
}

TEST_F(TestLaptop, ConnectionNetworkTypesTheAddress)
{
	open();
	laptop().showTab(Laptop::Tab::Settings);
	laptop().pressSetting(4);
	EXPECT_EQ(laptop().popup().title(), "CONNECTION");
	laptop().popup().pressChoice(2);
	laptop().popup().pressOk();
	EXPECT_TRUE(laptop().keyboardOpen());
	for (size_t i = 0; i < 20; i++)
	{
		laptop().pressKey(KeyKind::Backspace);
	}
	laptop().type("192.168.1.50");
	EXPECT_EQUAL_SCREENSHOT("laptop/keyboard_ip.png");
	laptop().pressKey(KeyKind::Enter);
	EXPECT_FALSE(laptop().keyboardOpen());
	ASSERT_FALSE(commands.empty());
	EXPECT_EQ(commands.back(), "connection 2 192.168.1.50");
	EXPECT_EQ(laptop().tab(), Laptop::Tab::Settings);
}

TEST_F(TestLaptop, ConnectionLockedDuringAPrint)
{
	open();
	laptop().setJobActive(true);
	laptop().showTab(Laptop::Tab::Settings);
	laptop().pressSetting(4);
	EXPECT_EQ(laptop().popup().title(), "CONNECTION");
	EXPECT_EQ(laptop().popup().choiceCount(), 0u); /* a message, not the choices */
}
