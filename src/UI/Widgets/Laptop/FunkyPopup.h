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

		Popup(Images& images, Fonts& fonts);
		~Popup() = default;
		Popup(const Popup&) = delete;
		Popup& operator=(const Popup&) = delete;

		/* The full-screen object the popup covers (built on first use) */
		void attach(lv_obj_t* screen);

		void choose(const std::string& title,
					const std::vector<std::string>& choices,
					size_t selected,
					std::function<void(size_t)> onOk,
					size_t columns = 0);
		void chooseRows(const std::string& title,
						const std::vector<Row>& rows,
						size_t selected,
						std::function<void(size_t)> onOk);
		void confirm(const std::string& title, const std::vector<std::string>& lines, std::function<void()> onOk);
		void message(const std::string& title,
					 Tone tone,
					 const std::string& badge,
					 const std::string& meta,
					 const std::string& text);
		void close();
		bool isOpen() const { return m_open; }

		/* Touches (also used by the tests) */
		void pressChoice(size_t index);
		void pressOk();
		void pressCancel();
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
		void addButtons(bool withOk);
		lv_obj_t* addButton(const char* image, int32_t modernX);
		void showSelection();
		static void clicked(lv_event_t* e);

		Images& m_images;
		Fonts& m_fonts;
		lv_obj_t* m_root = nullptr;	   /* dims the page and blocks its touches */
		lv_obj_t* m_frame = nullptr;
		lv_obj_t* m_content = nullptr; /* rebuilt for every popup */
		lv_obj_t* m_okButton = nullptr;
		lv_obj_t* m_cancelButton = nullptr;
		std::vector<lv_obj_t*> m_choices;

		bool m_open = false;
		std::string m_title;
		size_t m_pending = 0;
		std::function<void(size_t)> m_onChoice;
		std::function<void()> m_onConfirm;
	};
} // namespace UI::Funky
