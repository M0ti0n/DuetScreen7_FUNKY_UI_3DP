/*
 * test_filament_box.cpp
 *
 * The filament page (MMU box): bays and screens, pages, PANEL and MACROS flows, the G-code.
 */

#include "Debug.h"
#include "UI/Core/Navigation.h"
#include "UI/Screens/Home/HomeView.h"
#include "UI/Widgets/FilamentBox/FilamentBox.h"
#include "UI/Widgets/FilamentBox/FilamentModel.h"
#include "test_utils/UiTestSuite.h"
#include "utils/StorageHelper.h"
#include <gtest/gtest.h>

using UI::FilamentBox;
namespace FM = UI::FilamentModel;

/* ---------------- the model ---------------- */

TEST(FilamentModel, Commands)
{
	using V = std::vector<std::string>;
	EXPECT_EQ(FM::ChangeCommands(1, 0, true, "PETG"), (V{"T1", "M702", "M701 S\"PETG\""}));
	EXPECT_EQ(FM::ChangeCommands(0, 0, false, "PLA"), (V{"M701 S\"PLA\""}));
	EXPECT_EQ(FM::UnloadCommands(2, 0), (V{"T2", "M702"}));
	EXPECT_EQ(FM::UnloadCommands(0, 0), (V{"M702"}));
}

TEST(FilamentModel, MacroMaterialsAndPages)
{
	using V = std::vector<std::string>;
	EXPECT_EQ(FM::MacroMaterials({"Silk", "PETG", "PLA"}), (V{"PLA", "PETG", "Silk"}));
	EXPECT_EQ(FM::PageCount(0), 1u);
	EXPECT_EQ(FM::PageCount(4), 1u);
	EXPECT_EQ(FM::PageCount(5), 2u);
	EXPECT_EQ(FM::PageCount(9), 3u);
}

TEST(FilamentModel, SlotsAreKept)
{
	FM::SetSlotMaterial(3, 1);
	FM::SetSlotColour(3, 11);
	EXPECT_EQ(FM::SlotMaterial(3), 1);
	EXPECT_EQ(FM::SlotColour(3), 11);
	FM::SetSlotMaterial(3, FM::NONE);
	FM::SetSlotColour(3, FM::NONE);
	EXPECT_EQ(FM::SlotMaterial(3), FM::NONE);
	EXPECT_EQ(FM::SlotColour(3), FM::NONE);
}

/* ---------------- the page ---------------- */

struct EnableWorkshopHomeForFilament
{
	EnableWorkshopHomeForFilament() { StorageHelper::setData(ID_UI_WORKSHOP_HOME, true); }
};

class TestFilamentBox : public UiTestSuite, protected EnableWorkshopHomeForFilament
{
  protected:
	TestFilamentBox()
	{
		ZoneScoped;
		reset();
		UI::HomeView::setInstance(&view);
		view.show();
		view.getWorkshop().deactivate();
		UI::FilamentBoxPresenter::SetSendHook([this](const std::string& c) { sent.push_back(c); });
	}

	~TestFilamentBox() override
	{
		UI::FilamentBoxPresenter::SetSendHook(nullptr);
		reset();
		UI::HomeView::setInstance(nullptr);
		StorageHelper::setData(ID_UI_WORKSHOP_HOME, false);
	}

	static void reset()
	{
		FM::SetMode(FM::Mode::Panel);
		for (size_t t = 0; t < 8; t++)
		{
			FM::SetSlotMaterial(t, FM::NONE);
			FM::SetSlotColour(t, FM::NONE);
		}
	}

	FilamentBox& box() { return view.getFilamentBox(); }

	void open()
	{
		view.getWorkshop().open(UI::WorkshopLayout::Target::Filament);
		box().deactivate(); /* driven by the test */
	}

	static FilamentBox::State tools(size_t n, float temp = 215.0f)
	{
		FilamentBox::State s;
		s.connected = true;
		s.currentTool = 0;
		for (size_t i = 0; i < n; i++)
		{
			FilamentBox::Slot slot;
			slot.tool = static_cast<int>(i);
			slot.hasTemp = true;
			slot.temp = temp;
			s.slots.push_back(slot);
		}
		return s;
	}

	/* PANEL: T0 orange PLA (extruding), T1 blue PETG, T2 green PETG-CF (retracting), T3 empty sensor */
	static FilamentBox::State fourTools()
	{
		FM::SetSlotMaterial(0, FM::FindMaterial("PLA"));
		FM::SetSlotColour(0, 5);
		FM::SetSlotMaterial(1, FM::FindMaterial("PETG"));
		FM::SetSlotColour(1, 11);
		FM::SetSlotMaterial(2, FM::FindMaterial("PETG-CF"));
		FM::SetSlotColour(2, 40);
		auto s = tools(4);
		s.slots[0].temp = 215.0f;
		s.slots[0].activity = FilamentBox::Activity::Extruding;
		s.slots[1].temp = 240.1f;
		s.slots[2].temp = 251.9f;
		s.slots[2].activity = FilamentBox::Activity::Retracting;
		s.slots[3].temp = 24.3f;
		s.slots[3].runout = true;
		return s;
	}

	UI::HomeView view;
	std::vector<std::string> sent;
};

TEST_F(TestFilamentBox, OpensFromTheSpoolsAndGoesHome)
{
	open();
	EXPECT_TRUE(box().isVisible());
	EXPECT_FALSE(view.getWorkshop().isVisible());
	box().pressHome();
	EXPECT_FALSE(box().isVisible());
	EXPECT_TRUE(view.getWorkshop().isVisible());
}

