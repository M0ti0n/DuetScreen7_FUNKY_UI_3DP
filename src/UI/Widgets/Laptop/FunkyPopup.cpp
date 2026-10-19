/*
 * FunkyPopup.cpp
 */

#include "FunkyPopup.h"
#include "Debug.h"
#include <algorithm>
#include <cmath>
#include <fmt/format.h>

namespace UI::Funky
{
	/* The frame image is the 660 x 386 popup scaled by K and placed at FRAME_X / FRAME_Y.
	 * Its glass (34,34)-(662,382) holds the Modern UI popup content. */
	static constexpr double K = 1.43;
	static constexpr double FX = 0.98064516; /* (628 - 20) / 620 */
	static constexpr double FY = 0.84;		 /* (348 - 12) / 400 */
	static constexpr int32_t FRAME_X = 14;
	static constexpr int32_t FRAME_Y = 4;
	static constexpr int32_t SCREEN_W = 1024;
	static constexpr int32_t SCREEN_H = 600;
	static constexpr double BUTTON_TRASH_X = 102, BUTTON_MID_X = 275, BUTTON_RIGHT_X = 448;
	static constexpr uint32_t INK = 0x161412;
	static constexpr uint32_t PAPER = 0xF7F4EB;
	static constexpr uint32_t PAPER_DIM = 0x5C5448;
	static constexpr uint32_t PAPER_BLUE = 0x2668C4;

	/* Modern UI positions */
	static constexpr double TILE_Y = 35, TILE_H = 60;
	static constexpr double BUTTON_Y = 332, BUTTON_W = 110;
	static constexpr double BUTTON_CANCEL_X = 189, BUTTON_OK_X = 362, BUTTON_ALONE_X = 275;
	static constexpr double AREA_X = 55, AREA_W = 550, AREA_Y = 118, AREA_H = 196;
	static constexpr double CHOICE_H = 60, CHOICE_GAP_Y = 16, CAPTION_H = 34;

	int32_t Popup::mapX(double v)
	{
		return FRAME_X + static_cast<int32_t>(std::lround(K * (44.0 + (v - 20.0) * FX)));
	}

	int32_t Popup::mapY(double v)
	{
		return FRAME_Y + static_cast<int32_t>(std::lround(K * (40.0 + (v - 22.0) * FY)));
	}

	int32_t Popup::mapW(double v)
	{
		return static_cast<int32_t>(std::lround(K * FX * v));
	}

	int32_t Popup::mapH(double v)
	{
		return static_cast<int32_t>(std::lround(K * FY * v));
	}

	Popup::Popup(Images& images, Fonts& fonts)
		: m_images(images)
		, m_fonts(fonts)
	{
	}

	void Popup::attach(lv_obj_t* screen)
	{
		if (m_root != nullptr || screen == nullptr)
		{
			return;
		}
		m_root = box(screen, 0, 0, SCREEN_W, SCREEN_H);
		lv_obj_add_flag(m_root, LV_OBJ_FLAG_CLICKABLE); /* the page below gets no touches */
		lv_obj_set_style_bg_color(m_root, lv_color_hex(0x080605), 0);
		lv_obj_set_style_bg_opa(m_root, LV_OPA_50, 0);

		m_frame = lv_image_create(m_root); /* the image is loaded while a popup is open */
		lv_obj_set_pos(m_frame, FRAME_X, FRAME_Y);
		lv_obj_remove_flag(m_frame, LV_OBJ_FLAG_CLICKABLE);

		m_content = box(m_root, 0, 0, SCREEN_W, SCREEN_H);
		setHidden(m_root, true);
	}

