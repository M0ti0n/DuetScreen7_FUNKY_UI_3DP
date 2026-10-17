/*
 * Workshop.cpp
 */

#include "Workshop.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "UI/Core/Navigation.h"
#include "UI/Screens/Home/HomeView.h"
#include "UI/Styles/Styles.h"
#include "lvgl/src/draw/lv_image_decoder_private.h"
#include <cstdlib>
#include <fmt/format.h>

namespace UI
{
	using namespace WorkshopLayout;

	static constexpr const char* WORKSHOP_FOLDER = ASSETS_FOLDER "workshop/";

	/* Colours of the printer's little screen */
	static lv_color_t textColor() { return lv_color_hex(0xE8F1FB); }
	static lv_color_t dimColor() { return lv_color_hex(0x9FB7D4); }
	static lv_color_t accentColor() { return lv_color_hex(0x7FD0F2); }

	Workshop::Workshop(const std::string& name, LvObj& parent)
		: View(name, parent)
	{
		ZoneScoped;
		lv_obj_t* root = getRootPtr();
		lv_obj_remove_style_all(root);
		lv_obj_add_flag(root, LV_OBJ_FLAG_FLOATING); /* not part of the HomeView grid */
		lv_obj_set_pos(root, 0, 0);
		lv_obj_set_size(root, SCREEN_W, SCREEN_H);
		lv_obj_set_style_bg_color(root, lv_color_black(), 0);
		lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
		lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
	}

	Workshop::~Workshop()
	{
		for (auto* buf : m_images)
		{
			lv_draw_buf_destroy(buf);
		}
	}

	/* Decode a PNG once and keep it in RAM, so redraws never decode again. */
	lv_draw_buf_t* Workshop::load(const std::string& relPath)
	{
		ZoneScoped;
		const std::string path = std::string(WORKSHOP_FOLDER) + relPath;
		lv_image_decoder_dsc_t dsc{};
		if (lv_image_decoder_open(&dsc, path.c_str(), nullptr) != LV_RESULT_OK || dsc.decoded == nullptr)
		{
			LOG_ERROR("Workshop: failed to load {:s}", path);
			return nullptr;
		}
		lv_draw_buf_t* copy = lv_draw_buf_dup(dsc.decoded);
		lv_image_decoder_close(&dsc);
		if (copy != nullptr)
		{
			m_images.push_back(copy);
		}
		return copy;
	}

	lv_obj_t* Workshop::addHotspot(const Rect& r)
	{
		lv_obj_t* obj = lv_obj_create(getRootPtr());
		lv_obj_remove_style_all(obj);
		lv_obj_set_pos(obj, r.x, r.y);
		lv_obj_set_size(obj, r.w, r.h);
		lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
#if defined(DEBUG_BORDERS) && DEBUG_BORDERS
		lv_obj_set_style_border_width(obj, 2, 0);
		lv_obj_set_style_border_color(obj, lv_color_hex(0xFFE14D), 0);
		lv_obj_set_style_border_opa(obj, LV_OPA_70, 0);
#endif
		return obj;
	}