TEST_F(TestFilamentBox, FourToolsPanel)
{
	open();
	box().update(fourTools());
	EXPECT_EQ(box().pageCount(), 1u);
	EXPECT_EQUAL_SCREENSHOT("filament/panel_4.png");
}

TEST_F(TestFilamentBox, PagesAndUpgrades)
{
	open();
	auto s = tools(6);
	FM::SetSlotMaterial(4, 0);
	FM::SetSlotColour(4, 4);
	box().update(s);
	EXPECT_EQ(box().pageCount(), 2u);
	EXPECT_EQUAL_SCREENSHOT("filament/pages_1.png");
	box().pressPage(1);
	EXPECT_EQ(box().page(), 1u);
	EXPECT_EQUAL_SCREENSHOT("filament/pages_2.png");
	box().pressPage(1);
	EXPECT_EQ(box().page(), 1u);
	box().pressPage(-1);
	EXPECT_EQ(box().page(), 0u);
}

TEST_F(TestFilamentBox, SingleFilament)
{
	open();
	FM::SetSlotMaterial(0, FM::FindMaterial("PETG"));
	FM::SetSlotColour(0, 40);
	box().update(tools(1, 240.1f));
	EXPECT_EQUAL_SCREENSHOT("filament/single.png");
}

TEST_F(TestFilamentBox, PanelPicksMaterialThenColour)
{
	open();
	box().update(fourTools());
	box().pressMaterial(1);
	ASSERT_TRUE(box().popup().isOpen());
	EXPECT_EQ(box().popup().title(), "FILAMENT T1");
	EXPECT_EQ(box().popup().pickSelected(), FM::FindMaterial("PETG"));
	EXPECT_TRUE(box().popup().hasExtra()); /* CLEAR */
	EXPECT_EQUAL_SCREENSHOT("filament/material_popup.png");

	box().popup().pressChoice(2); /* ABS */
	box().popup().pressOk();
	ASSERT_TRUE(box().popup().isOpen());
	EXPECT_EQ(box().popup().title(), "COLOR T1");
	EXPECT_EQUAL_SCREENSHOT("filament/colour_popup.png");
	box().popup().pressPickPage(1);
	EXPECT_EQ(box().popup().pickPage(), 2u);
	box().popup().pressChoice(0); /* colour 16 */
	box().popup().pressOk();
	EXPECT_FALSE(box().popup().isOpen());
	EXPECT_EQ(FM::SlotMaterial(1), FM::FindMaterial("ABS"));
	EXPECT_EQ(FM::SlotColour(1), 16);
	EXPECT_TRUE(sent.empty()); /* PANEL sends nothing */

	/* CLEAR empties the slot */
	box().pressMaterial(1);
	box().popup().pressExtra();
	EXPECT_EQ(FM::SlotMaterial(1), FM::NONE);
}

TEST_F(TestFilamentBox, PanelExtrudeIsBlockedWhilePrinting)
{
	open();
	auto s = fourTools();
	s.printing = true;
	box().update(s);
	box().pressButton(0);
	ASSERT_TRUE(box().popup().isOpen());
	EXPECT_EQ(box().popup().title(), "FILAMENT");
	box().popup().pressCancel();
}

TEST_F(TestFilamentBox, MacrosChangeLoadsWithM701)
{
	FM::SetMode(FM::Mode::Macros);
	open();
	auto s = tools(3);
	s.slots[0].material = "PLA";
	s.folders = {"PLA", "PETG", "Silk"};
	box().update(s);
	EXPECT_EQUAL_SCREENSHOT("filament/macros.png");

	/* the material row only picks the colour */
	box().pressMaterial(0);
	EXPECT_EQ(box().popup().title(), "COLOR T0");
	box().popup().pressCancel();

	/* CHANGE: material, colour (X keeps it), then T / M702 / M701 */
	box().pressButton(0);
	ASSERT_TRUE(box().popup().isOpen());
	EXPECT_EQ(box().popup().title(), "FILAMENT T0");
	EXPECT_EQ(box().popup().choiceCount(), 3u);
	box().popup().pressChoice(1); /* PETG */
	box().popup().pressOk();
	box().popup().pressCancel();
	EXPECT_EQ(sent, (std::vector<std::string>{"M702", "M701 S\"PETG\""}));

	/* LOAD on another tool: select it first, nothing to unload */
	sent.clear();
	box().pressButton(1);
	box().popup().pressChoice(2); /* Silk */
	box().popup().pressOk();
	box().popup().pressChoice(3);
	box().popup().pressOk();
	EXPECT_EQ(sent, (std::vector<std::string>{"T1", "M701 S\"Silk\""}));
	EXPECT_EQ(FM::SlotColour(1), 3);

	/* UNLOAD */
	sent.clear();
	box().pressButton(0);
	box().popup().pressExtra();
	EXPECT_EQ(sent, (std::vector<std::string>{"M702"}));
}

TEST_F(TestFilamentBox, MacrosLocks)
{
	FM::SetMode(FM::Mode::Macros);
	open();
	auto s = tools(2);
	s.printing = true;
	box().update(s);
	box().pressButton(0);
	EXPECT_EQ(box().popup().title(), "FILAMENT"); /* unavailable while printing */
	box().popup().pressCancel();

	s.printing = false;
	s.paused = true;
	box().update(s);
	box().pressButton(1); /* not the active tool */
	EXPECT_EQ(box().popup().title(), "FILAMENT");
	box().popup().pressCancel();
	box().pressButton(0); /* the active tool: allowed */
	EXPECT_EQ(box().popup().title(), "FILAMENT T0");
	box().popup().pressCancel();
	EXPECT_TRUE(sent.empty());
}
