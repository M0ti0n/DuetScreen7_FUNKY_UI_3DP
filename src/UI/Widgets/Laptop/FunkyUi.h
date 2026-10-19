/*
 * FunkyUi.h
 *
 * Small building blocks shared by the laptop page and the popups: colours of the
 * "glass" screens, an image store (PNG decoded once), a font store, and helpers
 * for the tiles, labels and tap zones drawn with plain LVGL objects.
 *
 * Raw lv_obj user data must not be used: DuetScreen keeps the LvObj wrapper there.
 */

#pragma once

#include "UI/Styles/Font.h"
#include "lvgl/lvgl.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace UI::Funky
{
	/* Glass screen colours (same as the mock-ups) */
	namespace Colour
	{
		constexpr uint32_t TEXT = 0xE8F1FB;
		constexpr uint32_t DIM = 0x96AAC4;
		constexpr uint32_t ACCENT = 0x7FD0F2;
		constexpr uint32_t TILE = 0x1A283E;
		constexpr uint32_t TILE_SELECTED = 0x203450;
		constexpr uint32_t OUTLINE = 0x40587A;
		constexpr uint32_t DISABLED = 0x5A6980;
		constexpr uint32_t RED = 0xD6402E;
		constexpr uint32_t AMBER = 0xF0AA3C;
		constexpr uint32_t STOP_FILL = 0xAA281E;
		constexpr uint32_t STOP_OUTLINE = 0xEB5F4B;
		constexpr uint32_t DANGER_FILL = 0x963224;
		constexpr uint32_t DANGER_OUTLINE = 0xC8503C;
		constexpr uint32_t COMMAND = 0xAADCFF; /* typed G-code in the console */
		constexpr uint32_t INPUT_FILL = 0x0A121E;
		/* popup glass */
		constexpr uint32_t POPUP_TILE = 0x233D60;
		constexpr uint32_t POPUP_CHOICE = 0x284468;
		constexpr uint32_t POPUP_OUTLINE = 0x466288;
		constexpr uint32_t POPUP_DIM = 0x9FB7D4;
	} // namespace Colour

	inline lv_color_t col(uint32_t hex)
	{
		return lv_color_hex(hex);
	}

	/* PNGs decoded once and kept in RAM until released. LVGL has a fixed heap (LV_MEM_SIZE),
	 * so opaque art is kept as RGB888 (3 bytes a pixel) and pages release theirs when hidden. */
	class Images
	{
	  public:
		Images() = default;
		~Images();
		Images(const Images&) = delete;
		Images& operator=(const Images&) = delete;

		/* path relative to assets/workshop/; opaque: drop the alpha channel */
		const lv_draw_buf_t* get(const std::string& relPath, bool opaque = false);
		void release(const std::string& relPath);
		void clear();

	  private:
		std::map<std::string, lv_draw_buf_t*> m_bufs;
	};

	/* Fonts by size / weight; freed with the store */
	class Fonts
	{
	  public:
		const lv_font_t* get(uint32_t size, bool bold = false, bool mono = false);
		/* handwriting (PlaypenSans), for the paper pages */
		const lv_font_t* hand(uint32_t size, bool bold = false);

	  private:
		const lv_font_t* face(const char* typeface, uint32_t key, uint32_t size, bool bold);
		std::map<uint32_t, FontManager::Font> m_fonts;
	};

	enum class TileLook : uint8_t
	{
		Normal,
		Selected,
		Stop,
		Danger,
		Disabled,
		Input,
		PopupCard,
		PopupChoice,
		PopupChoiceSelected,
	};

	/* Plain container, no scroll, not clickable */
	lv_obj_t* box(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h);
	/* Rounded tile; clickable tiles darken while pressed */
	lv_obj_t* tile(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h, TileLook look, bool clickable);
	void setTileLook(lv_obj_t* tile, TileLook look);
	lv_obj_t* label(lv_obj_t* parent, const lv_font_t* font, uint32_t colour, const char* text);
	/* Label centred in its parent, with an offset */
	lv_obj_t* centredLabel(lv_obj_t* parent, const lv_font_t* font, uint32_t colour, const char* text, int32_t dy = 0);
	/* Invisible tap zone; darkens while pressed when 'feedback' */
	lv_obj_t* tapZone(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, bool feedback);
	void setHidden(lv_obj_t* obj, bool hidden);
	void setEnabled(lv_obj_t* obj, bool enabled);
	/* Thin-line icons (drawn with lv_line, points kept in 'store') */
	lv_obj_t* chevron(lv_obj_t* parent, int32_t cx, int32_t cy, bool up, uint32_t colour,
					  std::vector<std::unique_ptr<lv_point_precise_t[]>>& store);
	void houseIcon(lv_obj_t* parent, int32_t cx, int32_t cy, int32_t s, uint32_t colour,
				   std::vector<std::unique_ptr<lv_point_precise_t[]>>& store);
	void keyboardIcon(lv_obj_t* parent, int32_t cx, int32_t cy, int32_t s, uint32_t colour);
} // namespace UI::Funky
