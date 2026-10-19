/*
 * ObjectFolder.cpp
 */

#include "ObjectFolder.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "UI/Core/Navigation.h"
#include "UI/Screens/Home/HomeView.h"
#include "UI/Widgets/Workshop/WorkshopLayout.h"
#include <algorithm>
#include <cmath>
#include <fmt/format.h>

namespace UI
{
	using namespace ObjectFolderLayout;
	using Funky::box;
	using Funky::col;
	using Funky::label;
	using Funky::setHidden;

	namespace
	{
		constexpr uint32_t INK = 0x161412;
		constexpr uint32_t PENCIL = 0x606064;
		constexpr uint32_t RED = 0xCE3426;
		constexpr uint32_t BLUE = 0x2668C4;
		constexpr uint32_t BLUE_LIGHT = 0xB2D6F6;
		constexpr uint32_t PAPER = 0xF7F4EB;
		constexpr const char* BACKGROUND = "objects/background.png";
	} // namespace

	ObjectFolder::ObjectFolder(const std::string& name, LvObj& parent)
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

	ObjectFolder::~ObjectFolder() = default;

	/* ---------------- building ---------------- */

	void ObjectFolder::onInit()
	{
		ZoneScoped;
		lv_obj_t* root = getRootPtr();
		m_background = lv_image_create(root);
		lv_obj_set_pos(m_background, 0, 0);
		setArt(true);

		/* Bed outline: dashed pencil, sized to the bed */
		for (size_t i = 0; i < m_bedLines.size(); i++)
		{
			m_bedLines[i] = lv_line_create(root);
			lv_obj_remove_style_all(m_bedLines[i]);
			lv_obj_set_pos(m_bedLines[i], 0, 0);
			lv_obj_set_style_line_color(m_bedLines[i], col(PENCIL), 0);
			lv_obj_set_style_line_width(m_bedLines[i], 2, 0);
			lv_obj_set_style_line_dash_width(m_bedLines[i], 8, 0);
			lv_obj_set_style_line_dash_gap(m_bedLines[i], 6, 0);
		}
		m_bedEmpty = label(root, m_fonts.hand(20), PENCIL, "nothing printing");
		lv_obj_align(m_bedEmpty, LV_ALIGN_TOP_LEFT, CANVAS.x + CANVAS.w / 2 - 75, CANVAS.y + CANVAS.h / 2 - 14);

		/* Right page */
		m_job = label(root, m_fonts.get(17, true, true), INK, "");
		lv_obj_set_pos(m_job, JOB_X, JOB_Y - 11);
		lv_obj_set_size(m_job, JOB_W, lv_font_get_line_height(m_fonts.get(17, true, true)));
		lv_label_set_long_mode(m_job, LV_LABEL_LONG_MODE_DOTS);
		m_listEmpty = label(root, m_fonts.hand(22), PENCIL, "no objects yet");
		lv_obj_set_pos(m_listEmpty, EMPTY_X - 75, EMPTY_Y - 16);

		for (size_t r = 0; r < m_rows.size(); r++)
		{
			Row& row = m_rows[r];
			const int32_t y = ROW_Y + static_cast<int32_t>(r) * ROW_STEP;
			row.highlight = box(root, 490, y - 4, 380, ROW_H + 8);
			lv_obj_set_style_bg_color(row.highlight, col(BLUE_LIGHT), 0);
			lv_obj_set_style_bg_opa(row.highlight, 150, 0);
			row.numBox = lv_image_create(root);
			lv_obj_set_pos(row.numBox, NUM_X - 2, y - 2);
			row.num = label(row.numBox, m_fonts.hand(21, true), INK, "");
			lv_obj_align(row.num, LV_ALIGN_CENTER, 0, 0);
			row.nameBox = lv_image_create(root);
			lv_obj_set_pos(row.nameBox, NAME_X - 2, y - 2);
			row.name = label(root, m_fonts.get(19, false, true), INK, "");
			lv_obj_set_pos(row.name, NAME_X + 16, y + ROW_H / 2 - 12);
			lv_obj_set_size(row.name, NAME_W - 30, lv_font_get_line_height(m_fonts.get(19, false, true)));
			lv_label_set_long_mode(row.name, LV_LABEL_LONG_MODE_DOTS);
			row.strikePoints = std::make_unique<lv_point_precise_t[]>(2);
			row.strike = lv_line_create(root);
			lv_obj_remove_style_all(row.strike);
			lv_obj_set_pos(row.strike, 0, 0);
			lv_obj_set_style_line_color(row.strike, col(RED), 0);
			lv_obj_set_style_line_width(row.strike, 3, 0);
			lv_obj_set_style_line_rounded(row.strike, true, 0);
			row.numTap = Funky::tapZone(root, NUM_X - 4, y - 4, NUM_W + 8, ROW_H + 8, 6, false);
			row.nameTap = Funky::tapZone(root, NAME_X - 4, y - 4, NAME_W + 8, ROW_H + 8, 6, false);
			lv_obj_add_event_cb(row.numTap, clicked, LV_EVENT_CLICKED, this);
			lv_obj_add_event_cb(row.nameTap, clicked, LV_EVENT_CLICKED, this);
		}

		/* Pager cards */
		m_pageLabel = label(root, m_fonts.get(22, true, true), INK, "");
		auto shade = [&](const Rect& r)
		{
			lv_obj_t* s = box(root, r.x + 5, r.y + 5, r.w - 10, r.h - 10);
			lv_obj_set_style_bg_color(s, col(PAPER), 0);
			lv_obj_set_style_bg_opa(s, 190, 0);
			return s;
		};
		m_upShade = shade(PAGE_UP);
		m_downShade = shade(PAGE_DOWN);

		auto tap = [&](const Rect& r, bool feedback)
		{
			lv_obj_t* t = Funky::tapZone(root, r.x, r.y, r.w, r.h, 8, feedback);
			lv_obj_add_event_cb(t, clicked, LV_EVENT_CLICKED, this);
			return t;
		};
		m_up = tap(PAGE_UP, true);
		m_down = tap(PAGE_DOWN, true);
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

	void ObjectFolder::setArt(bool loaded)
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

	void ObjectFolder::onShow()
	{
		ZoneScoped;
		moveToFront();
		setArt(true);
		refresh();
	}

	void ObjectFolder::onHide()
	{
		m_popup.close();
		setArt(false);
	}

	/* ---------------- data ---------------- */

	void ObjectFolder::update(const State& state)
	{
		ZoneScoped;
		UI_LOCK();
		m_state = state;
		/* The selection follows the object being printed (as Modern UI) */
		if (m_state.current != m_lastCurrent)
		{
			m_lastCurrent = m_state.current;
			if (m_state.current >= 0 && objectValid(static_cast<size_t>(m_state.current)))
			{
				m_selected = m_state.current;
				m_page = static_cast<size_t>(m_selected) / ROWS;
			}
		}
		if (m_selected >= 0 && !objectValid(static_cast<size_t>(m_selected)))
		{
			m_selected = -1;
		}
		if (m_page >= pageCount())
		{
			m_page = pageCount() - 1;
		}
		refresh();
	}

	bool ObjectFolder::objectValid(size_t index) const
	{
		return index < m_state.objects.size() && m_state.objects[index].present;
	}

	size_t ObjectFolder::pageCount() const
	{
		const size_t n = m_state.objects.size();
		return n == 0 ? 1 : (n + ROWS - 1) / ROWS;
	}

	void ObjectFolder::refresh()
	{
		if (!m_built)
		{
			return;
		}
		refreshList();
		refreshBed();
	}

	void ObjectFolder::refreshList()
	{
		const bool any = !m_state.objects.empty();
		lv_label_set_text(m_job, m_state.job.empty() ? "-" : m_state.job.c_str());
		setHidden(m_listEmpty, any);
		for (size_t r = 0; r < m_rows.size(); r++)
		{
			Row& row = m_rows[r];
			const size_t i = m_page * ROWS + r;
			const bool shown = objectValid(i);
			for (lv_obj_t* o : {row.highlight, row.numBox, row.nameBox, row.name, row.strike, row.numTap, row.nameTap})
			{
				setHidden(o, !shown);
			}
			if (!shown)
			{
				continue;
			}
			const Object& obj = m_state.objects[i];
			const bool sel = static_cast<int>(i) == m_selected;
			setHidden(row.highlight, !sel);
			lv_image_set_src(row.numBox,
							 m_images.get(obj.cancelled ? "objects/num_red.png"
										  : sel		   ? "objects/num_sel.png"
													   : "objects/num_normal.png"));
			lv_image_set_src(row.nameBox, m_images.get(sel ? "objects/name_sel.png" : "objects/name_normal.png"));
			lv_label_set_text(row.num, fmt::format("{:d}", i + 1).c_str());
			lv_obj_set_style_text_color(row.num, col(obj.cancelled ? 0xFFFFFF : sel ? BLUE : INK), 0);
			lv_label_set_text(row.name, obj.name.empty() ? fmt::format("Object {:d}", i + 1).c_str() : obj.name.c_str());

			/* Cancelled: struck through in red pen */
			setHidden(row.strike, !obj.cancelled);
			if (obj.cancelled)
			{
				const int32_t y = ROW_Y + static_cast<int32_t>(r) * ROW_STEP + ROW_H / 2;
				lv_point_t size{};
				lv_text_get_size(&size, lv_label_get_text(row.name), m_fonts.get(19, false, true), 0, 0, LV_COORD_MAX,
								 LV_TEXT_FLAG_NONE);
				const int32_t textW = std::min<int32_t>(size.x, NAME_W - 30);
				row.strikePoints[0] = {static_cast<lv_value_precise_t>(NAME_X + 10), static_cast<lv_value_precise_t>(y + 1)};
				row.strikePoints[1] = {static_cast<lv_value_precise_t>(NAME_X + 22 + textW),
									   static_cast<lv_value_precise_t>(y - 2)};
				lv_line_set_points(row.strike, row.strikePoints.get(), 2);
			}
		}

		const size_t pages = pageCount();
		lv_label_set_text(m_pageLabel, fmt::format("{:d}/{:d}", m_page + 1, pages).c_str());
		lv_obj_update_layout(m_pageLabel);
		lv_obj_set_pos(m_pageLabel,
					   PAGE_LABEL.x + (PAGE_LABEL.w - lv_obj_get_width(m_pageLabel)) / 2,
					   PAGE_LABEL.y + (PAGE_LABEL.h - lv_obj_get_height(m_pageLabel)) / 2);
		const bool up = m_page > 0, down = m_page + 1 < pages;
		setHidden(m_upShade, up);
		setHidden(m_downShade, down);
		Funky::setEnabled(m_up, up);
		Funky::setEnabled(m_down, down);
	}

	void ObjectFolder::ensureMarkers(size_t count)
	{
		lv_obj_t* root = getRootPtr();
		while (m_markers.size() < count)
		{
			Marker m;
			m.box = lv_image_create(root);
			lv_obj_add_flag(m.box, LV_OBJ_FLAG_CLICKABLE);
			lv_obj_add_event_cb(m.box, clicked, LV_EVENT_CLICKED, this);
			m.label = label(m.box, m_fonts.hand(19, true), INK, "");
			lv_obj_align(m.label, LV_ALIGN_CENTER, 0, 0);
			m_markers.push_back(m);
		}
		/* keep the popup, the tap zones and the drawers above the markers */
		for (const Marker& m : m_markers)
		{
			lv_obj_move_to_index(m.box, 1);
		}
	}

	void ObjectFolder::refreshBed()
	{
		const bool any = !m_state.objects.empty() && m_state.hasBed;
		setHidden(m_bedEmpty, !m_state.objects.empty());

		/* The bed, as big as the canvas allows with its own proportions */
		int32_t bx = CANVAS.x, by = CANVAS.y, bw = CANVAS.w, bh = CANVAS.h;
		const float xs = m_state.xMax - m_state.xMin, ys = m_state.yMax - m_state.yMin;
		if (m_state.hasBed && xs > 0 && ys > 0)
		{
			if (xs * static_cast<float>(CANVAS.h) >= ys * static_cast<float>(CANVAS.w))
			{
				bh = static_cast<int32_t>(std::lround(static_cast<float>(CANVAS.w) * ys / xs));
				by = CANVAS.y + (CANVAS.h - bh) / 2;
			}
			else
			{
				bw = static_cast<int32_t>(std::lround(static_cast<float>(CANVAS.h) * xs / ys));
				bx = CANVAS.x + (CANVAS.w - bw) / 2;
			}
		}
		auto pt = [](int32_t x, int32_t y) {
			return lv_point_precise_t{static_cast<lv_value_precise_t>(x), static_cast<lv_value_precise_t>(y)};
		};
		m_bedPoints[0] = {pt(bx, by), pt(bx + bw, by)};
		m_bedPoints[1] = {pt(bx + bw, by), pt(bx + bw, by + bh)};
		m_bedPoints[2] = {pt(bx, by + bh), pt(bx + bw, by + bh)};
		m_bedPoints[3] = {pt(bx, by), pt(bx, by + bh)};
		for (size_t i = 0; i < m_bedLines.size(); i++)
		{
			lv_line_set_points(m_bedLines[i], m_bedPoints[i].data(), 2);
		}

		ensureMarkers(m_state.objects.size());
		std::vector<std::pair<int32_t, int32_t>> placed;
		for (size_t i = 0; i < m_markers.size(); i++)
		{
			Marker& m = m_markers[i];
			const bool shown = any && objectValid(i) && m_state.objects[i].hasPosition;
			setHidden(m.box, !shown);
			if (!shown)
			{
				continue;
			}
			const Object& obj = m_state.objects[i];
			float xr = (obj.x - m_state.xMin) / xs, yr = (obj.y - m_state.yMin) / ys;
			xr = std::clamp(xr, 0.0f, 1.0f);
			yr = std::clamp(yr, 0.0f, 1.0f);
			int32_t cx = bx + static_cast<int32_t>(std::lround(xr * static_cast<float>(bw)));
			int32_t cy = by + bh - static_cast<int32_t>(std::lround(yr * static_cast<float>(bh)));

			/* Markers close together are moved apart (rings around the spot) */
			static constexpr int8_t OFFSETS[][2] = {{0, 0},	 {0, -1}, {1, 0},  {0, 1},	 {-1, 0}, {1, -1}, {1, 1},
													{-1, 1}, {-1, -1}, {0, -2}, {2, 0},	 {0, 2},  {-2, 0}, {2, -2},
													{2, 2},	 {-2, 2},  {-2, -2}};
			const int32_t half = MARKER / 2;
			for (const auto& o : OFFSETS)
			{
				const int32_t x = std::clamp(cx + o[0] * MARKER_GAP, CANVAS.x + half, CANVAS.x + CANVAS.w - half);
				const int32_t y = std::clamp(cy + o[1] * MARKER_GAP, CANVAS.y + half, CANVAS.y + CANVAS.h - half);
				const bool clash = std::any_of(placed.begin(),
											   placed.end(),
											   [&](const auto& p)
											   { return std::abs(p.first - x) < MARKER_GAP && std::abs(p.second - y) < MARKER_GAP; });
				if (!clash)
				{
					cx = x;
					cy = y;
					break;
				}
			}
			placed.emplace_back(cx, cy);
			lv_obj_set_pos(m.box, cx - half, cy - half);
			const bool sel = static_cast<int>(i) == m_selected;
			lv_image_set_src(m.box,
							 m_images.get(obj.cancelled ? "objects/marker_red.png"
										  : sel		   ? "objects/marker_sel.png"
													   : "objects/marker_normal.png"));
			lv_label_set_text(m.label, fmt::format("{:d}", i + 1).c_str());
			lv_obj_set_style_text_color(m.label, col(obj.cancelled ? 0xFFFFFF : sel ? BLUE : INK), 0);
		}
		for (lv_obj_t* l : m_bedLines)
		{
			setHidden(l, !any);
		}
	}

	/* ---------------- touches ---------------- */

	void ObjectFolder::pressName(size_t row)
	{
		const size_t i = m_page * ROWS + row;
		if (objectValid(i))
		{
			m_selected = static_cast<int>(i);
			refresh();
		}
	}

	void ObjectFolder::pressNumber(size_t row)
	{
		const size_t i = m_page * ROWS + row;
		if (!objectValid(i) || m_state.objects[i].cancelled)
		{
			return;
		}
		m_selected = static_cast<int>(i);
		refresh();
		const Object& obj = m_state.objects[i];
		const std::string name = obj.name.empty() ? fmt::format("Object {:d}", i + 1) : obj.name;
		m_popup.confirm("CANCEL OBJECT",
						{"Do you want to cancel printing this object only?", fmt::format("{:d}  {:s}", i + 1, name)},
						[this, i]
						{
							if (auto p = getPresenter(); p)
							{
								p->cancelObject(i);
							}
						});
	}

	void ObjectFolder::pressMarker(size_t index)
	{
		if (!objectValid(index))
		{
			return;
		}
		m_selected = static_cast<int>(index);
		m_page = index / ROWS;
		refresh();
	}

	void ObjectFolder::pressPage(int dir)
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

	void ObjectFolder::pressHome()
	{
		m_popup.close();
		home();
	}

	void ObjectFolder::pressStop()
	{
		LOG_WARN("ObjectFolder: STOP sticker pressed, emergency stop");
		Comm::DUET.Estop();
	}

	void ObjectFolder::pressDrawer(size_t index)
	{
		/* Back to the workshop, where the drawer slides out and opens its page */
		m_popup.close();
		home();
		HomeView::instance().getWorkshop().openDrawer(index);
	}

	void ObjectFolder::clicked(lv_event_t* e)
	{
		auto* self = static_cast<ObjectFolder*>(lv_event_get_user_data(e));
		auto* target = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
		if (target == self->m_estop)
		{
			self->pressStop();
			return;
		}
		if (target == self->m_home)
		{
			self->pressHome();
			return;
		}
		if (target == self->m_up || target == self->m_down)
		{
			self->pressPage(target == self->m_up ? -1 : 1);
			return;
		}
		for (size_t i = 0; i < self->m_drawers.size(); i++)
		{
			if (self->m_drawers[i] == target)
			{
				self->pressDrawer(i);
				return;
			}
		}
		for (size_t r = 0; r < self->m_rows.size(); r++)
		{
			if (self->m_rows[r].numTap == target)
			{
				self->pressNumber(r);
				return;
			}
			if (self->m_rows[r].nameTap == target)
			{
				self->pressName(r);
				return;
			}
		}
		for (size_t i = 0; i < self->m_markers.size(); i++)
		{
			if (self->m_markers[i].box == target)
			{
				self->pressMarker(i);
				return;
			}
		}
	}
} // namespace UI
