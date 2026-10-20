#pragma once

#include "HomePresenter.h"
#include "UI/Components/Button/Button.h"
#include "UI/Components/Input/ModalNumberPad.h"
#include "UI/Components/Input/NumberPad.h"
#include "UI/Components/LVGL/LvKeyboard.h"
#include "UI/Components/MessageBox/AlertMessageBox.h"
#include "UI/Components/MessageBox/MessageBox.h"
#include "UI/Components/Screen/Screen.h"
#include "UI/Core/View.h"
#include "UI/Screens/Console/ConsoleView.h"
#include "UI/Screens/Control/ControlView.h"
#include "UI/Screens/Fan/FanView.h"
#include "UI/Screens/Heightmap/HeightmapView.h"
#include "UI/Screens/Move/MoveView.h"
#include "UI/Screens/ObjectCancel/ObjectCancelView.h"
#include "UI/Screens/Settings/SettingsView.h"
#include "UI/Screens/Status/FineTune.h"
#include "UI/Screens/Temperature/TemperatureView.h"
#include "UI/Widgets/Dashboard/Dashboard.h"
#include "UI/Widgets/SideBar/SideBar.h"
#include "UI/Widgets/StatusBar/StatusBar.h"
#include "UI/Widgets/Laptop/Laptop.h"
#include "UI/Widgets/FilamentBox/FilamentBox.h"
#include "UI/Widgets/JobsFolder/JobsFolder.h"
#include "UI/Widgets/ObjectFolder/ObjectFolder.h"
#include "UI/Widgets/Workshop/Workshop.h"
#include "lvgl/lvgl.h"
#include <list>
#include <memory>

namespace UI
{
	class HomeView : public View<HomePresenter, Screen>
	{
	  public:
		friend class HomePresenter;
		friend class HomeViewTest;

		static HomeView& instance();
		static void setInstance(HomeView* instance);

		void clear();

		std::shared_ptr<MessageBox>& createMessageBox();
		size_t getMessageBoxCount() const { return m_messageBoxList.size(); }
		std::shared_ptr<MessageBox> getMessageBox(size_t index) const;
		void popMessageBox();
		void clearMessageBoxes() { m_messageBoxList.clear(); }

		// Keyboard
		void showKeyboard(bool show);
		LvKeyboard& getKeyboard() { return m_kb; }

		// Sub-views
		Dashboard& getDashboard() { return m_dashboard; }
		Workshop& getWorkshop() { return m_workshop; }
		Laptop& getLaptop() { return m_laptop; }
		ObjectFolder& getObjectFolder() { return m_objectFolder; }
		JobsFolder& getJobsFolder() { return m_jobsFolder; }
		FilamentBox& getFilamentBox() { return m_filamentBox; }
		bool isWorkshopHome() const { return m_workshopHome; }
		StatusBar& getStatusBar() { return m_statusBar; }
		SideBar& getSideBar() { return m_sideBar; }
		LvContainer& getMainWindow() { return m_mainWindow; }
		ConsoleView& getConsoleView() { return m_consoleView; }
		ControlView& getControlView() { return m_controlView; }
		TabView& getFileView() { return m_files; }
		SettingsView& getSettingsView() { return m_settingsView; }
#if SIDE_BAR_APP_DRAWER
		MoveView& getMoveView() { return m_moveView; }
		TemperatureView& getTemperatureView() { return m_temperatureView; }
		FanView& getFanView() { return m_fanView; }
		FineTune& getFineTuneView() { return m_fineTuneView; }
		HeightmapView& getHeightmapView() { return m_heightmapView; }
		ObjectCancelView& getObjectCancelView() { return m_objectCancelView; }
#endif

		auto& getUpdatePrompt() { return m_updatePrompt; }
		ModalExtrusionFactor& getExtrusionFactorModal() { return m_extrusionFactorModal; }
		ModalNumberPad& getNumberPad() { return m_numberpad; }

		HomeView();
		~HomeView();

	  protected:
	  private:
		// Test seam: when set (by friend HomeViewTest), instance() returns this instead of the static singleton
		static HomeView* s_overrideInstance;
		static std::atomic<bool> s_instanceInitialized;
		virtual void onShow();
		virtual void onHide();

		StatusBar m_statusBar{getRoot()};
		SideBar m_sideBar{"sidebar", getRoot()};
		LvContainer m_mainWindow{"main_window", getRoot()};

		// Windows
		Dashboard m_dashboard{"dashboard", m_mainWindow};
		ConsoleView m_consoleView{"console",
#if CONSOLE_SIDE_PANEL
								  getRoot()
#else
								  m_mainWindow
#endif
		};
		ControlView m_controlView{"control", m_mainWindow};
		TabView m_files{"files", m_mainWindow};
		FileView m_macroView{
			"macros",
			m_files.addTab(_("file.macros")),
			FileView::StorageKeys{.sortBy = {"ui:home:file:macros:sort_by", OM::FileSystem::SortBy::NAME},
								  .sortDescending = {"ui:home:file:macros:sort_descending", false},
								  .displayMode = {"ui:home:file:macros:display_mode", FileView::DisplayMode::List}}};
		FileView m_jobView{
			"jobs",
			m_files.addTab(_("file.jobs")),
			FileView::StorageKeys{.sortBy = {"ui:home:file:jobs:sort_by", OM::FileSystem::SortBy::DATE},
								  .sortDescending = {"ui:home:file:jobs:sort_descending", true},
								  .displayMode = {"ui:home:file:jobs:display_mode", FileView::DisplayMode::Tile}}};
		SettingsView m_settingsView{"settings", m_mainWindow};
#if SIDE_BAR_APP_DRAWER
		MoveView m_moveView{"move", m_mainWindow};
		TemperatureView m_temperatureView{"temperature", m_mainWindow};
		FanView m_fanView{"fan", m_mainWindow};
		FineTune m_fineTuneView{"fine_tune", m_mainWindow};
		HeightmapView m_heightmapView{"heightmap", m_mainWindow};
		ObjectCancelView m_objectCancelView{"object_cancel", m_mainWindow};
#endif

		// Message box
		std::list<std::shared_ptr<MessageBox>> m_messageBoxList;
		Modal<AlertMessageBox> m_alert{"alert", getRoot(), layout_t(0, 0, 70, LV_SIZE_CONTENT)};

		// Update prompt
		Modal<MessageBox> m_updatePrompt{"update_prompt", getRoot(), layout_t(0, 0, 70, LV_SIZE_CONTENT)};

		ModalExtrusionFactor m_extrusionFactorModal{"extrusion_factor_modal", m_mainWindow};

		// Input
		ModalNumberPad m_numberpad{"numberpad", m_mainWindow};
		LvKeyboard m_kb{"keyboard", getRoot()};

		/* Workshop home scene (full screen, on top). Declared last so it is created last. */
		Workshop m_workshop{"workshop", getRoot()};
		/* Laptop page (console / alerts / settings), full screen, opened from the scene */
		Laptop m_laptop{"laptop", getRoot()};
		/* Object cancel folder, opened from the spare bed */
		ObjectFolder m_objectFolder{"object_folder", getRoot()};
		JobsFolder m_jobsFolder{"jobs_folder", getRoot()};
		FilamentBox m_filamentBox{"filament_box", getRoot()};
		bool m_workshopHome = true;
	};
} // namespace UI