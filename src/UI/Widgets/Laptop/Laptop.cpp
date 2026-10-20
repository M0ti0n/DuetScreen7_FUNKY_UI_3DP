/*
 * Laptop.cpp
 */

#include "Laptop.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "LaptopSettings.h"
#include "UI/Core/Navigation.h"
#include "UI/Widgets/Workshop/PrinterStyle.h"
#include "version.h"
#include <algorithm>
#include <fmt/format.h>

namespace UI
{
	using namespace LaptopLayout;
	using namespace Funky;
	using LaptopLog::AlertList;
	using LaptopLog::ConsoleLog;
	using LaptopLog::FormatAge;

	namespace
	{
		constexpr const char* SIDE_NAMES[] = {"STOP", "ALERTS", "CONSOLE", "SETTINGS", ""};
		enum Side : size_t
		{
			SIDE_STOP,
			SIDE_ALERTS,
			SIDE_CONSOLE,
			SIDE_SETTINGS,
			SIDE_HOME,
		};
		constexpr int32_t SIDE_TILE_H = 83;
		constexpr int32_t PAGER_TILE_H = 144;
		constexpr int32_t PAGER_STEP = PAGER_TILE_H + 11;
		constexpr int32_t ALERT_W = PAGER_X - PAGER_GAP - CONTENT_X; /* 636 */
		constexpr int32_t INPUT_W = CONTENT_R - CONTENT_X - CONSOLE_KB_BTN_W - 10;
		constexpr int32_t SLOT_W = 202;
		constexpr int32_t SLOT_STEP_X = SLOT_W + 15;
		constexpr int32_t SLOT_STEP_Y = SETTING_TILE_H + SETTING_GAP_Y;
		constexpr int32_t SECTION_LINE_END = PAGER_X - PAGER_GAP;

		/* SETTINGS slots, "" = empty */
		constexpr const char* SETTINGS_PAGES[2][Laptop::SETTING_SLOTS] = {
			{"BRIGHTNESS", "SOUND", "SCREEN DIMMING", "ORIENTATION", "CONNECTION", "ABOUT", "", "", "FACTORY RESET"},
			{"DRAWER INDICATOR", "PRINTER", "", "", "", "", "", "", ""},
		};
		constexpr const char* SECTION_NAMES[2] = {"SYSTEM", "PERSONALIZATION"};

		constexpr unsigned int BRIGHTNESS_VALUES[] = {10, 20, 40, 60, 80, 100};
		constexpr unsigned int DIM_VALUES[] = {0, 60, 300, 600};

		template <size_t N>
		size_t nearest(const unsigned int (&values)[N], unsigned int v)
		{
			size_t best = 0;
			for (size_t i = 1; i < N; i++)
			{
				const auto d = [v](unsigned int x) { return x > v ? x - v : v - x; };
				if (d(values[i]) < d(values[best]))
				{
					best = i;
				}
			}
			return best;
		}

		const char* soundName(unsigned int s)
		{
			static constexpr const char* names[] = {
				"OFF", "BUZZER", "SPEAKER 1", "SPEAKER 2", "SPEAKER 3", "SPEAKER 4", "SPEAKER 5"};
			return s < std::size(names) ? names[s] : "BUZZER";
		}

		const char* linkName(LaptopSettings::Link link)
		{
			switch (link)
			{
			case LaptopSettings::Link::Uart:
				return "UART";
			case LaptopSettings::Link::Usb:
				return "USB";
			case LaptopSettings::Link::Network:
				return "NETWORK";
			}
			return "";
		}

		uint32_t kindColour(ConsoleLog::Kind kind)
		{
			switch (kind)
			{
			case ConsoleLog::Kind::Command:
				return Colour::COMMAND;
			case ConsoleLog::Kind::Reply:
				return Colour::TEXT;
			case ConsoleLog::Kind::Warning:
				return Colour::AMBER;
			case ConsoleLog::Kind::Error:
				return Colour::RED;
			}
			return Colour::TEXT;
		}

		uint32_t levelColour(AlertList::Level level)
		{
			switch (level)
			{
			case AlertList::Level::Error:
				return Colour::RED;
			case AlertList::Level::Warning:
				return Colour::AMBER;
			case AlertList::Level::Info:
				return Colour::ACCENT;
			}
			return Colour::ACCENT;
		}

		constexpr size_t MAX_INPUT_CHARS = 100;
	} // namespace

	Laptop::Laptop(const std::string& name, LvObj& parent)
		: View(name, parent)
	{
		ZoneScoped;
		lv_obj_t* root = getRootPtr();
		lv_obj_remove_style_all(root);
		lv_obj_add_flag(root, LV_OBJ_FLAG_FLOATING); /* not part of the HomeView grid */
		lv_obj_set_pos(root, 0, 0);
		lv_obj_set_size(root, 1024, 600);
		lv_obj_set_style_bg_color(root, lv_color_black(), 0);
		lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
		lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_add_flag(root, LV_OBJ_FLAG_HIDDEN);

		/* Collect replies and alerts from the start, also while hidden */
		activate();
	}

	Laptop::~Laptop()
	{
		if (m_timer != nullptr)
		{
			lv_timer_delete(m_timer);
			m_timer = nullptr;
		}
	}

	void Laptop::hideInner()
	{
		/* Not View::hideInner(): the presenter stays active so nothing is missed */
		LvContainer::hideInner();
	}

	/* ---------------- building ---------------- */