	void Popup::open(const std::string& title, Tone tone, bool longTitle)
	{
		if (m_root == nullptr)
		{
			LOG_ERROR("Funky popup used before attach()");
			return;
		}
		if (m_frameLoaded != frameImage())
		{
			if (!m_frameLoaded.empty())
			{
				lv_image_set_src(m_frame, nullptr);
				m_images.release(m_frameLoaded);
			}
			m_frameLoaded = frameImage();
			lv_image_set_src(m_frame, m_images.get(m_frameLoaded));
		}
		lv_obj_clean(m_content);
		m_choices.clear();
		m_okButton = m_cancelButton = m_trashButton = nullptr;
		m_onChoice = nullptr;
		m_onConfirm = nullptr;
		m_onTrash = nullptr;
		m_pending = 0;
		m_title = title;

		/* Name tile: centred, glass-coloured, outline by tone */
		const double w = longTitle ? 550 : 250, x = longTitle ? 55 : 205;
		lv_obj_t* t = card(mapX(x), mapY(TILE_Y), mapW(w), mapH(TILE_H));
		const uint32_t info = m_theme == PopupTheme::Paper ? INK : Colour::ACCENT;
		const uint32_t outline = tone == Tone::Error ? Colour::RED : tone == Tone::Warning ? Colour::AMBER : info;
		lv_obj_set_style_border_color(t, col(outline), 0);
		lv_obj_set_style_border_width(t, 4, 0);
		centredLabel(t, m_fonts.get(34, true), textColour(), title.c_str(), 1);

		lv_obj_move_to_index(m_root, -1);
		setHidden(m_root, false);
		m_open = true;
	}

	lv_obj_t* Popup::addButton(const char* image, int32_t modernX)
	{
		lv_obj_t* b = lv_image_create(m_content);
		lv_image_set_src(b, m_images.get(image));
		lv_obj_set_pos(b, mapX(modernX), mapY(BUTTON_Y));
		lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_set_style_image_recolor(b, lv_color_black(), LV_STATE_PRESSED);
		lv_obj_set_style_image_recolor_opa(b, LV_OPA_40, LV_STATE_PRESSED);
		lv_obj_add_event_cb(b, clicked, LV_EVENT_CLICKED, this);
		return b;
	}

	std::string Popup::button(const char* name) const
	{
		return fmt::format("popup/btn_{:s}{:s}.png", name, m_theme == PopupTheme::Paper ? "_paper" : "");
	}

	const char* Popup::frameImage() const
	{
		return m_theme == PopupTheme::Paper ? "popup/frame_paper.png" : "popup/frame.png";
	}

	uint32_t Popup::textColour() const
	{
		return m_theme == PopupTheme::Paper ? INK : Colour::TEXT;
	}

	uint32_t Popup::dimColour() const
	{
		return m_theme == PopupTheme::Paper ? PAPER_DIM : Colour::POPUP_DIM;
	}

	lv_obj_t* Popup::card(int32_t x, int32_t y, int32_t w, int32_t h)
	{
		lv_obj_t* t = tile(m_content, x, y, w, h, TileLook::PopupCard, false);
		if (m_theme == PopupTheme::Paper)
		{
			lv_obj_set_style_bg_color(t, col(PAPER), 0);
			lv_obj_set_style_border_color(t, col(INK), 0);
			lv_obj_set_style_border_width(t, 3, 0);
		}
		return t;
	}

	void Popup::addButtons(bool withOk, bool withTrash)
	{
		if (withTrash && !withOk)
		{
			m_trashButton = addButton(button("trash").c_str(), static_cast<int32_t>(BUTTON_CANCEL_X));
			m_cancelButton = addButton(button("x").c_str(), static_cast<int32_t>(BUTTON_OK_X));
		}
		else if (withTrash)
		{
			m_trashButton = addButton(button("trash").c_str(), static_cast<int32_t>(BUTTON_TRASH_X));
			m_cancelButton = addButton(button("x").c_str(), static_cast<int32_t>(BUTTON_MID_X));
			m_okButton = addButton(button("check").c_str(), static_cast<int32_t>(BUTTON_RIGHT_X));
		}
		else if (withOk)
		{
			m_cancelButton = addButton(button("x").c_str(), static_cast<int32_t>(BUTTON_CANCEL_X));
			m_okButton = addButton(button("check").c_str(), static_cast<int32_t>(BUTTON_OK_X));
		}
		else
		{
			m_cancelButton = addButton(button("x").c_str(), static_cast<int32_t>(BUTTON_ALONE_X));
		}
	}

