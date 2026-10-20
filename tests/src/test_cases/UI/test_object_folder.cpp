/*
 * test_object_folder.cpp
 *
 * The object cancel folder: bed, list, pages, selection and the cancel popup.
 */

#include "Debug.h"
#include "UI/Core/Navigation.h"
#include "UI/Screens/Home/HomeView.h"
#include "UI/Widgets/ObjectFolder/ObjectFolder.h"
#include "test_utils/UiTestSuite.h"
#include "utils/StorageHelper.h"
#include <gtest/gtest.h>

using UI::ObjectFolder;

struct EnableWorkshopHomeForFolder
{
	EnableWorkshopHomeForFolder() { StorageHelper::setData(ID_UI_WORKSHOP_HOME, true); }
};

class TestObjectFolder : public UiTestSuite, protected EnableWorkshopHomeForFolder
{
  protected:
	TestObjectFolder()
	{
		ZoneScoped;
		UI::HomeView::setInstance(&view);
		view.show();
		view.getWorkshop().deactivate();
	}

	~TestObjectFolder() override
	{
		UI::HomeView::setInstance(nullptr);
		StorageHelper::setData(ID_UI_WORKSHOP_HOME, false);
	}

	ObjectFolder& folder() { return view.getObjectFolder(); }

	void open()
	{
		view.getWorkshop().open(UI::WorkshopLayout::Target::ObjectCancel);
		folder().deactivate(); /* driven by the test */
	}

	static ObjectFolder::State printing()
	{
		ObjectFolder::State s;
		s.printing = true;
		s.job = "engine_block_v8.gcode";
		s.hasBed = true;
		s.xMin = 0;
		s.xMax = 300;
		s.yMin = 0;
		s.yMax = 300;
		const struct
		{
			const char* name;
			float x, y;
		} objs[] = {{"Front left clip", 66, 75},
					{"Front right clip", 216, 81},
					{"Rear bracket", 165, 204},
					{"Cable guide", 75, 234},
					{"Sensor cover", 234, 225},
					{"Fan duct", 135, 126},
					{"Spool hub", 258, 144},
					{"Knob", 36, 150}};
		for (const auto& o : objs)
		{
			ObjectFolder::Object obj;
			obj.present = true;
			obj.name = o.name;
			obj.hasPosition = true;
			obj.x = o.x;
			obj.y = o.y;
			s.objects.push_back(obj);
		}
		s.objects[3].cancelled = true;
		s.current = 2;
		return s;
	}

	UI::HomeView view;
};

TEST_F(TestObjectFolder, OpensFromTheSpareBedAndGoesHome)
{
	open();
	EXPECT_TRUE(folder().isVisible());
	EXPECT_FALSE(view.getWorkshop().isVisible());
	folder().pressHome();
	EXPECT_FALSE(folder().isVisible());
	EXPECT_TRUE(view.getWorkshop().isVisible());
}

TEST_F(TestObjectFolder, Printing)
{
	open();
	folder().update(printing());
	EXPECT_EQ(folder().selected(), 2); /* follows the object being printed */
	EXPECT_EQ(folder().pageCount(), 2u);
	EXPECT_EQUAL_SCREENSHOT("objects/printing.png");

	folder().pressPage(1);
	EXPECT_EQ(folder().page(), 1u);
	EXPECT_EQUAL_SCREENSHOT("objects/printing_page2.png");
	folder().pressPage(1);
	EXPECT_EQ(folder().page(), 1u);

	/* A box on the bed selects its object and turns to its page */
	folder().pressMarker(1);
	EXPECT_EQ(folder().selected(), 1);
	EXPECT_EQ(folder().page(), 0u);

	/* A name selects; the selection stays until another object is printed */
	folder().pressName(4);
	EXPECT_EQ(folder().selected(), 4);
	folder().update(printing());
	EXPECT_EQ(folder().selected(), 4);
	auto s = printing();
	s.current = 6;
	folder().update(s);
	EXPECT_EQ(folder().selected(), 6);
	EXPECT_EQ(folder().page(), 1u);
}

TEST_F(TestObjectFolder, CancelAsksFirst)
{
	open();
	folder().update(printing());

	/* A cancelled object can't be cancelled again */
	folder().pressNumber(3);
	EXPECT_FALSE(folder().popup().isOpen());

	folder().pressNumber(2);
	ASSERT_TRUE(folder().popup().isOpen());
	EXPECT_EQ(folder().popup().title(), "CANCEL OBJECT");
	EXPECT_EQUAL_SCREENSHOT("objects/cancel_popup.png");
	folder().popup().pressCancel();
	EXPECT_FALSE(folder().popup().isOpen());

	folder().pressNumber(2);
	folder().popup().pressOk(); /* M486 P2 (not connected in the tests) */
	EXPECT_FALSE(folder().popup().isOpen());
}

TEST_F(TestObjectFolder, NothingPrinting)
{
	open();
	folder().update(ObjectFolder::State{});
	EXPECT_EQ(folder().selected(), -1);
	EXPECT_EQ(folder().pageCount(), 1u);
	EXPECT_EQUAL_SCREENSHOT("objects/idle.png");
}

TEST_F(TestObjectFolder, MarkersCloseTogetherMoveApart)
{
	open();
	auto s = printing();
	for (auto& o : s.objects)
	{
		o.x = 150;
		o.y = 150;
	}
	folder().update(s);
	EXPECT_EQUAL_SCREENSHOT("objects/crowded.png");
}

TEST_F(TestObjectFolder, RoundBedForADelta)
{
	open();
	auto s = printing();
	s.roundBed = true;
	s.xMin = s.yMin = -150;
	s.xMax = s.yMax = 150;
	for (auto& o : s.objects)
	{
		o.x -= 150;
		o.y -= 150;
	}
	s.objects[0].x = -148; /* outside the circle: kept inside */
	s.objects[0].y = -148;
	folder().update(s);
	EXPECT_EQUAL_SCREENSHOT("objects/round_bed.png");
}
