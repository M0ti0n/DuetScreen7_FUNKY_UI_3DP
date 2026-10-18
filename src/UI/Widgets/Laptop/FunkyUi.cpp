/*
 * FunkyUi.cpp
 */

#include "FunkyUi.h"
#include "Debug.h"
#include "UI/Styles/Styles.h"
#include "lvgl/src/draw/lv_image_decoder_private.h"

namespace UI::Funky
{
	static constexpr const char* WORKSHOP_FOLDER = ASSETS_FOLDER "workshop/";

	/* ---------------- images ---------------- */

	Images::~Images()
	{
		clear();
	}

	void Images::clear()
	{
		for (auto& [path, buf] : m_bufs)
		{
			if (buf != nullptr)
			{
				lv_draw_buf_destroy(buf);
			}
		}
		m_bufs.clear();
	}

	void Images::release(const std::string& relPath)
	{
		if (auto it = m_bufs.find(relPath); it != m_bufs.end())
		{
			if (it->second != nullptr)
			{
				lv_draw_buf_destroy(it->second);
			}
			m_bufs.erase(it);
		}
	}

	/* ARGB8888 / XRGB8888 to RGB888 (both B, G, R in memory) */
	static lv_draw_buf_t* toRgb888(const lv_draw_buf_t* src)
	{
		const lv_color_format_t cf = static_cast<lv_color_format_t>(src->header.cf);
		if (cf != LV_COLOR_FORMAT_ARGB8888 && cf != LV_COLOR_FORMAT_XRGB8888)
		{
			return nullptr;
		}
		const uint32_t w = src->header.w, h = src->header.h;
		lv_draw_buf_t* dst = lv_draw_buf_create(w, h, LV_COLOR_FORMAT_RGB888, LV_STRIDE_AUTO);
		if (dst == nullptr)
		{
			return nullptr;
		}
		for (uint32_t y = 0; y < h; y++)
		{
			const uint8_t* in = src->data + y * src->header.stride;
			uint8_t* out = dst->data + y * dst->header.stride;
			for (uint32_t x = 0; x < w; x++)
			{
				out[0] = in[0];
				out[1] = in[1];
				out[2] = in[2];
				in += 4;
				out += 3;
			}
		}
		return dst;
	}

	const lv_draw_buf_t* Images::get(const std::string& relPath, bool opaque)
	{
		ZoneScoped;
		if (auto it = m_bufs.find(relPath); it != m_bufs.end())
		{
			return it->second;
		}
		const std::string path = std::string(WORKSHOP_FOLDER) + relPath;
		lv_draw_buf_t* copy = nullptr;
		lv_image_decoder_dsc_t dsc{};
		if (lv_image_decoder_open(&dsc, path.c_str(), nullptr) == LV_RESULT_OK && dsc.decoded != nullptr)
		{
			copy = opaque ? toRgb888(dsc.decoded) : nullptr;
			if (copy == nullptr)
			{
				copy = lv_draw_buf_dup(dsc.decoded);
			}
			lv_image_decoder_close(&dsc);
		}
		if (copy == nullptr)
		{
			LOG_ERROR("Funky: failed to load {:s}", path);
		}
		m_bufs[relPath] = copy;
		return copy;
	}

	/* ---------------- fonts ---------------- */

	const lv_font_t* Fonts::get(uint32_t size, bool bold, bool mono)
	{
		const uint32_t key = size | (bold ? 0x10000u : 0u) | (mono ? 0x20000u : 0u);
		auto it = m_fonts.find(key);
		if (it == m_fonts.end())
		{
			it = m_fonts
					 .emplace(key,
							  FontManager::createFont(mono ? "GoogleSansCode" : "Montserrat",
													  size,
													  bold ? LV_FREETYPE_FONT_STYLE_BOLD : LV_FREETYPE_FONT_STYLE_NORMAL))
					 .first;
		}
		return it->second.get() != nullptr ? it->second.get() : LV_FONT_DEFAULT;
	}

	/* ---------------- objects ---------------- */

	lv_obj_t* box(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h)
	{
		lv_obj_t* obj = lv_obj_create(parent);
		lv_obj_remove_style_all(obj);
		lv_obj_set_pos(obj, x, y);
		lv_obj_set_size(obj, w, h);
		lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
		return obj;
	}