	void Workshop::onInit()
	{
		ZoneScoped;
		lv_obj_t* root = getRootPtr();

		m_background = lv_image_create(root);
		lv_image_set_src(m_background, load("home_idle.png"));
		lv_obj_set_pos(m_background, 0, 0);
		lv_obj_remove_flag(m_background, LV_OBJ_FLAG_CLICKABLE);

		/* Print on the printer's bed: a clipping box that grows upward from the bed */
		m_partClip = lv_obj_create(root);
		lv_obj_remove_style_all(m_partClip);
		lv_obj_set_pos(m_partClip, PRINT_PART.x, PRINT_PART.y);
		lv_obj_set_size(m_partClip, PRINT_PART_W, PRINT_PART_H);
		lv_obj_remove_flag(m_partClip, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_remove_flag(m_partClip, LV_OBJ_FLAG_CLICKABLE);
		m_part = lv_image_create(m_partClip);
		lv_image_set_src(m_part, load(PRINT_PART.file));
		lv_obj_set_pos(m_part, 0, 0);
		lv_obj_add_flag(m_partClip, LV_OBJ_FLAG_HIDDEN);

		/* Finished parts on the spare bed */
		for (size_t i = 0; i < PART_COUNT; i++)
		{
			m_spareParts[i] = lv_image_create(root);
			lv_image_set_src(m_spareParts[i], load(SPARE_BED_PARTS[i].file));
			lv_obj_set_pos(m_spareParts[i], SPARE_BED_PARTS[i].x, SPARE_BED_PARTS[i].y);
			lv_obj_add_flag(m_spareParts[i], LV_OBJ_FLAG_HIDDEN);
		}

		buildPrinterDisplay();

		/* Drawers: 7-frame slide-out animation */
		for (size_t i = 0; i < DRAWER_COUNT; i++)
		{
			for (int f = 0; f < DRAWER_FRAMES; f++)
			{
				m_drawerFrames[i][static_cast<size_t>(f)] = load(fmt::format("drawers/{:s}_{:d}.png", DRAWERS[i].id, f));
			}
			m_drawers[i] = lv_image_create(root);
			lv_image_set_src(m_drawers[i], m_drawerFrames[i][0]);
			lv_obj_set_pos(m_drawers[i], DRAWERS[i].sprite.x, DRAWERS[i].sprite.y);
			lv_obj_remove_flag(m_drawers[i], LV_OBJ_FLAG_CLICKABLE);

			m_drawerTaps[i] = addHotspot(DRAWERS[i].tap);
			lv_obj_add_event_cb(m_drawerTaps[i], tapEvent, LV_EVENT_CLICKED, this);
		}

		/* Scene objects */
		for (size_t i = 0; i < HOTSPOT_COUNT; i++)
		{
			m_spotTaps[i] = addHotspot(HOTSPOTS[i].tap);
			lv_obj_add_event_cb(m_spotTaps[i], tapEvent, LV_EVENT_CLICKED, this);
		}

		/* Fire alarm: emergency stop, same as the side bar's E-stop button */
		lv_obj_t* estop = addHotspot(ESTOP);
		lv_obj_add_event_cb(
			estop,
			[](lv_event_t*)
			{
				LOG_WARN("Workshop: fire alarm pressed, emergency stop");
				Comm::DUET.Estop();
			},
			LV_EVENT_CLICKED, nullptr);

		update(m_state);
	}

	void Workshop::tapEvent(lv_event_t* e)
	{
		auto* self = static_cast<Workshop*>(lv_event_get_user_data(e));
		auto* target = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
		for (size_t i = 0; i < DRAWER_COUNT; i++)
		{
			if (self->m_drawerTaps[i] == target)
			{
				self->openDrawer(i);
				return;
			}
		}
		for (size_t i = 0; i < HOTSPOT_COUNT; i++)
		{
			if (self->m_spotTaps[i] == target)
			{
				self->open(HOTSPOTS[i].target);
				return;
			}
		}
	}

	void Workshop::buildPrinterDisplay()
	{
		ZoneScoped;
		const std::string face = "Montserrat";
		m_fontSmall = FontManager::createFont(face, 11);
		m_fontMedium = FontManager::createFont(face, 14, LV_FREETYPE_FONT_STYLE_BOLD);
		m_fontLarge = FontManager::createFont(face, 20, LV_FREETYPE_FONT_STYLE_BOLD);
		auto font = [](const FontManager::Font& f) { return f.get() ? f.get() : LV_FONT_DEFAULT; };

		auto label = [&](lv_obj_t* parent, const FontManager::Font& f, lv_color_t color, const char* text)
		{
			lv_obj_t* l = lv_label_create(parent);
			lv_obj_set_style_text_font(l, font(f), 0);
			lv_obj_set_style_text_color(l, color, 0);
			lv_label_set_text(l, text);
			return l;
		};
		auto panel = [](lv_obj_t* parent)
		{
			lv_obj_t* p = lv_obj_create(parent);
			lv_obj_remove_style_all(p);
			lv_obj_set_size(p, LV_PCT(100), LV_PCT(100));
			lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);
			lv_obj_remove_flag(p, LV_OBJ_FLAG_CLICKABLE);
			return p;
		};

		m_display = lv_obj_create(getRootPtr());
		lv_obj_remove_style_all(m_display);
		lv_obj_set_pos(m_display, PRINTER_DISPLAY.x, PRINTER_DISPLAY.y);
		lv_obj_set_size(m_display, PRINTER_DISPLAY.w, PRINTER_DISPLAY.h);
		lv_obj_set_style_transform_pivot_x(m_display, PRINTER_DISPLAY.w / 2, 0);
		lv_obj_set_style_transform_pivot_y(m_display, PRINTER_DISPLAY.h / 2, 0);
		lv_obj_set_style_transform_rotation(m_display, PRINTER_DISPLAY_ROTATION, 0);
		lv_obj_remove_flag(m_display, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_remove_flag(m_display, LV_OBJ_FLAG_CLICKABLE);

		/* Idle: READY + nozzle / bed / chamber */
		m_idlePanel = panel(m_display);
		m_idleTitle = label(m_idlePanel, m_fontSmall, accentColor(), "READY");
		lv_obj_set_pos(m_idleTitle, 5, 2);
		lv_obj_t* unit = label(m_idlePanel, m_fontSmall, dimColor(), "°C");
		lv_obj_align(unit, LV_ALIGN_TOP_RIGHT, -5, 2);
		static constexpr const char* names[] = {"NOZ", "BED", "CHMB"};
		for (size_t i = 0; i < 3; i++)
		{
			m_idleValues[i] = label(m_idlePanel, m_fontLarge, textColor(), "--");
			lv_obj_set_pos(m_idleValues[i], 5 + static_cast<int>(i) * 37, 16);
			lv_obj_t* n = label(m_idlePanel, m_fontSmall, dimColor(), names[i]);
			lv_obj_set_pos(n, 5 + static_cast<int>(i) * 37, 38);
		}

		/* Printing: percentage, nozzle temperature, time left, progress bar */
		m_printPanel = panel(m_display);
		m_printTitle = label(m_printPanel, m_fontSmall, dimColor(), "PRINTING");
		lv_obj_set_pos(m_printTitle, 5, 2);
		m_printRight = label(m_printPanel, m_fontSmall, dimColor(), "");
		lv_obj_align(m_printRight, LV_ALIGN_TOP_RIGHT, -5, 2);
		m_percent = label(m_printPanel, m_fontLarge, textColor(), "0%");
		lv_obj_set_pos(m_percent, 5, 16);
		m_nozzle = label(m_printPanel, m_fontMedium, textColor(), "");
		lv_obj_align(m_nozzle, LV_ALIGN_TOP_RIGHT, -5, 16);
		m_timeLeft = label(m_printPanel, m_fontSmall, dimColor(), "");
		lv_obj_align(m_timeLeft, LV_ALIGN_TOP_RIGHT, -5, 32);
		m_bar = lv_bar_create(m_printPanel);
		lv_obj_remove_style_all(m_bar);
		lv_obj_set_size(m_bar, PRINTER_DISPLAY.w - 10, 5);
		lv_obj_set_pos(m_bar, 5, PRINTER_DISPLAY.h - 8);
		lv_bar_set_range(m_bar, 0, 1000);
		lv_obj_set_style_bg_color(m_bar, lv_color_white(), LV_PART_MAIN);
		lv_obj_set_style_bg_opa(m_bar, LV_OPA_20, LV_PART_MAIN);
		lv_obj_set_style_bg_color(m_bar, accentColor(), LV_PART_INDICATOR);
		lv_obj_set_style_bg_opa(m_bar, LV_OPA_COVER, LV_PART_INDICATOR);
		lv_obj_add_flag(m_printPanel, LV_OBJ_FLAG_HIDDEN);
	}

	void Workshop::setPartProgress(float progress)
	{
		const int h = static_cast<int>(static_cast<float>(PRINT_PART_H) * progress + 0.5f);
		if (h <= 0)
		{
			lv_obj_add_flag(m_partClip, LV_OBJ_FLAG_HIDDEN);
			return;
		}
		lv_obj_remove_flag(m_partClip, LV_OBJ_FLAG_HIDDEN);
		lv_obj_set_y(m_partClip, PRINT_PART.y + PRINT_PART_H - h);
		lv_obj_set_height(m_partClip, h);
		lv_obj_set_y(m_part, -(PRINT_PART_H - h)); /* keep the part standing on the bed */
	}

	void Workshop::update(const State& s)
	{
		ZoneScoped;
		UI_LOCK();
		m_state = s;
		if (m_display == nullptr)
		{
			return; /* not built yet; onInit() applies m_state */
		}

		if (s.jobKey != m_lastJobKey && s.printing)
		{
			/* New job: show a random 1-4 finished parts on the spare bed */
			m_lastJobKey = s.jobKey;
			const int n = 1 + std::rand() % static_cast<int>(PART_COUNT);
			m_partMask = 0;
			while (__builtin_popcount(m_partMask) < n)
			{
				const unsigned bit = static_cast<unsigned>(std::rand()) % static_cast<unsigned>(PART_COUNT);
				m_partMask = static_cast<uint8_t>(m_partMask | (1u << bit));
			}
		}
		for (size_t i = 0; i < PART_COUNT; i++)
		{
			const bool show = s.printing && (m_partMask & (1u << i)) != 0;
			if (show)
				lv_obj_remove_flag(m_spareParts[i], LV_OBJ_FLAG_HIDDEN);
			else
				lv_obj_add_flag(m_spareParts[i], LV_OBJ_FLAG_HIDDEN);
		}
		setPartProgress(s.printing ? s.progress : 0.0f);

		if (s.printing)
		{
			lv_obj_add_flag(m_idlePanel, LV_OBJ_FLAG_HIDDEN);
			lv_obj_remove_flag(m_printPanel, LV_OBJ_FLAG_HIDDEN);
			lv_label_set_text(m_printTitle, s.paused ? "PAUSED" : "PRINTING");
			lv_label_set_text(m_printRight, s.toolName.c_str());
			lv_label_set_text(m_percent, fmt::format("{:d}%", static_cast<int>(s.progress * 100.0f + 0.5f)).c_str());
			lv_label_set_text(m_nozzle,
							  s.hasNozzle ? fmt::format("{:d}°C", static_cast<int>(s.nozzle + 0.5f)).c_str() : "");
			const uint32_t minutes = s.timeLeft / 60;
			lv_label_set_text(m_timeLeft, fmt::format("{:d}h {:02d}m", minutes / 60, minutes % 60).c_str());
			lv_bar_set_value(m_bar, static_cast<int32_t>(s.progress * 1000.0f), LV_ANIM_OFF);
		}
		else
		{
			lv_obj_remove_flag(m_idlePanel, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(m_printPanel, LV_OBJ_FLAG_HIDDEN);
			lv_label_set_text(m_idleTitle, s.connected ? "READY" : "OFFLINE");
			auto temp = [](bool has, float v) { return has ? fmt::format("{:d}", static_cast<int>(v + 0.5f)) : "--"; };
			lv_label_set_text(m_idleValues[0], temp(s.hasNozzle, s.nozzle).c_str());
			lv_label_set_text(m_idleValues[1], temp(s.hasBed, s.bed).c_str());
			lv_label_set_text(m_idleValues[2], temp(s.hasChamber, s.chamber).c_str());
		}
	}

	/* ---------------- drawers & navigation ---------------- */

	void Workshop::animateDrawer(size_t index, bool opening)
	{
		lv_anim_t a;
		lv_anim_init(&a);
		lv_anim_set_var(&a, this);
		lv_anim_set_values(&a, opening ? 0 : DRAWER_FRAMES - 1, opening ? DRAWER_FRAMES - 1 : 0);
		lv_anim_set_duration(&a, DRAWER_ANIM_MS);
		const uintptr_t data = static_cast<uintptr_t>(index) | (opening ? uintptr_t{0x100} : uintptr_t{0});
		lv_anim_set_user_data(&a, reinterpret_cast<void*>(data));
		lv_anim_set_custom_exec_cb(&a,
								   [](lv_anim_t* anim, int32_t frame)
								   {
									   auto* self = static_cast<Workshop*>(anim->var);
									   const size_t i = reinterpret_cast<uintptr_t>(lv_anim_get_user_data(anim)) & 0xFF;
									   lv_image_set_src(self->m_drawers[i], self->m_drawerFrames[i][static_cast<size_t>(frame)]);
								   });
		lv_anim_set_completed_cb(&a,
								 [](lv_anim_t* anim)
								 {
									 auto* self = static_cast<Workshop*>(anim->var);
									 const uintptr_t data = reinterpret_cast<uintptr_t>(lv_anim_get_user_data(anim));
									 const size_t i = data & 0xFF;
									 if (data & 0x100)
									 {
										 self->open(DRAWERS[i].target); /* drawer is open: show its page */
									 }
									 self->m_busy = false;
								 });
		m_busy = true;
		lv_anim_start(&a);
	}

	void Workshop::openDrawer(size_t index)
	{
		ZoneScoped;
		if (m_busy || index >= DRAWER_COUNT)
		{
			return;
		}
		m_openDrawer = static_cast<int>(index);
		animateDrawer(index, true);
	}

	void Workshop::onShow()
	{
		ZoneScoped;
		/* Back on the home scene: slide the drawer that was used shut again */
		if (m_openDrawer >= 0 && m_drawers[static_cast<size_t>(m_openDrawer)] != nullptr)
		{
			const size_t i = static_cast<size_t>(m_openDrawer);
			m_openDrawer = -1;
			animateDrawer(i, false);
		}
		getPresenter()->refresh();
	}

	void Workshop::open(Target target)
	{
		ZoneScoped;
		LOG_INFO("Workshop: open target {:d}", static_cast<int>(target));
		HomeView& home = HomeView::instance();
		auto openControl = [&](void (ControlView::*tab)())
		{
			(home.getControlView().*tab)();
			openScreen(&home.getControlView(), true);
		};
		auto openFiles = [&](size_t tab)
		{
			home.getFileView().setActiveTab(tab);
			openScreen(&home.getFileView(), true);
		};
		auto showDashboardStatus = [&]()
		{
			/* The classic dashboard opens as a normal screen; Home comes back here */
			home.getDashboard().showStatusTab();
			openScreen(&home.getDashboard(), true);
		};

		switch (target)
		{
		case Target::Tools:
		case Target::Extruder:
		case Target::Filament:
			openControl(&ControlView::showTemperatureView);
			break;
		case Target::Motion:
			openControl(&ControlView::showMoveView);
			break;
		case Target::BedLevelling:
			openControl(&ControlView::showHeightmapView);
			break;
		case Target::ObjectCancel:
			openControl(&ControlView::showObjectCancelView);
			break;
		case Target::Macros:
			openFiles(0);
			break;
		case Target::Jobs:
			openFiles(1);
			break;
		case Target::Console:
			openScreen(&home.getConsoleView(), true);
			break;
		case Target::Tune:
		case Target::PrintStatus:
			showDashboardStatus();
			break;
		}
	}
} // namespace UI
