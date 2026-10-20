/*
 * Workshop.cpp
 */

#include "Workshop.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "UI/Core/Navigation.h"
#include "UI/Screens/Home/HomeView.h"
#include "UI/Styles/Styles.h"
#include "UI/Widgets/Laptop/FunkyUi.h"
#include "lvgl/src/draw/lv_image_decoder_private.h"
#include <algorithm>
#include <cstdlib>
#include <fmt/format.h>

namespace UI
{
	using namespace WorkshopLayout;

	static constexpr const char* WORKSHOP_FOLDER = ASSETS_FOLDER "workshop/";

	/* The printer's little screen: tint and text colours per mode */
	namespace
	{
		struct ScreenLook
		{
			uint32_t fill, title, value;
		};
		constexpr ScreenLook LOOKS[] = {
			{0x0C1015, 0x55606E, 0x55606E}, /* Offline */
			{0x1D3A5E, 0x7FD0F2, 0xE8F1FB}, /* Idle */
			{0x70500F, 0xFFD27A, 0xFFF3DA}, /* Busy */
			{0x1D5C36, 0x9BF0B4, 0xEFFFF3}, /* Printing */
			{0x761F18, 0xFFD5CC, 0xFFD5CC}, /* Error */
		};
		const PrinterDef& printerDef(PrinterStyle::Style st)
		{
			return PRINTERS[static_cast<size_t>(st)];
		}
	} // namespace

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
		for (auto& [path, buf] : m_images)
		{
			lv_draw_buf_destroy(buf);
		}
	}

	/* Decode a PNG once and keep it in RAM, so redraws never decode again. */
	lv_draw_buf_t* Workshop::load(const std::string& relPath)
	{
		ZoneScoped;
		for (const auto& [p, buf] : m_images)
		{
			if (p == relPath)
			{
				return buf;
			}
		}
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
			m_images.emplace_back(relPath, copy);
		}
		return copy;
	}

	lv_obj_t* Workshop::layer(const LayerDef& def)
	{
		lv_obj_t* img = lv_image_create(getRootPtr());
		lv_obj_remove_flag(img, LV_OBJ_FLAG_CLICKABLE);
		setLayer(img, def);
		return img;
	}

	void Workshop::setLayer(lv_obj_t* obj, const LayerDef& def)
	{
		lv_image_set_src(obj, load(def.file));
		lv_obj_set_pos(obj, def.x, def.y);
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
		lv_image_set_src(m_background, load(BASE));
		lv_obj_set_pos(m_background, 0, 0);
		lv_obj_remove_flag(m_background, LV_OBJ_FLAG_CLICKABLE);

		/* The printer and the beds (swapped by refreshStyle) */
		m_wallBed = layer(WALL_BED_SQUARE);
		m_printer = layer(PRINTERS[0].printer);

		/* The print on the printer's bed */
		auto clip = [&](lv_obj_t*& child)
		{
			lv_obj_t* c = lv_obj_create(root);
			lv_obj_remove_style_all(c);
			lv_obj_remove_flag(c, LV_OBJ_FLAG_SCROLLABLE);
			lv_obj_remove_flag(c, LV_OBJ_FLAG_CLICKABLE);
			child = lv_image_create(c);
			lv_obj_add_flag(c, LV_OBJ_FLAG_HIDDEN);
			return c;
		};
		m_ghostClip = clip(m_ghost);
		m_partClip = clip(m_part);
		m_outline = lv_image_create(root);
		lv_obj_remove_flag(m_outline, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(m_outline, LV_OBJ_FLAG_HIDDEN);

		m_spareBed = layer(SPARE_BED_SQUARE);
		m_folder = layer(FOLDER_SQUARE);
		lv_obj_add_flag(m_folder, LV_OBJ_FLAG_HIDDEN);

		/* Finished parts on the spare bed */
		for (size_t i = 0; i < PART_COUNT; i++)
		{
			m_spareParts[i] = layer(SPARE_BED_PARTS[i]);
			lv_obj_add_flag(m_spareParts[i], LV_OBJ_FLAG_HIDDEN);
		}

		buildPrinterDisplay();

		/* Drawers: 7-frame slide-out animation */
		for (size_t i = 0; i < DRAWER_COUNT; i++)
		{
			for (int f = 0; f < DRAWER_FRAMES; f++)
			{
				m_drawerFrames[i][static_cast<size_t>(f)] =
					load(fmt::format("drawers/{:s}_{:d}.png", DRAWERS[i].id, f));
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
			LV_EVENT_CLICKED,
			nullptr);

		refreshStyle();
		update(m_state);
	}

	void Workshop::refreshStyle()
	{
		ZoneScoped;
		if (m_printer == nullptr)
		{
			return;
		}
		const PrinterStyle::Style st = PrinterStyle::Current();
		if (m_styleSet && st == m_style)
		{
			return;
		}
		m_style = st;
		m_styleSet = true;
		const bool round = st == PrinterStyle::Style::Delta;
		const PrinterDef& p = printerDef(st);
		setLayer(m_wallBed, round ? WALL_BED_ROUND : WALL_BED_SQUARE);
		setLayer(m_printer, p.printer);
		setLayer(m_spareBed, round ? SPARE_BED_ROUND : SPARE_BED_SQUARE);
		setLayer(m_folder, round ? FOLDER_ROUND : FOLDER_SQUARE);
		const std::string id(p.id);
		lv_image_set_src(m_ghost, load("printers/" + id + "_ghost.png"));
		lv_image_set_src(m_part, load("printers/" + id + "_part.png"));
		lv_image_set_src(m_outline, load("printers/" + id + "_outline.png"));
		lv_obj_set_pos(m_outline, p.part.x, p.part.y);
		placePrinterDisplay();
		m_stage = -1; /* re-place the print */
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
		m_fontSmall = FontManager::createFont(face, 13, LV_FREETYPE_FONT_STYLE_BOLD);
		m_fontLarge = FontManager::createFont(face, 22, LV_FREETYPE_FONT_STYLE_BOLD);
		auto font = [](const FontManager::Font& f) { return f.get() ? f.get() : LV_FONT_DEFAULT; };

		m_display = lv_obj_create(getRootPtr());
		lv_obj_remove_style_all(m_display);
		lv_obj_set_style_bg_opa(m_display, LV_OPA_COVER, 0);
		lv_obj_set_style_radius(m_display, 2, 0);
		lv_obj_remove_flag(m_display, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_remove_flag(m_display, LV_OBJ_FLAG_CLICKABLE);

		auto label = [&](const FontManager::Font& f)
		{
			lv_obj_t* l = lv_label_create(m_display);
			lv_obj_set_style_text_font(l, font(f), 0);
			lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
			lv_label_set_text(l, "");
			return l;
		};
		m_title = label(m_fontSmall);
		m_value = label(m_fontLarge);

		/* ERROR: a warning triangle over the whole screen */
		m_triangle = lv_line_create(m_display);
		lv_obj_remove_style_all(m_triangle);
		lv_obj_set_style_line_width(m_triangle, 4, 0);
		lv_obj_set_style_line_rounded(m_triangle, true, 0);
		m_bang = label(m_fontLarge);
		lv_label_set_text(m_bang, "!");
		placePrinterDisplay();
	}

	void Workshop::placePrinterDisplay()
	{
		if (m_display == nullptr)
		{
			return;
		}
		const PrinterDef& p = printerDef(m_style);
		const Rect& r = p.display;
		lv_obj_set_pos(m_display, r.x, r.y);
		lv_obj_set_size(m_display, r.w, r.h);
		lv_obj_set_style_transform_pivot_x(m_display, r.w / 2, 0);
		lv_obj_set_style_transform_pivot_y(m_display, r.h / 2, 0);
		lv_obj_set_style_transform_rotation(m_display, p.displayRotation, 0);

		/* two rows, centred */
		const int32_t rowsH = 16 + 26;
		const int32_t top = (r.h - rowsH) / 2;
		lv_obj_set_width(m_title, r.w);
		lv_obj_set_pos(m_title, 0, top);
		lv_obj_set_width(m_value, r.w);
		lv_obj_set_pos(m_value, 0, top + 15);

		/* triangle: nearly the height of the screen */
		const int32_t th = r.h - 8, tw = th * 23 / 20;
		const int32_t cx = r.w / 2, y0 = 4;
		auto pt = [](int32_t x, int32_t y)
		{ return lv_point_precise_t{static_cast<lv_value_precise_t>(x), static_cast<lv_value_precise_t>(y)}; };
		m_trianglePoints = {pt(cx, y0), pt(cx + tw / 2, y0 + th), pt(cx - tw / 2, y0 + th), pt(cx, y0)};
		lv_line_set_points(m_triangle, m_trianglePoints.data(), static_cast<uint32_t>(m_trianglePoints.size()));
		lv_obj_set_width(m_bang, r.w);
		lv_obj_set_pos(m_bang, 0, y0 + th * 36 / 100);
	}

	void Workshop::setPartStage(int stage)
	{
		if (stage == m_stage)
		{
			return;
		}
		m_stage = stage;
		const Rect& r = printerDef(m_style).part;
		if (stage < 0)
		{
			lv_obj_add_flag(m_ghostClip, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(m_partClip, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(m_outline, LV_OBJ_FLAG_HIDDEN);
			return;
		}
		const int32_t h = r.h * stage / PRINT_STAGES; /* printed, from the bed up */
		lv_obj_remove_flag(m_outline, LV_OBJ_FLAG_HIDDEN);
		/* still to print: faint red, above the printed part */
		Funky::setHidden(m_ghostClip, h >= r.h);
		lv_obj_set_pos(m_ghostClip, r.x, r.y);
		lv_obj_set_size(m_ghostClip, r.w, r.h - h);
		lv_obj_set_pos(m_ghost, 0, 0);
		/* printed */
		Funky::setHidden(m_partClip, h <= 0);
		lv_obj_set_pos(m_partClip, r.x, r.y + r.h - h);
		lv_obj_set_size(m_partClip, r.w, h);
		lv_obj_set_pos(m_part, 0, -(r.h - h)); /* keep the part standing on the bed */
	}

	Workshop::Mode Workshop::ModeOf(const State& s)
	{
		if (s.error)
		{
			return Mode::Error;
		}
		if (!s.connected)
		{
			return Mode::Offline;
		}
		if (s.printing)
		{
			return Mode::Printing;
		}
		return s.busy ? Mode::Busy : Mode::Idle;
	}

	void Workshop::showDisplay(const State& s)
	{
		const Mode mode = ModeOf(s);
		const ScreenLook& look = LOOKS[static_cast<size_t>(mode)];
		lv_obj_set_style_bg_color(m_display, lv_color_hex(look.fill), 0);
		lv_obj_set_style_text_color(m_title, lv_color_hex(look.title), 0);
		lv_obj_set_style_text_color(m_value, lv_color_hex(look.value), 0);
		lv_obj_set_style_line_color(m_triangle, lv_color_hex(look.value), 0);
		lv_obj_set_style_text_color(m_bang, lv_color_hex(look.value), 0);

		const bool error = mode == Mode::Error;
		Funky::setHidden(m_triangle, !error);
		Funky::setHidden(m_bang, !error);
		Funky::setHidden(m_title, error);
		Funky::setHidden(m_value, error);
		auto temp = [](bool has, float v) { return has ? fmt::format("{:d}°", static_cast<int>(v + 0.5f)) : "--°"; };
		switch (mode)
		{
		case Mode::Offline:
			lv_label_set_text(m_title, "");
			lv_label_set_text(m_value, "NO LINK");
			break;
		case Mode::Idle:
			lv_label_set_text(m_title, "READY");
			lv_label_set_text(m_value, (temp(s.hasNozzle, s.nozzle) + "  " + temp(s.hasBed, s.bed)).c_str());
			break;
		case Mode::Busy:
			lv_label_set_text(m_title, "BUSY");
			lv_label_set_text(m_value, s.busyText.c_str());
			break;
		case Mode::Printing:
			lv_label_set_text(m_title, s.paused ? "PAUSED" : "PRINTING");
			lv_label_set_text(m_value, fmt::format("{:d}%", static_cast<int>(s.progress * 100.0f)).c_str());
			break;
		case Mode::Error:
			break;
		}
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
		refreshStyle();

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
		/* While printing: the object cancel folder on the spare bed, finished parts on it */
		Funky::setHidden(m_folder, !s.printing);
		for (size_t i = 0; i < PART_COUNT; i++)
		{
			Funky::setHidden(m_spareParts[i], !(s.printing && (m_partMask & (1u << i)) != 0));
		}
		/* The print grows in 10 steps of 10 % */
		const int stage = std::clamp(static_cast<int>(s.progress * static_cast<float>(PRINT_STAGES)), 0, PRINT_STAGES);
		setPartStage(s.printing ? stage : -1);
		showDisplay(s);
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
									   lv_image_set_src(self->m_drawers[i],
														self->m_drawerFrames[i][static_cast<size_t>(frame)]);
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
		refreshStyle();
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
			openScreen(&home.getObjectFolder(), true);
			break;
		case Target::Macros:
			openFiles(0);
			break;
		case Target::Jobs:
			openScreen(&home.getJobsFolder(), true);
			break;
		case Target::Console:
			openScreen(&home.getLaptop(), true);
			break;
		case Target::Tune:
		case Target::PrintStatus:
			showDashboardStatus();
			break;
		}
	}
} // namespace UI