	void Popup::choose(const std::string& title,
					   const std::vector<std::string>& choices,
					   size_t selected,
					   std::function<void(size_t)> onOk,
					   size_t columns)
	{
		/* Up to 4 in one row, else 3 per row; or the given number of columns */
		const size_t n = choices.size();
		const size_t perRow = (columns != 0 && columns <= n) ? columns : (n <= 4 ? n : 3);
		std::vector<Row> rows;
		for (size_t i = 0; i < n; i += perRow)
		{
			Row r;
			r.items.assign(choices.begin() + static_cast<std::ptrdiff_t>(i),
						   choices.begin() + static_cast<std::ptrdiff_t>(std::min(n, i + perRow)));
			r.itemWidth =
				static_cast<int32_t>((AREA_W - static_cast<double>(perRow - 1) * 20.0) / static_cast<double>(perRow));
			rows.push_back(std::move(r));
		}
		chooseRows(title, rows, selected, std::move(onOk));
	}

	void Popup::chooseRows(const std::string& title,
						   const std::vector<Row>& rows,
						   size_t selected,
						   std::function<void(size_t)> onOk)
	{
		const bool longTitle = title.size() > 11;
		open(title, Tone::Info, longTitle);
		if (!m_open)
		{
			return;
		}
		m_onChoice = std::move(onOk);

		double blockH = 0;
		for (size_t r = 0; r < rows.size(); r++)
		{
			blockH += CHOICE_H + (r > 0 ? CHOICE_GAP_Y : 0.0) + (rows[r].caption.empty() ? 0.0 : CAPTION_H);
		}
		double y = AREA_Y + (AREA_H - blockH) / 2.0;
		for (const Row& row : rows)
		{
			if (!row.caption.empty())
			{
				lv_obj_t* c = label(m_content, m_fonts.get(27), dimColour(), row.caption.c_str());
				lv_obj_set_width(c, mapW(AREA_W));
				lv_obj_set_style_text_align(c, LV_TEXT_ALIGN_CENTER, 0);
				lv_obj_set_pos(c, mapX(AREA_X), mapY(y + 2));
				y += CAPTION_H;
			}
			const double n = static_cast<double>(row.items.size());
			const double w = row.itemWidth > 0 ? row.itemWidth : (AREA_W - (n - 1) * row.gap) / n;
			double x = AREA_X + (AREA_W - (n * w + (n - 1) * row.gap)) / 2.0;
			for (const std::string& item : row.items)
			{
				lv_obj_t* t = tile(m_content, mapX(x), mapY(y), mapW(w), mapH(CHOICE_H), TileLook::PopupChoice, true);
				centredLabel(t, m_fonts.get(29, true), textColour(), item.c_str(), 1);
				lv_obj_add_event_cb(t, clicked, LV_EVENT_CLICKED, this);
				m_choices.push_back(t);
				x += w + row.gap;
			}
			y += CHOICE_H + CHOICE_GAP_Y;
		}
		m_pending = selected < m_choices.size() ? selected : 0;
		showSelection();
		addButtons(true);
	}

	void Popup::confirm(const std::string& title,
						const std::vector<std::string>& lines,
						std::function<void()> onOk,
						std::function<void()> onTrash)
	{
		open(title, Tone::Info, title.size() > 11);
		if (!m_open)
		{
			return;
		}
		m_onConfirm = std::move(onOk);
		m_onTrash = std::move(onTrash);
		card(mapX(AREA_X), mapY(139), mapW(AREA_W), mapH(150));
		static constexpr double ys[4][4] = {{205, 0, 0, 0}, {186, 226, 0, 0}, {170, 205, 240, 0}, {158, 190, 222, 254}};
		const size_t n = std::min<size_t>(lines.size(), 4);
		for (size_t i = 0; i < n; i++)
		{
			const bool last = (i + 1 == n);
			lv_obj_t* l = label(m_content,
								m_fonts.get(29, n > 1 && last),
								(n == 1 || last) ? textColour() : dimColour(),
								lines[i].c_str());
			lv_obj_set_width(l, mapW(AREA_W));
			lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
			lv_obj_set_pos(l, mapX(AREA_X), mapY(ys[n - 1][i] + 11) - 17);
		}
		addButtons(m_onConfirm != nullptr, m_onTrash != nullptr);
	}