	void setTileLook(lv_obj_t* t, TileLook look)
	{
		uint32_t fill = Colour::TILE, outline = Colour::OUTLINE;
		int32_t width = 2, radius = 8;
		lv_opa_t opa = LV_OPA_COVER;
		switch (look)
		{
		case TileLook::Normal:
			break;
		case TileLook::Selected:
			fill = Colour::TILE_SELECTED;
			outline = Colour::ACCENT;
			width = 3;
			break;
		case TileLook::Stop:
			fill = Colour::STOP_FILL;
			outline = Colour::STOP_OUTLINE;
			break;
		case TileLook::Danger:
			fill = Colour::DANGER_FILL;
			outline = Colour::DANGER_OUTLINE;
			break;
		case TileLook::Disabled:
			outline = 0x2C3A50;
			break;
		case TileLook::Input:
			fill = Colour::INPUT_FILL;
			break;
		case TileLook::PopupCard:
			fill = Colour::POPUP_TILE;
			outline = Colour::POPUP_OUTLINE;
			radius = 14;
			break;
		case TileLook::PopupChoice:
			fill = Colour::POPUP_CHOICE;
			outline = Colour::POPUP_OUTLINE;
			radius = 11;
			break;
		case TileLook::PopupChoiceSelected:
			fill = Colour::POPUP_CHOICE;
			outline = Colour::ACCENT;
			width = 4;
			radius = 11;
			break;
		}
		lv_obj_set_style_radius(t, radius, 0);
		lv_obj_set_style_bg_color(t, col(fill), 0);
		lv_obj_set_style_bg_opa(t, opa, 0);
		lv_obj_set_style_border_color(t, col(outline), 0);
		lv_obj_set_style_border_width(t, width, 0);
		lv_obj_set_style_border_opa(t, LV_OPA_COVER, 0);
	}

	lv_obj_t* tile(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h, TileLook look, bool clickable)
	{
		lv_obj_t* t = box(parent, x, y, w, h);
		setTileLook(t, look);
		if (clickable)
		{
			lv_obj_add_flag(t, LV_OBJ_FLAG_CLICKABLE);
			lv_obj_set_style_bg_color(t, lv_color_black(), LV_STATE_PRESSED);
			lv_obj_set_style_bg_opa(t, LV_OPA_50, LV_STATE_PRESSED);
		}
		return t;
	}

	lv_obj_t* label(lv_obj_t* parent, const lv_font_t* font, uint32_t colour, const char* text)
	{
		lv_obj_t* l = lv_label_create(parent);
		lv_obj_set_style_text_font(l, font, 0);
		lv_obj_set_style_text_color(l, col(colour), 0);
		lv_label_set_text(l, text);
		lv_obj_remove_flag(l, LV_OBJ_FLAG_CLICKABLE);
		return l;
	}

	lv_obj_t* centredLabel(lv_obj_t* parent, const lv_font_t* font, uint32_t colour, const char* text, int32_t dy)
	{
		lv_obj_t* l = label(parent, font, colour, text);
		lv_obj_align(l, LV_ALIGN_CENTER, 0, dy);
		return l;
	}

	lv_obj_t* tapZone(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, bool feedback)
	{
		lv_obj_t* z = box(parent, x, y, w, h);
		lv_obj_add_flag(z, LV_OBJ_FLAG_CLICKABLE);
		if (feedback)
		{
			lv_obj_set_style_radius(z, radius, 0);
			lv_obj_set_style_bg_color(z, lv_color_black(), LV_STATE_PRESSED);
			lv_obj_set_style_bg_opa(z, LV_OPA_40, LV_STATE_PRESSED);
		}
		return z;
	}

	void setHidden(lv_obj_t* obj, bool hidden)
	{
		if (obj == nullptr)
		{
			return;
		}
		if (hidden)
			lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
		else
			lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
	}

	void setEnabled(lv_obj_t* obj, bool enabled)
	{
		if (obj == nullptr)
		{
			return;
		}
		if (enabled)
			lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
		else
			lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
	}

	static lv_point_precise_t pt(int32_t x, int32_t y)
	{
		return {static_cast<lv_value_precise_t>(x), static_cast<lv_value_precise_t>(y)};
	}