	void Laptop::onInit()
	{
		ZoneScoped;
		lv_obj_t* root = getRootPtr();

		m_bgUp = lv_image_create(root);
		lv_obj_set_pos(m_bgUp, 0, 0);
		m_bgKeyboard = lv_image_create(root);
		lv_obj_set_pos(m_bgKeyboard, 0, 0);
		m_shiftKeys = lv_image_create(root);
		lv_obj_set_pos(m_shiftKeys, KEYBOARD.x, KEYBOARD.y);
		setArt(true);

		/* The LCD clips the page; the page is always the full screen size */
		m_lcd = box(root, LCD_UP.x, LCD_UP.y, LCD_UP.w, LCD_UP.h);
		m_page = box(m_lcd, 0, 0, LCD_UP.w, LCD_UP.h);

		buildConsole();
		buildAlerts();
		buildSettings();
		buildSideColumn();
		buildKeyboard();
		m_popup.attach(root);

		m_built = true;
		m_timer = lv_timer_create(ageTimer, 1000, this);
		showTab(m_tab);
		applyKeyboardState();
	}

	void Laptop::buildSideColumn()
	{
		for (size_t i = 0; i < m_side.size(); i++)
		{
			const int32_t y = MARGIN + static_cast<int32_t>(i) * (SIDE_TILE_H + SIDE_GAP);
			lv_obj_t* t = tile(m_page, SIDE_X, y, SIDE_W, SIDE_TILE_H, TileLook::Normal, true);
			lv_obj_add_event_cb(t, clicked, LV_EVENT_CLICKED, this);
			if (i == SIDE_HOME)
			{
				houseIcon(t, SIDE_W / 2, SIDE_TILE_H / 2 + 2, 20, Colour::TEXT, m_points);
			}
			else
			{
				centredLabel(t, m_fonts.get(i == SIDE_STOP ? 21 : 18, true), Colour::TEXT, SIDE_NAMES[i], 1);
			}
			m_side[i] = t;
		}
		/* Unread alerts: red dot with an ink rim */
		m_alertDot = box(m_side[SIDE_ALERTS], SIDE_W - 30, 10, 20, 20);
		lv_obj_set_style_radius(m_alertDot, LV_RADIUS_CIRCLE, 0);
		lv_obj_set_style_bg_color(m_alertDot, col(Colour::RED), 0);
		lv_obj_set_style_bg_opa(m_alertDot, LV_OPA_COVER, 0);
		lv_obj_set_style_border_color(m_alertDot, lv_color_hex(0x0E0D0C), 0);
		lv_obj_set_style_border_width(m_alertDot, 2, 0);
		lv_obj_set_style_border_opa(m_alertDot, LV_OPA_COVER, 0);

		/* Keyboard open: the column is pushed off the top and does nothing */
		m_sideBlock = box(m_page, 0, 0, SIDE_X + SIDE_W + 8, LCD_UP.h);
		lv_obj_add_flag(m_sideBlock, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_set_style_bg_color(m_sideBlock, lv_color_hex(0x080C14), 0);
		lv_obj_set_style_bg_opa(m_sideBlock, 150, 0);
	}

	void Laptop::buildConsole()
	{
		m_consolePane = box(m_page, 0, 0, LCD_UP.w, LCD_UP.h);
		const lv_font_t* mono = m_fonts.get(18, false, true);
		for (size_t i = 0; i < CONSOLE_LINES; i++)
		{
			const int32_t y = CONSOLE_LOG_BOTTOM - (static_cast<int32_t>(CONSOLE_LINES - i)) * CONSOLE_LINE_H;
			m_lineAge[i] = label(m_consolePane, mono, Colour::DIM, "");
			lv_obj_set_pos(m_lineAge[i], CONSOLE_AGE_X, y);
			m_lineText[i] = label(m_consolePane, mono, Colour::TEXT, "");
			lv_obj_set_pos(m_lineText[i], CONSOLE_TEXT_X, y);
			lv_obj_set_width(m_lineText[i], CONTENT_R - CONSOLE_TEXT_X);
			lv_label_set_long_mode(m_lineText[i], LV_LABEL_LONG_MODE_CLIP);
		}
		const uint16_t glyph = lv_font_get_glyph_width(mono, 'M', 'M');
		if (glyph > 0)
		{
			m_console.setWrap(static_cast<size_t>((CONTENT_R - CONSOLE_TEXT_X) / glyph));
		}

		/* Command line (also the entry line of a setting) and the keyboard button: on the page,
		 * so a setting can use them too */
		m_entryTitle = label(m_page, m_fonts.get(17), Colour::DIM, "");
		lv_obj_set_pos(m_entryTitle, CONTENT_X + 4, CONSOLE_INPUT_Y - 34);
		m_input = tile(m_page, CONTENT_X, CONSOLE_INPUT_Y, INPUT_W, CONSOLE_INPUT_H, TileLook::Input, true);
		lv_obj_add_event_cb(m_input, clicked, LV_EVENT_CLICKED, this);
		m_inputPrompt = label(m_input, m_fonts.get(19, true, true), Colour::ACCENT, ">");
		lv_obj_align(m_inputPrompt, LV_ALIGN_LEFT_MID, 12, 0);
		const lv_font_t* inputFont = m_fonts.get(19, false, true);
		const uint16_t inputGlyph = lv_font_get_glyph_width(inputFont, 'M', 'M');
		if (inputGlyph > 0)
		{
			m_inputGlyph = inputGlyph;
			m_inputChars = static_cast<size_t>((INPUT_W - 34 - 30) / inputGlyph);
		}
		m_inputText = label(m_input, inputFont, Colour::TEXT, "");
		lv_obj_set_width(m_inputText, static_cast<int32_t>(m_inputChars) * m_inputGlyph + 4);
		lv_label_set_long_mode(m_inputText, LV_LABEL_LONG_MODE_CLIP);
		lv_obj_align(m_inputText, LV_ALIGN_LEFT_MID, 34, 0);
		m_cursor = box(m_input, 0, 0, 11, 24);
		lv_obj_set_style_bg_color(m_cursor, col(Colour::ACCENT), 0);
		lv_obj_set_style_bg_opa(m_cursor, LV_OPA_COVER, 0);

		m_kbButton = tile(m_page,
						  CONTENT_R - CONSOLE_KB_BTN_W,
						  CONSOLE_INPUT_Y,
						  CONSOLE_KB_BTN_W,
						  CONSOLE_INPUT_H,
						  TileLook::Normal,
						  true);
		lv_obj_add_event_cb(m_kbButton, clicked, LV_EVENT_CLICKED, this);
		keyboardIcon(m_kbButton, CONSOLE_KB_BTN_W / 2, CONSOLE_INPUT_H / 2, 10, Colour::TEXT);
	}

	Laptop::Pager Laptop::buildPager(lv_obj_t* parent, bool middleClickable)
	{
		Pager p;
		p.up = tile(parent, PAGER_X, MARGIN, PAGER_W, PAGER_TILE_H, TileLook::Normal, true);
		p.upIcon = chevron(p.up, PAGER_W / 2, PAGER_TILE_H / 2, true, Colour::TEXT, m_points);
		lv_obj_add_event_cb(p.up, clicked, LV_EVENT_CLICKED, this);
		if (middleClickable)
		{
			p.middle = tile(parent, PAGER_X, MARGIN + PAGER_STEP, PAGER_W, PAGER_TILE_H, TileLook::Selected, true);
			lv_obj_add_event_cb(p.middle, clicked, LV_EVENT_CLICKED, this);
			p.middleTop = centredLabel(p.middle, m_fonts.get(19, true), Colour::TEXT, "", -12);
			p.middleBottom = centredLabel(p.middle, m_fonts.get(14, true), Colour::DIM, "", 16);
		}
		else
		{
			p.middle = box(parent, PAGER_X, MARGIN + PAGER_STEP, PAGER_W, PAGER_TILE_H);
			p.middleTop = centredLabel(p.middle, m_fonts.get(18, true), Colour::DIM, "", 0);
		}
		p.down = tile(parent, PAGER_X, MARGIN + 2 * PAGER_STEP, PAGER_W, PAGER_TILE_H, TileLook::Normal, true);
		p.downIcon = chevron(p.down, PAGER_W / 2, PAGER_TILE_H / 2, false, Colour::TEXT, m_points);
		lv_obj_add_event_cb(p.down, clicked, LV_EVENT_CLICKED, this);
		return p;
	}

	void Laptop::setPagerArrows(Pager& p, bool upEnabled, bool downEnabled)
	{
		setEnabled(p.up, upEnabled);
		setEnabled(p.down, downEnabled);
		lv_obj_set_style_line_color(p.upIcon, col(upEnabled ? Colour::TEXT : Colour::DISABLED), 0);
		lv_obj_set_style_line_color(p.downIcon, col(downEnabled ? Colour::TEXT : Colour::DISABLED), 0);
	}

	void Laptop::buildAlerts()
	{
		m_alertsPane = box(m_page, 0, 0, LCD_UP.w, LCD_UP.h);
		for (size_t r = 0; r < m_alertRows.size(); r++)
		{
			AlertRowObjs& row = m_alertRows[r];
			const int32_t y = MARGIN + static_cast<int32_t>(r) * (ALERT_ROW_H + ALERT_ROW_GAP);
			row.tile = tile(m_alertsPane, CONTENT_X, y, ALERT_W, ALERT_ROW_H, TileLook::Normal, true);
			lv_obj_add_event_cb(row.tile, clicked, LV_EVENT_CLICKED, this);
			row.badge = box(row.tile, 10, ALERT_ROW_H / 2 - 13, 60, 26);
			lv_obj_set_style_radius(row.badge, 5, 0);
			lv_obj_set_style_border_width(row.badge, 2, 0);
			lv_obj_set_style_border_opa(row.badge, LV_OPA_COVER, 0);
			row.badgeText = centredLabel(row.badge, m_fonts.get(15, true), Colour::TEXT, "", 0);
			row.age = label(row.tile, m_fonts.get(16, false, true), Colour::DIM, "");
			lv_obj_align(row.age, LV_ALIGN_LEFT_MID, 82, 0);
			row.text = label(row.tile, m_fonts.get(17), Colour::TEXT, "");
			lv_obj_set_size(row.text, ALERT_W - 170 - 34, lv_font_get_line_height(m_fonts.get(17)));
			lv_label_set_long_mode(row.text, LV_LABEL_LONG_MODE_DOTS);
			lv_obj_align(row.text, LV_ALIGN_LEFT_MID, 170, 0);
			row.dot = box(row.tile, ALERT_W - 23, ALERT_ROW_H / 2 - 5, 10, 10);
			lv_obj_set_style_radius(row.dot, LV_RADIUS_CIRCLE, 0);
			lv_obj_set_style_bg_color(row.dot, col(Colour::ACCENT), 0);
			lv_obj_set_style_bg_opa(row.dot, LV_OPA_COVER, 0);
		}
		m_alertsEmpty = label(m_alertsPane, m_fonts.get(18), Colour::DIM, "No alerts");
		lv_obj_set_pos(m_alertsEmpty, CONTENT_X + 16, MARGIN + 18);
		m_alertPager = buildPager(m_alertsPane, true);
	}

	void Laptop::buildSettings()
	{
		m_settingsPane = box(m_page, 0, 0, LCD_UP.w, LCD_UP.h);
		m_sectionName = label(m_settingsPane, m_fonts.get(15, true), Colour::DIM, "");
		lv_obj_set_pos(m_sectionName, CONTENT_X + 4, MARGIN + 3);
		m_sectionLine = box(m_settingsPane, CONTENT_X + 120, MARGIN + 11, 100, 2);
		lv_obj_set_style_bg_color(m_sectionLine, col(Colour::OUTLINE), 0);
		lv_obj_set_style_bg_opa(m_sectionLine, LV_OPA_COVER, 0);
		for (size_t i = 0; i < m_slots.size(); i++)
		{
			const auto c = static_cast<int32_t>(i % SETTING_COLS), r = static_cast<int32_t>(i / SETTING_COLS);
			SettingSlot& s = m_slots[i];
			s.tile = tile(m_settingsPane,
						  CONTENT_X + c * SLOT_STEP_X,
						  SETTING_TOP + r * SLOT_STEP_Y,
						  SLOT_W,
						  SETTING_TILE_H,
						  TileLook::Normal,
						  true);
			lv_obj_add_event_cb(s.tile, clicked, LV_EVENT_CLICKED, this);
			s.name = centredLabel(s.tile, m_fonts.get(17, true), Colour::TEXT, "", -12);
			s.value = centredLabel(s.tile, m_fonts.get(16), Colour::ACCENT, "", 14);
		}
		m_settingsPager = buildPager(m_settingsPane, false);
	}

	void Laptop::buildKeyboard()
	{
		for (size_t i = 0; i < KEY_COUNT; i++)
		{
			const Rect& r = KEYS[i].rect;
			if (KEYS[i].kind == KeyKind::Shift)
			{
				lv_obj_t* mark = box(getRootPtr(), r.x, r.y, r.w, r.h - 4);
				lv_obj_set_style_radius(mark, 6, 0);
				lv_obj_set_style_border_color(mark, col(Colour::ACCENT), 0);
				lv_obj_set_style_border_width(mark, 3, 0);
				lv_obj_set_style_border_opa(mark, LV_OPA_COVER, 0);
				m_shiftMarks.push_back(mark);
			}
		}
		for (size_t i = 0; i < KEY_COUNT; i++)
		{
			const Rect& r = KEYS[i].rect;
			m_keys[i] = tapZone(getRootPtr(), r.x, r.y, r.w, r.h, 6, true);
			lv_obj_add_event_cb(m_keys[i], clicked, LV_EVENT_CLICKED, this);
		}
	}

	/* ---------------- showing ---------------- */

	void Laptop::setArt(bool loaded)
	{
		/* The art is only kept while the page is shown (LVGL's heap is fixed) */
		static constexpr const char* FILES[] = {"laptop/screen_up.png", "laptop/keyboard.png", "laptop/keys_shift.png"};
		lv_obj_t* objs[] = {m_bgUp, m_bgKeyboard, m_shiftKeys};
		for (size_t i = 0; i < std::size(FILES); i++)
		{
			if (objs[i] == nullptr)
			{
				continue;
			}
			if (loaded)
			{
				lv_image_set_src(objs[i], m_images.get(FILES[i], true));
			}
			else
			{
				lv_image_set_src(objs[i], nullptr);
				m_images.release(FILES[i]);
			}
		}
	}

	void Laptop::onShow()
	{
		ZoneScoped;
		moveToFront();
		if (m_built)
		{
			setArt(true);
			showTab(m_tab);
		}
	}

	void Laptop::onHide()
	{
		if (m_keyboardOpen)
		{
			closeKeyboard();
		}
		m_popup.close();
		setArt(false);
	}

	void Laptop::showTab(Tab tab)
	{
		m_tab = tab;
		if (!m_built)
		{
			return;
		}
		const bool entry = m_target == Target::Entry;
		setHidden(m_consolePane, tab != Tab::Console || entry);
		setHidden(m_alertsPane, tab != Tab::Alerts || entry);
		setHidden(m_settingsPane, tab != Tab::Settings || entry);
		refreshSide();
		refreshConsole();
		refreshInput();
		refreshAlerts();
		refreshSettings();
	}

	void Laptop::refreshSide()
	{
		if (!m_built)
		{
			return;
		}
		setTileLook(m_side[SIDE_STOP], TileLook::Stop);
		setTileLook(m_side[SIDE_ALERTS], m_tab == Tab::Alerts ? TileLook::Selected : TileLook::Normal);
		setTileLook(m_side[SIDE_CONSOLE], m_tab == Tab::Console ? TileLook::Selected : TileLook::Normal);
		setTileLook(m_side[SIDE_SETTINGS], m_tab == Tab::Settings ? TileLook::Selected : TileLook::Normal);
		setTileLook(m_side[SIDE_HOME], TileLook::Normal);
		setHidden(m_alertDot, !m_alerts.anyUnread());
	}

	void Laptop::refreshConsole()
	{
		if (!m_built)
		{
			return;
		}
		const auto& lines = m_console.lines();
		const size_t shown = std::min(lines.size(), static_cast<size_t>(CONSOLE_LINES));
		const size_t firstRow = CONSOLE_LINES - shown;
		const size_t firstLine = lines.size() - shown;
		const uint32_t t = now();
		for (size_t r = 0; r < CONSOLE_LINES; r++)
		{
			if (r < firstRow)
			{
				lv_label_set_text(m_lineAge[r], "");
				lv_label_set_text(m_lineText[r], "");
				continue;
			}
			const ConsoleLog::Line& line = lines[firstLine + (r - firstRow)];
			lv_label_set_text(m_lineAge[r], line.first ? FormatAge(t - line.time).c_str() : "");
			lv_label_set_text(m_lineText[r], line.text.c_str());
			lv_obj_set_style_text_color(m_lineText[r], col(kindColour(line.kind)), 0);
		}
	}

	size_t Laptop::caret() const
	{
		return std::min(m_caret, inputText().size());
	}

	void Laptop::caretToEnd()
	{
		m_caret = inputText().size();
	}

	std::string& Laptop::editText()
	{
		return m_target == Target::Entry ? m_entryText : m_console.draft();
	}

	const std::string& Laptop::inputText() const
	{
		return m_target == Target::Entry ? m_entryText : m_console.draft();
	}

	void Laptop::refreshInput()
	{
		if (!m_built)
		{
			return;
		}
		const bool entry = m_target == Target::Entry;
		const bool show = entry || m_tab == Tab::Console;
		setHidden(m_input, !show);
		setHidden(m_kbButton, !show || entry);
		setHidden(m_entryTitle, !entry);

		/* A long line scrolls so the cursor stays in view (monospaced font: one cell a character) */
		const std::string& text = inputText();
		const size_t pos = caret();
		size_t start = 0;
		if (text.size() > m_inputChars && pos + 1 > m_inputChars)
		{
			start = std::min(pos + 1 - m_inputChars, text.size() - m_inputChars + 1);
		}
		lv_label_set_text(m_inputText, text.substr(start, m_inputChars).c_str());
		const int32_t x = 34 + static_cast<int32_t>(pos - start) * m_inputGlyph;
		if (pos >= text.size())
		{
			/* at the end: a block */
			lv_obj_set_size(m_cursor, 11, 24);
			lv_obj_align(m_cursor, LV_ALIGN_LEFT_MID, x + (text.empty() ? 0 : 2), 0);
		}
		else
		{
			/* inside the text: a bar before the character */
			lv_obj_set_size(m_cursor, 3, 28);
			lv_obj_align(m_cursor, LV_ALIGN_LEFT_MID, x - 2, 0);
		}
	}

	void Laptop::refreshAges()
	{
		if (!m_built || !isVisible())
		{
			return;
		}
		if (m_tab == Tab::Console)
		{
			refreshConsole();
		}
		if (m_tab == Tab::Alerts)
		{
			refreshAlerts();
		}
	}

	void Laptop::refreshAlerts()
	{
		if (!m_built)
		{
			return;
		}
		const uint32_t t = now();
		bool any = false;
		for (size_t r = 0; r < m_alertRows.size(); r++)
		{
			AlertRowObjs& row = m_alertRows[r];
			const int idx = m_alerts.rowIndex(r);
			setHidden(row.tile, idx < 0);
			if (idx < 0)
			{
				continue;
			}
			any = true;
			const AlertList::Entry& e = m_alerts.entry(static_cast<size_t>(idx));
			const uint32_t c = levelColour(e.level);
			lv_obj_set_style_border_color(row.badge, col(c), 0);
			lv_label_set_text(row.badgeText, AlertList::levelName(e.level));
			lv_obj_set_style_text_color(row.badgeText, col(c), 0);
			lv_label_set_text(row.age, FormatAge(t - e.time).c_str());
			lv_label_set_text(row.text, e.text.c_str());
			setHidden(row.dot, !e.unread);
		}
		setHidden(m_alertsEmpty, any);
		lv_label_set_text(m_alertPager.middleTop, AlertList::filterName(m_alerts.filter()));
		lv_label_set_text(m_alertPager.middleBottom,
						  fmt::format("{:d}/{:d}", m_alerts.page() + 1, m_alerts.pageCount()).c_str());
		setPagerArrows(m_alertPager, m_alerts.page() > 0, m_alerts.page() + 1 < m_alerts.pageCount());
		refreshSide();
	}

	std::string Laptop::settingValue(size_t page, size_t slot) const
	{
		if (page == 1)
		{
			if (slot == 1)
			{
				return PrinterStyle::SettingName(PrinterStyle::GetSetting());
			}
			return slot == 0 ? (LaptopSettings::DrawerIndicator() ? "ON" : "OFF") : "";
		}
		switch (slot)
		{
		case 0:
			return fmt::format("{:d}%", LaptopSettings::Brightness());
		case 1:
			return soundName(LaptopSettings::SoundSetting());
		case 2:
		{
			const unsigned int s = LaptopSettings::DimTimeout();
			if (s == 0)
			{
				return "OFF";
			}
			return s % 60 == 0 ? fmt::format("{:d} MIN", s / 60) : fmt::format("{:d} S", s);
		}
		case 3:
			return LaptopSettings::UpsideDown() ? "UPSIDE DOWN" : "NORMAL";
		case 4:
			return linkName(LaptopSettings::Connection());
		default: /* ABOUT: the version is in its popup */
			return "";
		}
	}

	void Laptop::refreshSettings()
	{
		if (!m_built)
		{
			return;
		}
		const size_t page = m_settingsPage;
		lv_label_set_text(m_sectionName, SECTION_NAMES[page]);
		lv_obj_update_layout(m_sectionName);
		const int32_t lineX = CONTENT_X + 4 + lv_obj_get_width(m_sectionName) + 16;
		lv_obj_set_x(m_sectionLine, lineX);
		lv_obj_set_width(m_sectionLine, std::max<int32_t>(0, SECTION_LINE_END - lineX));

		for (size_t i = 0; i < m_slots.size(); i++)
		{
			SettingSlot& s = m_slots[i];
			const char* name = SETTINGS_PAGES[page][i];
			const bool used = name[0] != '\0';
			setHidden(s.tile, !used);
			if (!used)
			{
				continue;
			}
			const bool danger = std::string_view(name) == "FACTORY RESET";
			setTileLook(s.tile, danger ? TileLook::Danger : TileLook::Normal);
			lv_obj_set_style_bg_color(s.tile, lv_color_black(), LV_STATE_PRESSED);
			lv_obj_set_style_bg_opa(s.tile, LV_OPA_50, LV_STATE_PRESSED);
			/* long names get a smaller font */
			uint32_t size = danger ? 18 : 17;
			lv_label_set_text(s.name, name);
			lv_obj_set_style_text_font(s.name, m_fonts.get(size, true), 0);
			lv_obj_update_layout(s.name);
			while (size > 13 && lv_obj_get_width(s.name) > SLOT_W - 30)
			{
				--size;
				lv_obj_set_style_text_font(s.name, m_fonts.get(size, true), 0);
				lv_obj_update_layout(s.name);
			}
			lv_obj_align(s.name, LV_ALIGN_CENTER, 0, danger ? 1 : -12);
			const std::string value = settingValue(page, i);
			lv_label_set_text(s.value, value.c_str());
			setHidden(s.value, danger || value.empty());
		}
		lv_label_set_text(m_settingsPager.middleTop, fmt::format("{:d}/2", page + 1).c_str());
		setPagerArrows(m_settingsPager, page > 0, page == 0);
	}

	void Laptop::applyKeyboardState()
	{
		if (!m_built)
		{
			return;
		}
		const bool kb = m_keyboardOpen;
		setHidden(m_bgUp, kb);
		setHidden(m_bgKeyboard, !kb);
		setHidden(m_shiftKeys, !kb || !m_shift);
		for (lv_obj_t* mark : m_shiftMarks)
		{
			setHidden(mark, !kb || !m_shift);
		}
		const Rect& lcd = kb ? LCD_KEYBOARD : LCD_UP;
		lv_obj_set_pos(m_lcd, lcd.x, lcd.y);
		lv_obj_set_size(m_lcd, lcd.w, lcd.h);
		lv_obj_set_y(m_page, lcd.h - LCD_UP.h);
		setHidden(m_sideBlock, !kb);
		for (lv_obj_t* k : m_keys)
		{
			setHidden(k, !kb);
		}
	}

	/* ---------------- data ---------------- */

	void Laptop::response(bool isError, const std::string& text)
	{
		UI_LOCK();
		const uint32_t t = now();
		m_console.addReply(text, t);
		const bool alerts = m_alerts.collect(text, isError, t);
		if (m_built)
		{
			refreshConsole();
			if (alerts)
			{
				refreshAlerts();
			}
		}
	}

	void Laptop::info(const std::string& text)
	{
		UI_LOCK();
		m_alerts.add(AlertList::Level::Info, text, now());
		refreshAlerts();
	}

	/* ---------------- keyboard ---------------- */

	void Laptop::openKeyboard()
	{
		if (!m_built)
		{
			return;
		}
		if (m_target == Target::Console && m_tab != Tab::Console)
		{
			showTab(Tab::Console);
		}
		m_keyboardOpen = true;
		caretToEnd();
		applyKeyboardState();
		refreshInput();
	}

	void Laptop::closeKeyboard()
	{
		m_keyboardOpen = false;
		m_shift = false;
		if (m_target == Target::Entry)
		{
			m_target = Target::Console;
			m_onEntry = nullptr;
			m_entryText.clear();
		}
		m_console.resetHistoryPos();
		applyKeyboardState();
		showTab(m_tab);
	}

	void Laptop::openEntry(const std::string& title,
						   const std::string& text,
						   std::function<void(const std::string&)> onEnter)
	{
		m_target = Target::Entry;
		m_entryText = text;
		m_caret = text.size();
		m_onEntry = std::move(onEnter);
		if (m_built)
		{
			lv_label_set_text(m_entryTitle, title.c_str());
		}
		showTab(m_tab);
		openKeyboard();
	}

	void Laptop::sendCommand()
	{
		std::string& draft = m_console.draft();
		const size_t b = draft.find_first_not_of(' ');
		if (b == std::string::npos)
		{
			draft.clear();
			refreshInput();
			return;
		}
		const std::string cmd = draft.substr(b);
		m_console.addCommand(cmd, now());
		Comm::DUET.SendGcode(cmd);
		draft.clear();
		m_caret = 0;
		refreshConsole();
		refreshInput();
	}

	void Laptop::pressKey(size_t index)
	{
		if (index >= KEY_COUNT || !m_keyboardOpen)
		{
			return;
		}
		const KeyDef& k = KEYS[index];
		std::string& text = editText();
		const size_t pos = caret();
		switch (k.kind)
		{
		case KeyKind::Char:
		case KeyKind::Space:
			if (text.size() < MAX_INPUT_CHARS)
			{
				const char c = k.kind == KeyKind::Space ? ' ' : (m_shift ? k.shifted : k.normal);
				text.insert(pos, 1, c);
				m_caret = pos + 1;
			}
			break;
		case KeyKind::Backspace:
			if (pos > 0)
			{
				text.erase(pos - 1, 1);
				m_caret = pos - 1;
			}
			break;
		case KeyKind::Left:
			m_caret = pos > 0 ? pos - 1 : 0;
			break;
		case KeyKind::Right:
			m_caret = pos < text.size() ? pos + 1 : pos;
			break;
		case KeyKind::Shift:
			m_shift = !m_shift;
			applyKeyboardState();
			break;
		case KeyKind::Up:
		case KeyKind::Down:
			if (m_target == Target::Console)
			{
				m_console.draft() = m_console.historyStep(k.kind == KeyKind::Up ? -1 : 1);
				caretToEnd();
			}
			break;
		case KeyKind::Enter:
			if (m_target == Target::Console)
			{
				sendCommand();
			}
			else
			{
				auto onEntry = std::move(m_onEntry);
				const std::string value = m_entryText;
				closeKeyboard();
				if (onEntry && !value.empty())
				{
					onEntry(value);
				}
				refreshSettings();
				return;
			}
			break;
		case KeyKind::Hide:
			closeKeyboard();
			return;
		}
		refreshInput();
	}

	void Laptop::pressKey(KeyKind kind)
	{
		for (size_t i = 0; i < KEY_COUNT; i++)
		{
			if (KEYS[i].kind == kind)
			{
				pressKey(i);
				return;
			}
		}
	}

	void Laptop::type(const std::string& text)
	{
		for (char c : text)
		{
			if (c == ' ')
			{
				pressKey(KeyKind::Space);
				continue;
			}
			for (size_t i = 0; i < KEY_COUNT; i++)
			{
				const KeyDef& k = KEYS[i];
				if (k.kind != KeyKind::Char || (k.normal != c && k.shifted != c))
				{
					continue;
				}
				const bool wantShift = k.normal != c;
				if (wantShift != m_shift)
				{
					pressKey(KeyKind::Shift);
				}
				pressKey(i);
				break;
			}
		}
	}

	/* ---------------- touches ---------------- */

	void Laptop::pressStop()
	{
		LOG_WARN("Laptop: STOP pressed, emergency stop");
		Comm::DUET.Estop();
	}

	void Laptop::pressHome()
	{
		m_popup.close();
		if (m_keyboardOpen)
		{
			closeKeyboard();
		}
		home();
	}

	void Laptop::pressAlertRow(size_t row)
	{
		const int idx = m_alerts.rowIndex(row);
		if (idx < 0)
		{
			return;
		}
		const size_t i = static_cast<size_t>(idx);
		m_alerts.markRead(i);
		const AlertList::Entry& e = m_alerts.entry(i);
		const Tone tone = e.level == AlertList::Level::Error	 ? Tone::Error
						  : e.level == AlertList::Level::Warning ? Tone::Warning
																 : Tone::Info;
		const char* title = e.level == AlertList::Level::Error	   ? "ERROR"
							: e.level == AlertList::Level::Warning ? "WARNING"
																   : "INFO";
		m_popup.message(title, tone, AlertList::levelName(e.level), FormatAge(now() - e.time) + " ago", e.text);
		refreshAlerts();
	}

	void Laptop::pressAlertFilter()
	{
		m_alerts.nextFilter();
		refreshAlerts();
	}

	void Laptop::pressAlertPage(int dir)
	{
		m_alerts.pageStep(dir);
		refreshAlerts();
	}

	void Laptop::pressSettingsPage(int dir)
	{
		if (dir < 0 && m_settingsPage > 0)
		{
			--m_settingsPage;
		}
		else if (dir > 0 && m_settingsPage == 0)
		{
			++m_settingsPage;
		}
		refreshSettings();
	}

	void Laptop::chooseConnection()
	{
		if (m_jobActive)
		{
			m_popup.message(
				"CONNECTION", Tone::Warning, "WRN", "", "The connection can't be changed while a print is running.");
			return;
		}
		m_popup.choose("CONNECTION",
					   {"UART", "USB", "NETWORK"},
					   static_cast<size_t>(LaptopSettings::Connection()),
					   [this](size_t c)
					   {
						   const auto link = static_cast<LaptopSettings::Link>(c);
						   if (link != LaptopSettings::Link::Network)
						   {
							   LaptopSettings::SetConnection(link);
							   refreshSettings();
							   return;
						   }
						   /* NETWORK: type the printer's address on the laptop keyboard */
						   openEntry("Printer IP address (ENTER to connect)",
									 LaptopSettings::DuetIp(),
									 [](const std::string& ip)
									 { LaptopSettings::SetConnection(LaptopSettings::Link::Network, ip); });
					   });
	}

	void Laptop::pressSetting(size_t slot)
	{
		if (slot >= SETTING_SLOTS)
		{
			return;
		}
		auto refresh = [this] { refreshSettings(); };
		if (m_settingsPage == 1)
		{
			if (slot == 0)
			{
				m_popup.choose("DRAWER INDICATOR",
							   {"ON", "OFF"},
							   LaptopSettings::DrawerIndicator() ? 0 : 1,
							   [refresh](size_t c)
							   {
								   LaptopSettings::SetDrawerIndicator(c == 0);
								   refresh();
							   });
			}
			else if (slot == 1)
			{
				/* The printer on the workshop desk; AUTO follows the kinematics (delta or not) */
				using PrinterStyle::Setting;
				m_popup.choose(
					"PRINTER",
					{"AUTO", "COREXY", "DELTA", "BED SLINGER"},
					static_cast<size_t>(PrinterStyle::GetSetting()),
					[refresh](size_t c)
					{
						PrinterStyle::SetSetting(static_cast<Setting>(c));
						refresh();
					},
					2);
			}
			return;
		}
		switch (slot)
		{
		case 0:
		{
			std::vector<std::string> labels;
			for (unsigned int v : BRIGHTNESS_VALUES)
			{
				labels.push_back(fmt::format("{:d}%", v));
			}
			m_popup.choose(
				"BRIGHTNESS",
				labels,
				nearest(BRIGHTNESS_VALUES, LaptopSettings::Brightness()),
				[refresh](size_t c)
				{
					LaptopSettings::SetBrightness(BRIGHTNESS_VALUES[c]);
					refresh();
				},
				3);
			break;
		}
		case 1:
			m_popup.chooseRows("SOUND",
							   {{"", {"OFF", "BUZZER"}, 170, 30}, {"SPEAKER", {"1", "2", "3", "4", "5"}, 98, 15}},
							   LaptopSettings::SoundSetting(),
							   [refresh](size_t c)
							   {
								   LaptopSettings::SetSoundSetting(static_cast<unsigned int>(c));
								   refresh();
							   });
			break;
		case 2:
			m_popup.choose("SCREEN DIMMING",
						   {"OFF", "1 MIN", "5 MIN", "10 MIN"},
						   nearest(DIM_VALUES, LaptopSettings::DimTimeout()),
						   [refresh](size_t c)
						   {
							   LaptopSettings::SetDimTimeout(DIM_VALUES[c]);
							   refresh();
						   });
			break;
		case 3:
			m_popup.choose("ORIENTATION",
						   {"NORMAL", "UPSIDE DOWN"},
						   LaptopSettings::UpsideDown() ? 1 : 0,
						   [refresh](size_t c)
						   {
							   LaptopSettings::SetUpsideDown(c == 1);
							   refresh();
						   });
			break;
		case 4:
			chooseConnection();
			break;
		case 5:
			m_popup.message("ABOUT",
							Tone::Info,
							"INFO",
							"",
							fmt::format("DuetScreen {:s} with the Funky UI.\nConnection: {:s}, printer at {:s}.",
										FIRMWARE_VERSION,
										linkName(LaptopSettings::Connection()),
										LaptopSettings::DuetIp()));
			break;
		case 8:
			m_popup.confirm("ALERT !",
							{"FACTORY RESET will return", "the screen settings", "to the default values. Continue?"},
							[refresh]
							{
								LaptopSettings::FactoryReset();
								refresh();
							});
			break;
		default:
			break;
		}
	}

	/* ---------------- events ---------------- */

	void Laptop::clicked(lv_event_t* e)
	{
		auto* self = static_cast<Laptop*>(lv_event_get_user_data(e));
		auto* target = static_cast<lv_obj_t*>(lv_event_get_current_target(e));

		for (size_t i = 0; i < KEY_COUNT; i++)
		{
			if (self->m_keys[i] == target)
			{
				self->pressKey(i);
				return;
			}
		}
		if (target == self->m_side[SIDE_STOP])
		{
			self->pressStop();
			return;
		}
		if (target == self->m_side[SIDE_ALERTS])
		{
			self->showTab(Tab::Alerts);
			return;
		}
		if (target == self->m_side[SIDE_CONSOLE])
		{
			self->showTab(Tab::Console);
			return;
		}
		if (target == self->m_side[SIDE_SETTINGS])
		{
			self->showTab(Tab::Settings);
			return;
		}
		if (target == self->m_side[SIDE_HOME])
		{
			self->pressHome();
			return;
		}
		if (target == self->m_input)
		{
			self->openKeyboard();
			return;
		}
		if (target == self->m_kbButton)
		{
			if (self->m_keyboardOpen)
				self->closeKeyboard();
			else
				self->openKeyboard();
			return;
		}
		for (size_t r = 0; r < self->m_alertRows.size(); r++)
		{
			if (self->m_alertRows[r].tile == target)
			{
				self->pressAlertRow(r);
				return;
			}
		}
		if (target == self->m_alertPager.up)
		{
			self->pressAlertPage(-1);
			return;
		}
		if (target == self->m_alertPager.down)
		{
			self->pressAlertPage(1);
			return;
		}
		if (target == self->m_alertPager.middle)
		{
			self->pressAlertFilter();
			return;
		}
		if (target == self->m_settingsPager.up)
		{
			self->pressSettingsPage(-1);
			return;
		}
		if (target == self->m_settingsPager.down)
		{
			self->pressSettingsPage(1);
			return;
		}
		for (size_t i = 0; i < self->m_slots.size(); i++)
		{
			if (self->m_slots[i].tile == target)
			{
				self->pressSetting(i);
				return;
			}
		}
	}

	void Laptop::ageTimer(lv_timer_t* t)
	{
		auto* self = static_cast<Laptop*>(lv_timer_get_user_data(t));
		self->refreshAges();
	}
} // namespace UI