	void Popup::message(
		const std::string& title, Tone tone, const std::string& badge, const std::string& meta, const std::string& text)
	{
		open(title, tone, false);
		if (!m_open)
		{
			return;
		}
		const int32_t cx = mapX(AREA_X), cy = mapY(118), cw = mapW(AREA_W), ch = mapH(196);
		card(cx, cy, cw, ch);

		const uint32_t toneColour = tone == Tone::Error			   ? Colour::RED
									: tone == Tone::Warning		   ? Colour::AMBER
									: m_theme == PopupTheme::Paper ? INK
																   : Colour::ACCENT;
		lv_obj_t* b = box(m_content, cx + 26, cy + 20, 86, 37);
		lv_obj_set_style_radius(b, 7, 0);
		lv_obj_set_style_border_color(b, col(toneColour), 0);
		lv_obj_set_style_border_width(b, 3, 0);
		lv_obj_set_style_border_opa(b, LV_OPA_COVER, 0);
		centredLabel(b, m_fonts.get(21, true), toneColour, badge.c_str(), 1);

		lv_obj_t* m = label(m_content, m_fonts.get(24), dimColour(), meta.c_str());
		lv_obj_set_pos(m, cx + 132, cy + 25);

		lv_obj_t* body = label(m_content, m_fonts.get(26), textColour(), text.c_str());
		lv_label_set_long_mode(body, LV_LABEL_LONG_MODE_WRAP);
		lv_obj_set_width(body, cw - 52);
		lv_obj_set_style_text_line_space(body, 6, 0);
		lv_obj_set_pos(body, cx + 26, cy + 70);
		lv_obj_set_style_max_height(body, ch - 84, 0);
		addButtons(false);
	}

	void Popup::close()
	{
		if (m_root == nullptr)
		{
			return;
		}
		setHidden(m_root, true);
		if (m_open)
		{
			/* free the frame (LVGL's heap is fixed) and the content */
			lv_obj_clean(m_content);
			m_choices.clear();
			m_okButton = m_cancelButton = m_trashButton = nullptr;
			lv_image_set_src(m_frame, nullptr);
			m_images.release(m_frameLoaded);
			m_frameLoaded.clear();
		}
		m_open = false;
	}

	void Popup::showSelection()
	{
		for (size_t i = 0; i < m_choices.size(); i++)
		{
			setTileLook(m_choices[i], i == m_pending ? TileLook::PopupChoiceSelected : TileLook::PopupChoice);
			if (m_theme == PopupTheme::Paper)
			{
				lv_obj_set_style_bg_color(m_choices[i], col(PAPER), 0);
				lv_obj_set_style_border_color(m_choices[i], col(i == m_pending ? PAPER_BLUE : INK), 0);
				lv_obj_set_style_border_width(m_choices[i], i == m_pending ? 4 : 2, 0);
			}
			lv_obj_set_style_bg_color(m_choices[i], lv_color_black(), LV_STATE_PRESSED);
			lv_obj_set_style_bg_opa(m_choices[i], LV_OPA_50, LV_STATE_PRESSED);
		}
	}

	void Popup::pressChoice(size_t index)
	{
		if (!m_open || index >= m_choices.size())
		{
			return;
		}
		m_pending = index;
		showSelection();
	}

	void Popup::pressOk()
	{
		if (!m_open || m_okButton == nullptr)
		{
			return;
		}
		/* Close first: the callback may open the next popup */
		auto onChoice = std::move(m_onChoice);
		auto onConfirm = std::move(m_onConfirm);
		const size_t choice = m_pending;
		close();
		if (onChoice)
		{
			onChoice(choice);
		}
		if (onConfirm)
		{
			onConfirm();
		}
	}

	void Popup::pressTrash()
	{
		if (!m_open || m_trashButton == nullptr)
		{
			return;
		}
		auto onTrash = std::move(m_onTrash);
		close();
		if (onTrash)
		{
			onTrash();
		}
	}

	void Popup::pressCancel()
	{
		if (!m_open)
		{
			return;
		}
		close();
	}

	void Popup::clicked(lv_event_t* e)
	{
		auto* self = static_cast<Popup*>(lv_event_get_user_data(e));
		auto* target = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
		if (target == self->m_okButton)
		{
			self->pressOk();
			return;
		}
		if (target == self->m_trashButton)
		{
			self->pressTrash();
			return;
		}
		if (target == self->m_cancelButton)
		{
			self->pressCancel();
			return;
		}
		for (size_t i = 0; i < self->m_choices.size(); i++)
		{
			if (self->m_choices[i] == target)
			{
				self->pressChoice(i);
				return;
			}
		}
	}
} // namespace UI::Funky
