/*
 * Laptop.h
 *
 * The laptop page, opened from the laptop on the workshop scene. A close-up of an old
 * ThinkPad-like laptop whose screen holds:
 *
 *   side column   STOP (red, emergency stop), ALERTS (red dot when unread), CONSOLE,
 *                 SETTINGS, HOME (back to the workshop)
 *   CONSOLE       the log (ages, newest at the bottom), the command line and the
 *                 keyboard button
 *   ALERTS        7 rows per page, 3 pages; right column: up / filter / down
 *   SETTINGS      SYSTEM (page 1) and PERSONALIZATION (page 2); right column: up / page / down
 *
 * The keyboard: the screen slides down so only the bottom of the page shows (the side
 * column goes off the top and does nothing); the laptop's own keyboard fills the rest.
 * It types into the command line (or a setting such as the printer's IP address).
 *
 * The log and the alerts are collected while the page is hidden: the presenter stays
 * active (hideInner() does not deactivate it).
 */

#pragma once

#include "FunkyPopup.h"
#include "FunkyUi.h"
#include "LaptopLayout.h"
#include "LaptopLog.h"
#include "LaptopPresenter.h"
#include "UI/Core/View.h"
#include <array>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace UI
{
	class Laptop : public View<LaptopPresenter>
	{
	  public:
		enum class Tab : uint8_t
		{
			Alerts,
			Console,
			Settings,
		};

		static constexpr size_t SETTING_SLOTS = LaptopLayout::SETTING_COLS * LaptopLayout::SETTING_ROWS;
		static constexpr size_t KEY_COUNT = std::size(LaptopLayout::KEYS);

		Laptop(const std::string& name, LvObj& parent);
		~Laptop() override;

		/* Data (kept while hidden) */
		void response(bool isError, const std::string& text);
		void info(const std::string& text);
		void setJobActive(bool active) { m_jobActive = active; }

		/* Page */
		void showTab(Tab tab);
		Tab tab() const { return m_tab; }
		void openKeyboard();
		void closeKeyboard();
		bool keyboardOpen() const { return m_keyboardOpen; }

		/* Touches (also used by the tests) */
		void pressStop();
		void pressHome();
		void pressKey(size_t index);
		void pressKey(LaptopLayout::KeyKind kind);
		void type(const std::string& text); /* character keys, shifting as needed */
		void pressAlertRow(size_t row);
		void pressAlertFilter();
		void pressAlertPage(int dir);
		void pressSetting(size_t slot);
		void pressSettingsPage(int dir);

		/* State, for the tests */
		Funky::Popup& popup() { return m_popup; }
		LaptopLog::ConsoleLog& consoleLog() { return m_console; }
		LaptopLog::AlertList& alertList() { return m_alerts; }
		const std::string& inputText() const;
		bool shift() const { return m_shift; }
		size_t cursor() const { return caret(); }
		size_t settingsPage() const { return m_settingsPage; }
		void setClock(std::function<uint32_t()> clock) { m_clock = std::move(clock); }

	  protected:
		void onInit() override;
		void onShow() override;
		void onHide() override;
		void hideInner() override; /* keeps the presenter active */

	  private:
		struct SettingSlot
		{
			lv_obj_t* tile = nullptr;
			lv_obj_t* name = nullptr;
			lv_obj_t* value = nullptr;
		};
		struct AlertRowObjs
		{
			lv_obj_t* tile = nullptr;
			lv_obj_t* badge = nullptr;
			lv_obj_t* badgeText = nullptr;
			lv_obj_t* age = nullptr;
			lv_obj_t* text = nullptr;
			lv_obj_t* dot = nullptr;
		};
		struct Pager
		{
			lv_obj_t* up = nullptr;
			lv_obj_t* upIcon = nullptr;
			lv_obj_t* middle = nullptr;
			lv_obj_t* middleTop = nullptr;
			lv_obj_t* middleBottom = nullptr;
			lv_obj_t* down = nullptr;
			lv_obj_t* downIcon = nullptr;
		};
		enum class Target : uint8_t
		{
			Console,
			Entry,
		};

		uint32_t now() const { return m_clock ? m_clock() : lv_tick_get(); }

		void buildSideColumn();
		void buildConsole();
		void buildAlerts();
		void buildSettings();
		void buildKeyboard();
		Pager buildPager(lv_obj_t* parent, bool middleClickable);
		void setPagerArrows(Pager& p, bool upEnabled, bool downEnabled);

		void refreshSide();
		void refreshConsole();
		void refreshInput();
		void refreshAges();
		void refreshAlerts();
		void refreshSettings();
		void applyKeyboardState();
		void setArt(bool loaded);

		void sendCommand();
		void openEntry(const std::string& title, const std::string& text, std::function<void(const std::string&)> onEnter);
		std::string& editText();
		size_t caret() const;
		void caretToEnd();

		/* settings */
		void chooseConnection();
		std::string settingValue(size_t page, size_t slot) const;

		static void clicked(lv_event_t* e);
		static void ageTimer(lv_timer_t* t);

		Funky::Images m_images;
		Funky::Fonts m_fonts;
		Funky::Popup m_popup{m_images, m_fonts};
		std::vector<std::unique_ptr<lv_point_precise_t[]>> m_points;

		LaptopLog::ConsoleLog m_console;
		LaptopLog::AlertList m_alerts;
		std::function<uint32_t()> m_clock;
		bool m_built = false;
		bool m_jobActive = false;
		Tab m_tab = Tab::Console;
		bool m_keyboardOpen = false;
		bool m_shift = false;
		Target m_target = Target::Console;
		std::string m_entryText;
		std::function<void(const std::string&)> m_onEntry;
		size_t m_settingsPage = 0;
		lv_timer_t* m_timer = nullptr;

		/* objects */
		lv_obj_t* m_bgUp = nullptr;
		lv_obj_t* m_bgKeyboard = nullptr;
		lv_obj_t* m_shiftKeys = nullptr;
		std::vector<lv_obj_t*> m_shiftMarks;
		lv_obj_t* m_lcd = nullptr;
		lv_obj_t* m_page = nullptr;
		lv_obj_t* m_sideBlock = nullptr;

		std::array<lv_obj_t*, 5> m_side{};
		lv_obj_t* m_alertDot = nullptr;

		lv_obj_t* m_consolePane = nullptr;
		std::array<lv_obj_t*, LaptopLayout::CONSOLE_LINES> m_lineAge{};
		std::array<lv_obj_t*, LaptopLayout::CONSOLE_LINES> m_lineText{};
		lv_obj_t* m_input = nullptr;
		lv_obj_t* m_inputPrompt = nullptr;
		lv_obj_t* m_inputText = nullptr;
		lv_obj_t* m_cursor = nullptr;
		lv_obj_t* m_kbButton = nullptr;
		lv_obj_t* m_entryTitle = nullptr;
		size_t m_inputChars = 50;
		int32_t m_inputGlyph = 11;
		size_t m_caret = 0; /* cursor in the text being typed (bytes; the keys type ASCII) */

		lv_obj_t* m_alertsPane = nullptr;
		std::array<AlertRowObjs, LaptopLog::AlertList::PAGE_ROWS> m_alertRows{};
		lv_obj_t* m_alertsEmpty = nullptr;
		Pager m_alertPager;

		lv_obj_t* m_settingsPane = nullptr;
		lv_obj_t* m_sectionName = nullptr;
		lv_obj_t* m_sectionLine = nullptr;
		std::array<SettingSlot, SETTING_SLOTS> m_slots{};
		Pager m_settingsPager;

		std::array<lv_obj_t*, KEY_COUNT> m_keys{};
	};
} // namespace UI
