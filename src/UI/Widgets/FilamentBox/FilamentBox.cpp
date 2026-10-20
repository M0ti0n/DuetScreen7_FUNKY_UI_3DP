/*
 * FilamentBox.cpp
 */

#include "FilamentBox.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "UI/Core/Navigation.h"
#include "UI/Screens/Home/HomeView.h"
#include "UI/Widgets/Workshop/WorkshopLayout.h"
#include <algorithm>
#include <fmt/format.h>

namespace UI
{
	using namespace FilamentLayout;
	using Funky::box;
	using Funky::col;
	using Funky::label;
	using Funky::setHidden;
	namespace FM = FilamentModel;

	namespace
	{
		constexpr uint32_t CYAN = 0x6ED6F0;
		constexpr uint32_t TEXT = 0xE2EEF8;
		constexpr uint32_t ROW_BORDER = 0x466E96;
		constexpr uint32_t ROW_FILL = 0x10223A;
		constexpr uint32_t BUTTON_FILL = 0x0C1E34;
		constexpr uint32_t PINK = 0xFFCEC4;
		constexpr uint32_t RED = 0xFF7864;
		constexpr uint32_t RED_FILL = 0x46100E;
		constexpr uint32_t GREEN = 0x78F096;
		constexpr uint32_t AMBER = 0xFFBE5A;
		constexpr uint32_t IDLE = 0x6080A0;
		constexpr uint32_t UPGRADES = 0x6096BA;
		constexpr size_t EXTRUDER_DRAWER = 2; /* WorkshopLayout::DRAWERS */
	} // namespace

