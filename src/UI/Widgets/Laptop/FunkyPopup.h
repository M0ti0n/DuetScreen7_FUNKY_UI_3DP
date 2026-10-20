/*
 * FunkyPopup.h
 *
 * The standard popup of the Funky UI (docs/workshop/POPUPS.md): a printer-screen frame
 * over a dimmed page, a centred name tile on top, the content in Modern UI positions
 * (its 660 x 460 popup mapped onto our glass) and the icon buttons at the bottom:
 * X (red) cancels, check (green) confirms.
 *
 *   choose()   preset tiles; tap selects, check applies, X closes
 *   confirm()  info card with up to 4 lines; X / check
 *   message()  ERROR / WARNING / INFO with the whole text; X only
 *
 * The name tile outline follows the tone: blue (info), amber (warning), red (error).
 */

#pragma once

#include "FunkyUi.h"
#include "lvgl/lvgl.h"
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace UI::Funky
{
	enum class Tone : uint8_t
	{
		Info,
		Warning,
		Error,
	};

	/* Glass: the printer-screen look (laptop, everywhere else). Paper: the folders on the desk
	 * (object cancel, jobs): cardboard glass, paper tiles with ink outlines, ink icons. */
	enum class PopupTheme : uint8_t
	{
		Glass,
		Paper,
	};

	class Popup
	{
	  public:
		struct Row
		{
			std::string caption;			/* small text above the row ("" = none) */
			std::vector<std::string> items; /* tiles, left to right */
			int32_t itemWidth = 0;			/* Modern UI units; 0 = fill the row */
			int32_t gap = 20;
		};

		/* A paged picker (Modern UI filament popups): text tiles or colour swatches, ▲ / ▼ tiles
		 * on the right when there is more than one page, an optional text button left of X. */
		struct Picker
		{
			std::string title;
			std::string info;				 /* dim line under the title */
			std::vector<std::string> labels; /* text tiles... */
			std::vector<uint32_t> swatches;	 /* ...or colour tiles (RGB) */
			size_t perPage = 6;
			size_t columns = 3;
			int selected = -1;				  /* item index, -1 = none */
			std::string extra;				  /* "UNLOAD", "CLEAR", "" = none */
			std::function<void(size_t)> onOk; /* the item chosen (check) */
			std::function<void()> onCancel;	  /* X */
			std::function<void()> onExtra;
		};

		Popup(Images& images, Fonts& fonts);
		~Popup() = default;
		Popup(const Popup&) = delete;
		Popup& operator=(const Popup&) = delete;

		/* The full-screen object the popup covers (built on first use) */
		void attach(lv_obj_t* screen);
		void setTheme(PopupTheme theme) { m_theme = theme; }

		void choose(const std::string& title,
					const std::vector<std::string>& choices,
					size_t selected,
					std::function<void(size_t)> onOk,
					size_t columns = 0);
		void chooseRows(const std::string& title,
						const std::vector<Row>& rows,
						size_t selected,
						std::function<void(size_t)> onOk);
		/* onTrash: adds the bin button left of X (Modern UI order: bin, X, check).
		 * onOk nullptr: no check button (bin, X or X alone) */
		void confirm(const std::string& title,
					 const std::vector<std::string>& lines,
					 std::function<void()> onOk,
					 std::function<void()> onTrash = nullptr);
		void message(const std::string& title,
					 Tone tone,
					 const std::string& badge,
					 const std::string& meta,
					 const std::string& text);
		void pick(const Picker& picker);
		void close();
		bool isOpen() const { return m_open; }

		/* Touches (also used by the tests) */
		void pressChoice(size_t index);
		void pressOk();
		void pressCancel();
		void pressTrash();
		void pressExtra();
		void pressPickPage(int dir);
		bool hasTrash() const { return m_trashButton != nullptr; }
		bool hasExtra() const { return m_extraButton != nullptr; }
		int pickSelected() const { return m_pickSelected; }
		size_t pickPage() const { return m_pickPage; }
		size_t pickPages() const;
		size_t pending() const { return m_pending; }
		size_t choiceCount() const { return m_choices.size(); }
		const std::string& title() const { return m_title; }

		/* Modern UI popup coordinates (660 x 460) to screen pixels */
		static int32_t mapX(double v);
		static int32_t mapY(double v);
		static int32_t mapW(double v);
		static int32_t mapH(double v);

	  private:
		void open(const std::string& title, Tone tone, bool longTitle);
		void addButtons(bool withOk, bool withTrash = false);
		lv_obj_t* addButton(const char* image, int32_t modernX);
		std::string button(const char* name) const;
		const char* frameImage() const;
		uint32_t textColour() const;
		uint32_t dimColour() const;
		lv_obj_t* card(int32_t x, int32_t y, int32_t w, int32_t h);
		void showSelection();
		void renderPick();
		static void clicked(lv_event_t* e);

		Images& m_images;
		Fonts& m_fonts;
		lv_obj_t* m_root = nullptr; /* dims the page and blocks its touches */
		lv_obj_t* m_frame = nullptr;
		lv_obj_t* m_content = nullptr; /* rebuilt for every popup */
		lv_obj_t* m_okButton = nullptr;
		lv_obj_t* m_cancelButton = nullptr;
		lv_obj_t* m_trashButton = nullptr;
		lv_obj_t* m_extraButton = nullptr;
		lv_obj_t* m_pageUp = nullptr;
		lv_obj_t* m_pageDown = nullptr;
		std::vector<lv_obj_t*> m_choices;
		std::vector<std::unique_ptr<lv_point_precise_t[]>> m_points;
		bool m_picking = false;
		Picker m_picker;
		size_t m_pickPage = 0;
		int m_pickSelected = -1;
		PopupTheme m_theme = PopupTheme::Glass;
		std::string m_frameLoaded;

		bool m_open = false;
		std::string m_title;
		size_t m_pending = 0;
		std::function<void(size_t)> m_onChoice;
		std::function<void()> m_onConfirm;
		std::function<void()> m_onTrash;
		std::function<void()> m_onCancel;
	};
} // namespace UI::Funky