	static lv_obj_t* polyline(lv_obj_t* parent,
							  std::initializer_list<lv_point_precise_t> pts,
							  uint32_t colour,
							  int32_t width,
							  std::vector<std::unique_ptr<lv_point_precise_t[]>>& store)
	{
		auto arr = std::make_unique<lv_point_precise_t[]>(pts.size());
		size_t i = 0;
		for (const auto& p : pts)
		{
			arr[i++] = p;
		}
		lv_obj_t* line = lv_line_create(parent);
		lv_obj_remove_style_all(line);
		lv_obj_set_pos(line, 0, 0);
		lv_line_set_points(line, arr.get(), static_cast<uint32_t>(pts.size()));
		lv_obj_set_style_line_color(line, col(colour), 0);
		lv_obj_set_style_line_width(line, width, 0);
		lv_obj_set_style_line_rounded(line, true, 0);
		lv_obj_remove_flag(line, LV_OBJ_FLAG_CLICKABLE);
		store.push_back(std::move(arr));
		return line;
	}

	lv_obj_t* chevron(lv_obj_t* parent, int32_t cx, int32_t cy, bool up, uint32_t colour,
					  std::vector<std::unique_ptr<lv_point_precise_t[]>>& store)
	{
		const int32_t s = up ? 1 : -1;
		return polyline(parent, {pt(cx - 14, cy + 7 * s), pt(cx, cy - 8 * s), pt(cx + 14, cy + 7 * s)}, colour, 4, store);
	}

	void houseIcon(lv_obj_t* parent, int32_t cx, int32_t cy, int32_t s, uint32_t colour,
				   std::vector<std::unique_ptr<lv_point_precise_t[]>>& store)
	{
		/* The Modern UI home-axis house, with a door (screen home, not axis homing) */
		auto f = [s](int32_t percent) { return s * percent / 100; };
		polyline(parent, {pt(cx - f(130), cy - f(5)), pt(cx, cy - s), pt(cx + f(130), cy - f(5))}, colour, 2, store);
		polyline(parent,
				 {pt(cx - f(85), cy - f(38)), pt(cx - f(85), cy + f(95)), pt(cx + f(85), cy + f(95)),
				  pt(cx + f(85), cy - f(38))},
				 colour,
				 2,
				 store);
		polyline(parent,
				 {pt(cx - f(28), cy + f(95)), pt(cx - f(28), cy + f(20)), pt(cx + f(28), cy + f(20)),
				  pt(cx + f(28), cy + f(95))},
				 colour,
				 2,
				 store);
		lv_obj_t* knob = box(parent, cx + f(10), cy + f(53), 3, 3);
		lv_obj_set_style_radius(knob, LV_RADIUS_CIRCLE, 0);
		lv_obj_set_style_bg_color(knob, col(colour), 0);
		lv_obj_set_style_bg_opa(knob, LV_OPA_COVER, 0);
	}

	void keyboardIcon(lv_obj_t* parent, int32_t cx, int32_t cy, int32_t s, uint32_t colour)
	{
		const int32_t w = s * 28 / 10, h = s * 16 / 10;
		lv_obj_t* frame = box(parent, cx - w / 2, cy - h / 2, w, h);
		lv_obj_set_style_radius(frame, 3, 0);
		lv_obj_set_style_border_color(frame, col(colour), 0);
		lv_obj_set_style_border_width(frame, 2, 0);
		lv_obj_set_style_border_opa(frame, LV_OPA_COVER, 0);
		auto dot = [&](int32_t x, int32_t y, int32_t dw)
		{
			lv_obj_t* d = box(parent, x, y, dw, 2);
			lv_obj_set_style_bg_color(d, col(colour), 0);
			lv_obj_set_style_bg_opa(d, LV_OPA_COVER, 0);
		};
		const int32_t step = s * 52 / 100;
		for (int32_t r = 0; r < 2; r++)
		{
			for (int32_t i = 0; i < 5; i++)
			{
				dot(cx - s * 105 / 100 + i * step, cy - s * 42 / 100 + r * s * 42 / 100, 2);
			}
		}
		dot(cx - s * 6 / 10, cy + s * 4 / 10, s * 12 / 10);
	}
} // namespace UI::Funky