	FilamentBox::FilamentBox(const std::string& name, LvObj& parent)
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
	}

	FilamentBox::~FilamentBox() = default;

	/* ---------------- building ---------------- */

	void FilamentBox::onInit()
	{
		ZoneScoped;
		lv_obj_t* root = getRootPtr();
		m_background = lv_image_create(root);
		lv_obj_set_pos(m_background, 0, 0);
		setArt(true);

		auto sprite = [&](const Sprite& s)
		{
			lv_obj_t* img = lv_image_create(root);
			lv_image_set_src(img, m_images.get(s.file));
			lv_obj_set_pos(img, s.x, s.y);
			lv_obj_remove_flag(img, LV_OBJ_FLAG_CLICKABLE);
			return img;
		};
		auto icon = [&](lv_obj_t* parent, const char* name)
		{
			lv_obj_t* img = lv_image_create(parent);
			lv_image_set_src(img, m_images.get(fmt::format("filament/icon_{:s}.png", name)));
			lv_obj_set_style_image_recolor_opa(img, LV_OPA_COVER, 0);
			lv_obj_remove_flag(img, LV_OBJ_FLAG_CLICKABLE);
			return img;
		};
		auto rowBox = [&](lv_obj_t* parent, const Rect& r)
		{
			lv_obj_t* b = box(parent, r.x, r.y, r.w, r.h);
			lv_obj_set_style_radius(b, 7, 0);
			lv_obj_set_style_border_width(b, 2, 0);
			lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
			lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
			lv_obj_set_style_bg_color(b, lv_color_black(), LV_STATE_PRESSED);
			lv_obj_set_style_bg_opa(b, LV_OPA_60, LV_STATE_PRESSED);
			lv_obj_add_event_cb(b, clicked, LV_EVENT_CLICKED, this);
			return b;
		};

		const lv_font_t* small = m_fonts.get(15, true);
		const lv_font_t* big = m_fonts.get(16, true);
		for (size_t i = 0; i < m_bays.size(); i++)
		{
			Bay& b = m_bays[i];
			const FilamentLayout::Bay& L = BAYS[i];
			b.empty = sprite(L.empty);
			b.filament = sprite(L.filament);
			lv_obj_set_style_image_recolor_opa(b.filament, LV_OPA_COVER, 0);
			b.shade = sprite(L.shade);
			b.spool = sprite(L.spool);
			b.glow = sprite(L.glow);

			b.screen = box(root, L.screen.x, L.screen.y, L.screen.w, L.screen.h);
			b.red = box(b.screen, 0, 0, L.screen.w, L.screen.h);
			lv_obj_set_style_bg_color(b.red, col(RED_FILL), 0);
			lv_obj_set_style_bg_opa(b.red, 215, 0);

			/* top row: thermometer, tool, temperature */
			b.thermo = icon(b.screen, "thermo");
			lv_obj_set_pos(b.thermo, ROW_TOP.x + 12, ROW_TOP.y + (ROW_TOP.h - 29) / 2);
			b.tool = label(b.screen, big, TEXT, "");
			lv_obj_set_pos(b.tool, ROW_TOP.x + 38, ROW_TOP.y + ROW_TOP.h / 2 - 10);
			b.temp = label(b.screen, big, CYAN, "");
			lv_obj_set_width(b.temp, 80);
			lv_obj_set_style_text_align(b.temp, LV_TEXT_ALIGN_RIGHT, 0);
			lv_obj_set_pos(b.temp, ROW_TOP.w - 88, ROW_TOP.y + ROW_TOP.h / 2 - 10);

			/* middle row: activity, material */
			b.materialBox = rowBox(b.screen, ROW_MATERIAL);
			b.activity = icon(b.materialBox, "idle");
			lv_obj_align(b.activity, LV_ALIGN_LEFT_MID, 10, 0);
			b.material = label(b.materialBox, small, TEXT, "");
			lv_obj_set_width(b.material, ROW_MATERIAL.w - 56);
			lv_label_set_long_mode(b.material, LV_LABEL_LONG_MODE_DOTS);
			lv_obj_align(b.material, LV_ALIGN_LEFT_MID, 36, 1);
			b.chevron = icon(b.materialBox, "chevron");
			lv_obj_align(b.chevron, LV_ALIGN_RIGHT_MID, -8, 0);

			/* bottom row: the button */
			b.buttonBox = rowBox(b.screen, ROW_BUTTON);
			b.buttonIcon = icon(b.buttonBox, "change");
			lv_obj_align(b.buttonIcon, LV_ALIGN_LEFT_MID, 12, 0);
			b.buttonText = label(b.buttonBox, big, CYAN, "");
			lv_obj_align(b.buttonText, LV_ALIGN_CENTER, 14, 1);

			/* no tool behind this bay */
			b.upgrades = label(b.screen, m_fonts.get(15, true), UPGRADES, "SPACE FOR\nUPGRADES\n:)");
			lv_obj_set_style_text_align(b.upgrades, LV_TEXT_ALIGN_CENTER, 0);
			lv_obj_set_style_text_line_space(b.upgrades, 4, 0);
			lv_obj_align(b.upgrades, LV_ALIGN_CENTER, 0, 0);
		}

		/* pages */
		m_plate = sprite(PLATE);
		m_plateText = label(root, m_fonts.get(18, true), 0x222428, "");
		auto tap = [&](const Rect& r, bool feedback)
		{
			lv_obj_t* t = Funky::tapZone(root, r.x, r.y, r.w, r.h, 6, feedback);
			lv_obj_add_event_cb(t, clicked, LV_EVENT_CLICKED, this);
			return t;
		};
		m_plateBack = tap({PLATE_FACE.x, PLATE_FACE.y, static_cast<int16_t>(PLATE_FACE.w / 3), PLATE_FACE.h}, false);
		m_plateNext = tap({static_cast<int16_t>(PLATE_FACE.x + PLATE_FACE.w * 2 / 3),
						   PLATE_FACE.y,
						   static_cast<int16_t>(PLATE_FACE.w / 3),
						   PLATE_FACE.h},
						  false);
		m_arrowNext = sprite(ARROW_NEXT);
		m_arrowBack = sprite(ARROW_BACK);
		m_arrowNextTap = tap(ARROW_NEXT_TAP, true);
		m_arrowBackTap = tap(ARROW_BACK_TAP, true);

		m_estop = tap(ESTOP, true);
		m_home = tap(HOME, true);
		for (size_t i = 0; i < m_drawers.size(); i++)
		{
			const auto& d = WorkshopLayout::DRAWERS[i].tap;
			m_drawers[i] = tap({d.x, d.y, d.w, d.h}, true);
		}

		m_popup.attach(root);
		m_built = true;
		refresh();
	}

	void FilamentBox::setArt(bool loaded)
	{
		if (m_background == nullptr)
		{
			return;
		}
		if (loaded)
		{
			lv_image_set_src(m_background, m_images.get(BACKGROUND, true));
		}
		else
		{
			lv_image_set_src(m_background, nullptr);
			m_images.release(BACKGROUND);
		}
	}

	void FilamentBox::onShow()
	{
		ZoneScoped;
		moveToFront();
		setArt(true);
		refresh();
	}

	void FilamentBox::onHide()
	{
		m_popup.close();
		setArt(false);
	}

	/* ---------------- data ---------------- */

	void FilamentBox::update(const State& state)
	{
		ZoneScoped;
		UI_LOCK();
		m_state = state;
		if (m_page >= pageCount())
		{
			m_page = pageCount() - 1;
		}
		refresh();
	}

	size_t FilamentBox::pageCount() const
	{
		return FM::PageCount(m_state.slots.size());
	}

	const FilamentBox::Slot* FilamentBox::slotOf(size_t bay) const
	{
		const size_t i = m_page * FM::SLOTS_PER_PAGE + bay;
		return i < m_state.slots.size() ? &m_state.slots[i] : nullptr;
	}

	std::string FilamentBox::materialOf(const Slot& slot) const
	{
		if (FM::GetMode() == FM::Mode::Macros)
		{
			return slot.material;
		}
		const int m = FM::SlotMaterial(static_cast<size_t>(slot.tool));
		return m == FM::NONE ? std::string() : std::string(FM::MATERIALS[static_cast<size_t>(m)]);
	}

	bool FilamentBox::loaded(const Slot& slot) const
	{
		return !materialOf(slot).empty();
	}

	void FilamentBox::refresh()
	{
		if (!m_built)
		{
			return;
		}
		for (size_t b = 0; b < m_bays.size(); b++)
		{
			refreshBay(b);
		}
		const size_t pages = pageCount();
		const bool paged = pages > 1;
		for (lv_obj_t* o :
			 {m_plate, m_plateText, m_plateBack, m_plateNext, m_arrowNext, m_arrowBack, m_arrowNextTap, m_arrowBackTap})
		{
			setHidden(o, !paged);
		}
		lv_label_set_text(m_plateText, fmt::format("{:d} / {:d}", m_page + 1, pages).c_str());
		lv_obj_update_layout(m_plateText);
		lv_obj_set_pos(m_plateText,
					   PLATE_FACE.x + (PLATE_FACE.w - lv_obj_get_width(m_plateText)) / 2,
					   PLATE_FACE.y + (PLATE_FACE.h - lv_obj_get_height(m_plateText)) / 2 + 1);
	}

	void FilamentBox::refreshBay(size_t bayIndex)
	{
		Bay& b = m_bays[bayIndex];
		const Slot* s = slotOf(bayIndex);
		const bool used = s != nullptr;
		const bool macros = FM::GetMode() == FM::Mode::Macros;
		const bool isLoaded = used && loaded(*s);
		const bool runout = used && s->runout;
		const bool coloured = used && isLoaded && !runout;

		setHidden(b.spool, !used);
		setHidden(b.filament, !coloured);
		setHidden(b.shade, !coloured);
		setHidden(b.empty, !used || coloured);
		setHidden(b.glow, !runout);
		setHidden(b.red, !runout);
		setHidden(b.upgrades, used);
		for (lv_obj_t* o : {b.thermo, b.tool, b.temp, b.materialBox, b.buttonBox})
		{
			setHidden(o, !used);
		}
		if (!used)
		{
			return;
		}
		if (coloured)
		{
			lv_obj_set_style_image_recolor(
				b.filament, col(FM::SpoolColour(FM::SlotColour(static_cast<size_t>(s->tool)))), 0);
		}

		const uint32_t accent = runout ? RED : CYAN;
		const uint32_t text = runout ? PINK : TEXT;
		lv_obj_set_style_image_recolor(b.thermo, col(accent), 0);
		lv_label_set_text(b.tool, fmt::format("T{:d}", s->tool).c_str());
		lv_obj_set_style_text_color(b.tool, col(text), 0);
		lv_label_set_text(b.temp, s->hasTemp ? fmt::format("{:.1f}°C", s->temp).c_str() : "---°C");
		lv_obj_set_style_text_color(b.temp, col(accent), 0);

		/* middle row */
		lv_obj_set_style_border_color(b.materialBox, col(runout ? RED : ROW_BORDER), 0);
		lv_obj_set_style_bg_color(b.materialBox, col(runout ? 0x5A1412 : ROW_FILL), 0);
		setHidden(b.chevron, runout);
		if (runout)
		{
			lv_image_set_src(b.activity, m_images.get("filament/icon_warn.png"));
			lv_obj_set_style_image_recolor(b.activity, col(PINK), 0);
			lv_label_set_text(b.material, "NO FILAMENT");
			lv_obj_set_style_text_color(b.material, col(PINK), 0);
		}
		else
		{
			const char* ic = s->activity == Activity::Extruding	   ? "extrude"
							 : s->activity == Activity::Retracting ? "retract"
																   : "idle";
			const uint32_t ac = s->activity == Activity::Extruding	  ? GREEN
								: s->activity == Activity::Retracting ? AMBER
																	  : IDLE;
			lv_image_set_src(b.activity, m_images.get(fmt::format("filament/icon_{:s}.png", ic)));
			lv_obj_set_style_image_recolor(b.activity, col(ac), 0);
			const std::string m = materialOf(*s);
			lv_label_set_text(b.material, m.empty() ? "---" : m.c_str());
			lv_obj_set_style_text_color(b.material, col(m.empty() ? IDLE : TEXT), 0);
		}
		lv_obj_align(b.activity, LV_ALIGN_LEFT_MID, 10, 0);
		lv_obj_set_style_image_recolor(b.chevron, col(CYAN), 0);

		/* bottom row: EXTRUDE (PANEL), CHANGE / LOAD (MACROS) */
		const char* buttonText = !macros ? "EXTRUDE" : isLoaded ? "CHANGE" : "LOAD";
		const char* buttonIcon = !macros ? "extrude" : isLoaded ? "change" : "load";
		lv_label_set_text(b.buttonText, buttonText);
		lv_image_set_src(b.buttonIcon, m_images.get(fmt::format("filament/icon_{:s}.png", buttonIcon)));
		lv_obj_align(b.buttonIcon, LV_ALIGN_LEFT_MID, 12, 0);
		lv_obj_set_style_border_color(b.buttonBox, col(runout ? RED : CYAN), 0);
		lv_obj_set_style_bg_color(b.buttonBox, col(runout ? 0x3C0E0C : BUTTON_FILL), 0);
		lv_obj_set_style_text_color(b.buttonText, col(runout ? PINK : CYAN), 0);
		lv_obj_set_style_image_recolor(b.buttonIcon, col(runout ? PINK : CYAN), 0);
	}

	/* ---------------- touches ---------------- */

	void FilamentBox::alert(const std::string& text)
	{
		m_popup.confirm("FILAMENT", {text}, nullptr);
	}

	void FilamentBox::send(const std::vector<std::string>& commands)
	{
		FilamentBoxPresenter::Send(commands);
	}

	void FilamentBox::pickColour(int tool,
								 const std::string& material,
								 std::function<void(int)> done,
								 std::function<void()> cancelled)
	{
		Funky::Popup::Picker p;
		p.title = fmt::format("COLOR T{:d}", tool);
		p.info = material;
		p.swatches.assign(FM::PALETTE.begin(), FM::PALETTE.end());
		p.perPage = 8;
		p.columns = 4;
		p.selected = FM::SlotColour(static_cast<size_t>(tool));
		p.onOk = [done](size_t c) { done(static_cast<int>(c)); };
		p.onCancel = std::move(cancelled);
		m_popup.pick(p);
	}

	/* Material popup, then the colour popup (Modern UI FmStartChange / FmFinishFlow) */
	void FilamentBox::startChange(const Slot& slot)
	{
		const bool macros = FM::GetMode() == FM::Mode::Macros;
		const int tool = slot.tool;
		const std::string loadedName = materialOf(slot);
		std::vector<std::string> list;
		if (macros)
		{
			list = FM::MacroMaterials(m_state.folders);
		}
		else
		{
			list.assign(FM::MATERIALS.begin(), FM::MATERIALS.end());
		}

		Funky::Popup::Picker p;
		p.title = fmt::format("FILAMENT T{:d}", tool);
		p.labels = list;
		p.perPage = 6;
		p.columns = 3;
		if (macros && list.empty())
		{
			p.info = "No folders found in 0:/filaments";
		}
		else
		{
			p.info = fmt::format("Loaded: {:s}", loadedName.empty() ? "---" : loadedName);
		}
		const auto it = std::find(list.begin(), list.end(), loadedName);
		p.selected = it == list.end() ? -1 : static_cast<int>(it - list.begin());
		if (!loadedName.empty())
		{
			/* UNLOAD (MACROS) / CLEAR (PANEL): the three-button layout */
			p.extra = macros ? "UNLOAD" : "CLEAR";
			const int current = m_state.currentTool;
			p.onExtra = [this, macros, tool, current]()
			{
				if (macros)
				{
					send(FM::UnloadCommands(tool, current));
				}
				else
				{
					FM::SetSlotMaterial(static_cast<size_t>(tool), FM::NONE);
				}
				refresh();
			};
		}
		p.onOk = [this, list, tool, loadedName, macros](size_t item)
		{
			const std::string material = list[item];
			/* X on the colour keeps the colour; the material choice stands */
			auto finish = [this, material, tool, loadedName, macros](int colour)
			{
				if (colour >= 0)
				{
					FM::SetSlotColour(static_cast<size_t>(tool), colour);
				}
				if (!macros)
				{
					FM::SetSlotMaterial(static_cast<size_t>(tool), FM::FindMaterial(material));
				}
				else if (material != loadedName)
				{
					/* RRF way: select the tool, unload what is in it, load the new one (its macros heat) */
					send(FM::ChangeCommands(tool, m_state.currentTool, !loadedName.empty(), material));
				}
				refresh();
			};
			pickColour(tool, material, finish, [finish]() { finish(-1); });
		};
		m_popup.pick(p);
	}

	void FilamentBox::pressMaterial(size_t bay)
	{
		const Slot* s = bay < m_bays.size() ? slotOf(bay) : nullptr;
		if (s == nullptr)
		{
			return;
		}
		if (FM::GetMode() == FM::Mode::Panel)
		{
			startChange(*s); /* PANEL: material, then colour */
			return;
		}
		/* MACROS: the material is RRF's, only the colour */
		const int tool = s->tool;
		pickColour(
			tool,
			materialOf(*s),
			[this, tool](int colour)
			{
				FM::SetSlotColour(static_cast<size_t>(tool), colour);
				refresh();
			},
			nullptr);
	}

	void FilamentBox::pressButton(size_t bay)
	{
		const Slot* s = bay < m_bays.size() ? slotOf(bay) : nullptr;
		if (s == nullptr)
		{
			return;
		}
		if (FM::GetMode() == FM::Mode::Panel)
		{
			/* PANEL: load / unload by hand on the extruder page */
			if (m_state.printing)
			{
				alert("EXTRUDE is unavailable while the print is running.");
				return;
			}
			pressDrawer(EXTRUDER_DRAWER);
			return;
		}
		if (m_state.printing)
		{
			alert("Filament change is unavailable while the print is running.");
			return;
		}
		if (m_state.paused && s->tool != m_state.currentTool)
		{
			/* a tool change in a paused print would park the active tool and pick up another */
			alert("While the print is paused only the filament of the active tool can be changed.");
			return;
		}
		if (getPresenter() && getPresenter()->isActive())
		{
			getPresenter()->requestFolders(); /* fresh list for the next time */
		}
		startChange(*s);
	}

	void FilamentBox::pressPage(int dir)
	{
		if (dir < 0 && m_page > 0)
		{
			--m_page;
		}
		else if (dir > 0 && m_page + 1 < pageCount())
		{
			++m_page;
		}
		refresh();
	}

	void FilamentBox::pressHome()
	{
		m_popup.close();
		home();
	}

	void FilamentBox::pressStop()
	{
		LOG_WARN("FilamentBox: fire bell pressed, emergency stop");
		Comm::DUET.Estop();
	}

	void FilamentBox::pressDrawer(size_t index)
	{
		/* Back to the workshop, where the drawer slides out and opens its page */
		m_popup.close();
		home();
		HomeView::instance().getWorkshop().openDrawer(index);
	}

	void FilamentBox::clicked(lv_event_t* e)
	{
		auto* self = static_cast<FilamentBox*>(lv_event_get_user_data(e));
		auto* target = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
		if (target == self->m_estop)
		{
			self->pressStop();
		}
		else if (target == self->m_home)
		{
			self->pressHome();
		}
		else if (target == self->m_arrowNextTap || target == self->m_plateNext)
		{
			self->pressPage(1);
		}
		else if (target == self->m_arrowBackTap || target == self->m_plateBack)
		{
			self->pressPage(-1);
		}
		else
		{
			for (size_t i = 0; i < self->m_drawers.size(); i++)
			{
				if (self->m_drawers[i] == target)
				{
					self->pressDrawer(i);
					return;
				}
			}
			for (size_t b = 0; b < self->m_bays.size(); b++)
			{
				if (self->m_bays[b].materialBox == target)
				{
					self->pressMaterial(b);
					return;
				}
				if (self->m_bays[b].buttonBox == target)
				{
					self->pressButton(b);
					return;
				}
			}
		}
	}
} // namespace UI
