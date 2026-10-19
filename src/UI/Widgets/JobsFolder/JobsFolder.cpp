/*
 * JobsFolder.cpp
 */

#include "JobsFolder.h"
#include "Debug.h"
#include "Hardware/Duet.h"
#include "Storage.h"
#include "UI/Core/Navigation.h"
#include "UI/Screens/Home/HomeView.h"
#include "UI/Widgets/Workshop/WorkshopLayout.h"
#include "utils/StorageHelper.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <fmt/format.h>

namespace UI
{
	using namespace JobsFolderLayout;
	using Funky::box;
	using Funky::col;
	using Funky::label;
	using Funky::setHidden;

	namespace
	{
		constexpr uint32_t INK = 0x161412;
		constexpr uint32_t PENCIL = 0x606064;
		constexpr uint32_t PAPER = 0xF7F4EB;
		constexpr uint32_t RED = 0xCE3426;
		constexpr uint32_t TAG = 0xFADE5C;
		constexpr uint32_t PHOTO_BACK = 0x948F86; /* old-photo grey: thumbnails have no background */
		constexpr const char* BACKGROUND = "jobs/background.png";

		constexpr StorageKey<unsigned int> ID_FUNKY_JOBS_SORT = {"funky:jobs_sort", 0u};
		constexpr StorageKey<bool> ID_FUNKY_JOBS_ASCENDING = {"funky:jobs_ascending", false};

		/* Places a label centred on (cx, cy); the size comes from the text, as the page may not be laid out yet */
		void centreOn(lv_obj_t* label, const lv_font_t* font, int32_t cx, int32_t cy)
		{
			lv_point_t size{};
			lv_text_get_size(&size, lv_label_get_text(label), font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
			lv_obj_set_pos(label, cx - size.x / 2, cy - size.y / 2);
		}

		constexpr std::array<const char*, 6> INFO_KEYS = {
			"print time", "filament", "layer height", "height", "slicer", "saved"};
	} // namespace

	JobsFolder::JobsFolder(const std::string& name, LvObj& parent)
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

		const unsigned int sort = StorageHelper::getData(ID_FUNKY_JOBS_SORT);
		m_sort = sort <= static_cast<unsigned int>(JobsList::Sort::Filament) ? static_cast<JobsList::Sort>(sort)
																			 : JobsList::Sort::Newest;
		m_ascending = StorageHelper::getData(ID_FUNKY_JOBS_ASCENDING);
	}

	JobsFolder::~JobsFolder() = default;

	/* ---------------- formatting ---------------- */

	std::string JobsFolder::FormatPrintTime(uint32_t seconds)
	{
		if (seconds == 0)
		{
			return "-";
		}
		const uint32_t minutes = (seconds + 30) / 60;
		if (minutes < 60)
		{
			return fmt::format("{:d}m", std::max<uint32_t>(minutes, 1));
		}
		return fmt::format("{:d}h {:d}m", minutes / 60, minutes % 60);
	}

	std::string JobsFolder::FormatSaved(const std::string& date)
	{
		static constexpr std::array<const char*, 12> MONTHS = {
			"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
		int y = 0, mo = 0, d = 0, h = 0, mi = 0;
		if (std::sscanf(date.c_str(), "%d-%d-%dT%d:%d", &y, &mo, &d, &h, &mi) < 3 || mo < 1 || mo > 12)
		{
			return date.empty() ? "-" : date;
		}
		return fmt::format("{:d} {:s} {:d}, {:02d}:{:02d}", d, MONTHS[static_cast<size_t>(mo - 1)], y, h, mi);
	}

	bool JobsFolder::SamePath(const std::string& a, const std::string& b)
	{
		auto strip = [](const std::string& p) -> std::string_view
		{
			std::string_view v(p);
			if (v.size() >= 2 && v[1] == ':')
			{
				v.remove_prefix(2);
			}
			while (!v.empty() && v.front() == '/')
			{
				v.remove_prefix(1);
			}
			return v;
		};
		return !a.empty() && !b.empty() && strip(a) == strip(b);
	}

	/* ---------------- building ---------------- */

	void JobsFolder::onInit()
	{
		ZoneScoped;
		lv_obj_t* root = getRootPtr();
		m_background = lv_image_create(root);
		lv_obj_set_pos(m_background, 0, 0);
		setArt(true);

		/* Left page: the photo (dark backing, thumbnail, frame with tape over it) */
		m_thumbBack = box(root, THUMB.x, THUMB.y, THUMB.w, THUMB.h);
		lv_obj_set_style_bg_color(m_thumbBack, col(PHOTO_BACK), 0);
		lv_obj_set_style_bg_opa(m_thumbBack, LV_OPA_COVER, 0);
		m_thumb = lv_image_create(root);
		lv_obj_set_pos(m_thumb, THUMB.x, THUMB.y);
		lv_obj_set_size(m_thumb, THUMB.w, THUMB.h);
		lv_image_set_inner_align(m_thumb, LV_IMAGE_ALIGN_CONTAIN);
		m_photo = lv_image_create(root);
		lv_obj_set_pos(m_photo, PHOTO_X, PHOTO_Y);
		lv_image_set_src(m_photo, m_images.get("jobs/photo.png"));

		const lv_font_t* nameFont = m_fonts.get(17, true, true);
		m_fileName = label(root, nameFont, INK, "");
		lv_obj_set_size(m_fileName, NAME_W, lv_font_get_line_height(nameFont));
		lv_obj_set_pos(m_fileName, NAME_CX - NAME_W / 2, NAME_Y - lv_font_get_line_height(nameFont) / 2);
		lv_obj_set_style_text_align(m_fileName, LV_TEXT_ALIGN_CENTER, 0);
		lv_label_set_long_mode(m_fileName, LV_LABEL_LONG_MODE_DOTS);

		m_divider = lv_line_create(root);
		lv_obj_remove_style_all(m_divider);
		lv_obj_set_pos(m_divider, 0, 0);
		lv_obj_set_style_line_color(m_divider, col(PENCIL), 0);
		lv_obj_set_style_line_width(m_divider, 1, 0);
		m_dividerPoints = {lv_point_precise_t{70, 330}, lv_point_precise_t{422, 330}};
		lv_line_set_points(m_divider, m_dividerPoints.data(), 2);

		const lv_font_t* keyFont = m_fonts.hand(15);
		const lv_font_t* valueFont = m_fonts.get(15, true, true);
		for (size_t i = 0; i < INFO_KEYS.size(); i++)
		{
			const int32_t y = INFO_Y + static_cast<int32_t>(i) * INFO_STEP;
			m_infoKeys[i] = label(root, keyFont, PENCIL, INFO_KEYS[i]);
			lv_obj_set_pos(m_infoKeys[i], INFO_KEY_X, y - 2);
			m_infoValues[i] = label(root, valueFont, INK, "");
			lv_obj_set_pos(m_infoValues[i], INFO_VALUE_X, y);
			/* the PRINT stamp sits right of the first four values */
			lv_obj_set_size(m_infoValues[i],
							i < 4 ? STAMP_X - INFO_VALUE_X : 422 - INFO_VALUE_X,
							lv_font_get_line_height(valueFont));
			lv_label_set_long_mode(m_infoValues[i], LV_LABEL_LONG_MODE_DOTS);
		}
		m_stamp = lv_image_create(root);
		lv_obj_set_pos(m_stamp, STAMP_X, STAMP_Y);
		lv_image_set_src(m_stamp, m_images.get("jobs/stamp.png"));

		m_pick = label(root, m_fonts.hand(20), PENCIL, "pick a job from the list");
		centreOn(m_pick, m_fonts.hand(20), PICK_CX, PICK_Y);

		/* Right page: header */
		m_pagesLabel = label(root, m_fonts.hand(16), PENCIL, "");
		m_sortUp = lv_image_create(root);
		lv_obj_set_pos(m_sortUp, SORT_UP.x, SORT_UP.y);
		m_sortDown = lv_image_create(root);
		lv_obj_set_pos(m_sortDown, SORT_DOWN.x, SORT_DOWN.y);

		/* Rows */
		const lv_font_t* fileFont = m_fonts.get(16, false, true);
		const lv_font_t* ageFont = m_fonts.hand(14);
		const lv_font_t* tagFont = m_fonts.hand(13, true);
		for (size_t r = 0; r < m_rows.size(); r++)
		{
			Row& row = m_rows[r];
			const int32_t y = ROW_Y + static_cast<int32_t>(r) * ROW_STEP;
			row.box = lv_image_create(root);
			lv_obj_set_pos(row.box, ROW_X - 6, y - 4);
			row.icon = lv_image_create(root);
			lv_obj_set_pos(row.icon, ROW_X + 10, y + 9);
			lv_image_set_src(row.icon, m_images.get("jobs/folder.png"));
			row.name = label(root, fileFont, INK, "");
			lv_label_set_long_mode(row.name, LV_LABEL_LONG_MODE_DOTS);
			row.age = label(root, ageFont, PENCIL, "");
			lv_obj_set_size(row.age, 90, lv_font_get_line_height(ageFont));
			lv_obj_set_pos(row.age, AGE_RIGHT - 90, y + ROW_H / 2 - lv_font_get_line_height(ageFont) / 2);
			lv_obj_set_style_text_align(row.age, LV_TEXT_ALIGN_RIGHT, 0);
			row.tag = box(root, 0, y + 8, 40, ROW_H - 16);
			lv_obj_set_style_bg_color(row.tag, col(TAG), 0);
			lv_obj_set_style_bg_opa(row.tag, LV_OPA_COVER, 0);
			lv_obj_set_style_border_color(row.tag, col(INK), 0);
			lv_obj_set_style_border_width(row.tag, 2, 0);
			lv_obj_set_style_radius(row.tag, 3, 0);
			row.tagText = label(row.tag, tagFont, INK, "");
			lv_obj_align(row.tagText, LV_ALIGN_CENTER, 0, 0);
			row.printing = label(root, m_fonts.hand(14, true), RED, "");
			lv_obj_set_pos(row.printing, 0, y + ROW_H / 2 - lv_font_get_line_height(m_fonts.hand(14, true)) / 2);
			row.tap = Funky::tapZone(root, ROW_X - 4, y - 2, ROW_W + 8, ROW_H + 4, 4, false);
			lv_obj_add_event_cb(row.tap, clicked, LV_EVENT_CLICKED, this);
		}
		m_listEmpty = label(root, m_fonts.hand(22), PENCIL, "no jobs yet");
		centreOn(m_listEmpty, m_fonts.hand(22), EMPTY_CX, EMPTY_Y);

		/* Side cards */
		m_sortValue = label(root, m_fonts.get(18, true, true), INK, "");
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
		m_sortTap = tap(SORT, true);
		m_down = tap(PAGE_DOWN, true);
		m_sortUpTap = tap(SORT_UP, false);
		m_sortDownTap = tap(SORT_DOWN, false);
		m_stampTap = tap(STAMP_TAP, true);
		m_estop = tap(ESTOP, true);
		m_home = tap(HOME, true);
		for (size_t i = 0; i < m_drawers.size(); i++)
		{
			const auto& d = WorkshopLayout::DRAWERS[i].tap;
			m_drawers[i] = tap({d.x, d.y, d.w, d.h}, true);
		}

		m_popup.setTheme(Funky::PopupTheme::Paper);
		m_popup.attach(root);
		m_built = true;
		refresh();
	}

	void JobsFolder::setArt(bool loaded)
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

	void JobsFolder::onShow()
	{
		ZoneScoped;
		moveToFront();
		setArt(true);
		refresh();
	}

	void JobsFolder::onHide()
	{
		m_popup.close();
		setArt(false);
	}

	/* ---------------- data ---------------- */

	void JobsFolder::setFiles(const std::string& folder, const std::vector<JobsList::Entry>& entries)
	{
		ZoneScoped;
		UI_LOCK();
		if (folder != m_folder)
		{
			m_page = 0;
		}
		m_folder = folder;
		m_entries = entries;
		resort();
		/* The selected file is gone (deleted, other folder): nothing selected */
		if (m_details.valid &&
			std::none_of(m_entries.begin(),
						 m_entries.end(),
						 [&](const JobsList::Entry& e) { return !e.folder && e.path == m_details.path; }))
		{
			m_details = Details{};
		}
		if (m_page >= pageCount())
		{
			m_page = pageCount() - 1;
		}
		refresh();
	}

	void JobsFolder::setDetails(const Details& details)
	{
		UI_LOCK();
		m_details = details;
		if (m_built)
		{
			refreshDetails();
			refreshList();
		}
	}

	void JobsFolder::setPrinting(bool printing, bool paused, const std::string& jobPath)
	{
		UI_LOCK();
		if (printing == m_printing && paused == m_paused && jobPath == m_printingPath)
		{
			return;
		}
		m_printing = printing;
		m_paused = paused;
		m_printingPath = printing ? jobPath : std::string();
		if (m_built)
		{
			refreshList();
		}
	}

	void JobsFolder::resort()
	{
		JobsList::SortEntries(m_entries, m_sort, m_ascending);
	}

	size_t JobsFolder::pageCount() const
	{
		const size_t n = m_entries.size();
		return n == 0 ? 1 : (n + ROWS - 1) / ROWS;
	}

	void JobsFolder::refresh()
	{
		if (!m_built)
		{
			return;
		}
		refreshList();
		refreshDetails();
	}

	void JobsFolder::refreshList()
	{
		const bool fila = m_sort == JobsList::Sort::Filament;
		setHidden(m_listEmpty, !m_entries.empty());
		for (size_t r = 0; r < m_rows.size(); r++)
		{
			Row& row = m_rows[r];
			const size_t i = m_page * ROWS + r;
			const bool shown = i < m_entries.size();
			for (lv_obj_t* o : {row.box, row.icon, row.name, row.age, row.tag, row.printing, row.tap})
			{
				setHidden(o, !shown);
			}
			if (!shown)
			{
				continue;
			}
			const JobsList::Entry& e = m_entries[i];
			const bool sel = !e.folder && m_details.valid && e.path == m_details.path;
			const int32_t y = ROW_Y + static_cast<int32_t>(r) * ROW_STEP;
			lv_image_set_src(row.box, m_images.get(sel ? "jobs/row_sel.png" : "jobs/row.png"));
			setHidden(row.icon, !e.folder);

			const std::string material = e.folder ? std::string() : JobsList::MaterialOf(e.name);
			const bool tag = fila && !material.empty();
			setHidden(row.tag, !tag);
			setHidden(row.age, tag || e.age.empty());
			int32_t right = AGE_RIGHT - 8;
			if (tag)
			{
				lv_label_set_text(row.tagText, material.c_str());
				lv_obj_update_layout(row.tagText);
				const int32_t w = lv_obj_get_width(row.tagText) + 14;
				lv_obj_set_size(row.tag, w, ROW_H - 16);
				lv_obj_set_pos(row.tag, AGE_RIGHT - w, y + 8);
				lv_obj_align(row.tagText, LV_ALIGN_CENTER, 0, 0);
				right = AGE_RIGHT - w - 8;
			}
			else if (!e.age.empty())
			{
				lv_label_set_text(row.age, e.age.c_str());
				lv_point_t size{};
				lv_text_get_size(&size, e.age.c_str(), m_fonts.hand(14), 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
				right = AGE_RIGHT - size.x - 10;
			}

			/* The file being printed: a red pencil note left of the age / tag */
			const bool running = m_printing && !e.folder && SamePath(e.path, m_printingPath);
			setHidden(row.printing, !running);
			if (running)
			{
				const char* note = m_paused ? "paused" : "printing";
				lv_label_set_text(row.printing, note);
				lv_point_t size{};
				lv_text_get_size(&size, note, m_fonts.hand(14, true), 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
				lv_obj_set_x(row.printing, right - size.x);
				right -= size.x + 10;
			}

			const lv_font_t* font = e.folder ? m_fonts.get(17, true, true) : m_fonts.get(16, false, true);
			const int32_t x = e.folder ? FOLDER_NAME_X : NAME_X;
			lv_obj_set_style_text_font(row.name, font, 0);
			lv_obj_set_size(row.name, std::max<int32_t>(right - x, 40), lv_font_get_line_height(font));
			lv_obj_set_pos(row.name, x, y + ROW_H / 2 - lv_font_get_line_height(font) / 2 + 1);
			lv_label_set_text(row.name, e.up ? ".." : e.name.c_str());
		}

		/* Header */
		const size_t pages = pageCount();
		lv_label_set_text(m_pagesLabel, fmt::format("{:d}/{:d} pages", m_page + 1, pages).c_str());
		lv_obj_update_layout(m_pagesLabel);
		lv_obj_set_pos(m_pagesLabel,
					   PAGES_RIGHT - lv_obj_get_width(m_pagesLabel),
					   HEADER.y + HEADER.h / 2 - lv_obj_get_height(m_pagesLabel) / 2);
		lv_image_set_src(m_sortUp, m_images.get(m_ascending ? "jobs/sort_up_on.png" : "jobs/sort_up_off.png"));
		lv_image_set_src(m_sortDown, m_images.get(m_ascending ? "jobs/sort_down_off.png" : "jobs/sort_down_on.png"));

		/* Side cards */
		lv_label_set_text(m_sortValue, JobsList::SortName(m_sort));
		lv_obj_update_layout(m_sortValue);
		lv_obj_set_pos(m_sortValue,
					   SORT.x + (SORT.w - lv_obj_get_width(m_sortValue)) / 2,
					   SORT_VALUE_Y - lv_obj_get_height(m_sortValue) / 2);
		const bool up = m_page > 0, down = m_page + 1 < pages;
		setHidden(m_upShade, up);
		setHidden(m_downShade, down);
		Funky::setEnabled(m_up, up);
		Funky::setEnabled(m_down, down);
	}

	void JobsFolder::refreshDetails()
	{
		const Details& d = m_details;
		setHidden(m_pick, d.valid);
		for (lv_obj_t* o : {m_thumbBack, m_thumb, m_photo, m_fileName, m_divider, m_stamp, m_stampTap})
		{
			setHidden(o, !d.valid);
		}
		for (size_t i = 0; i < INFO_KEYS.size(); i++)
		{
			setHidden(m_infoKeys[i], !d.valid);
			setHidden(m_infoValues[i], !d.valid);
		}
		if (!d.valid)
		{
			lv_image_set_src(m_thumb, nullptr);
			return;
		}

		/* The thumbnail, or a pencil sketch on the page */
		const bool picture = !d.thumbnail.empty();
		setHidden(m_thumbBack, !picture);
		lv_image_set_src(m_thumb,
						 picture ? static_cast<const void*>(d.thumbnail.c_str())
								 : static_cast<const void*>(m_images.get("jobs/no_picture.png")));

		lv_label_set_text(m_fileName, d.name.c_str());
		const std::string material = JobsList::MaterialOf(d.name);
		std::array<std::string, 6> values;
		if (d.infoKnown)
		{
			values[0] = FormatPrintTime(d.printTime);
			const std::string metres = d.filamentMm > 0 ? fmt::format("{:.2f} m", d.filamentMm / 1000.0f) : "";
			values[1] = material.empty() ? (metres.empty() ? "-" : metres)
										 : (metres.empty() ? material : fmt::format("{:s}, {:s}", material, metres));
			values[2] = d.layerHeight > 0 ? fmt::format("{:.2f} mm", d.layerHeight) : "-";
			values[3] = d.height > 0 ? fmt::format("{:.1f} mm", d.height) : "-";
			values[4] = d.slicer.empty() ? "-" : d.slicer;
			values[5] = FormatSaved(d.saved);
		}
		else
		{
			values.fill("...");
			if (!material.empty())
			{
				values[1] = material;
			}
		}
		for (size_t i = 0; i < values.size(); i++)
		{
			lv_label_set_text(m_infoValues[i], values[i].c_str());
		}
	}

	bool JobsFolder::presenterActive()
	{
		auto p = getPresenter();
		return p && p->isActive();
	}

	/* ---------------- touches ---------------- */

	void JobsFolder::pressRow(size_t row)
	{
		const size_t i = m_page * ROWS + row;
		if (i >= m_entries.size())
		{
			return;
		}
		const JobsList::Entry e = m_entries[i];
		if (e.up)
		{
			if (presenterActive())
			{
				getPresenter()->folderUp();
			}
			return;
		}
		if (e.folder)
		{
			if (presenterActive())
			{
				getPresenter()->openFolder(e.path);
			}
			return;
		}
		if (presenterActive())
		{
			getPresenter()->select(e.path); /* fills the details */
			return;
		}
		Details d;
		d.valid = true;
		d.path = e.path;
		d.name = e.name;
		setDetails(d);
	}

	void JobsFolder::pressPrint()
	{
		if (!m_details.valid)
		{
			return;
		}
		const std::string path = m_details.path, name = m_details.name;
		if (m_printing)
		{
			/* No second print; other files can still be deleted, the one printing can't */
			if (SamePath(path, m_printingPath))
			{
				m_popup.confirm("PRINT IN PROGRESS", {"This file is being printed.", name}, nullptr);
			}
			else
			{
				m_popup.confirm("PRINT IN PROGRESS",
								{"A print is running.", "You can still delete this file.", name},
								nullptr,
								[this, path, name] { confirmDelete(path, name); });
			}
			return;
		}
		m_popup.confirm(
			"START PRINT",
			{"Do you want to start this print?", name},
			[this, path]
			{
				if (presenterActive())
				{
					getPresenter()->startPrint(path);
				}
			},
			[this, path, name] { confirmDelete(path, name); });
	}

	void JobsFolder::confirmDelete(const std::string& path, const std::string& name)
	{
		m_popup.confirm("DELETE FILE",
						{"Do you want to delete this file?", name},
						[this, path]
						{
							if (presenterActive())
							{
								getPresenter()->deleteFile(path);
							}
						});
	}

	void JobsFolder::setSort(JobsList::Sort sort, bool ascending)
	{
		const bool changed = sort != m_sort || ascending != m_ascending;
		m_sort = sort;
		m_ascending = ascending;
		if (changed)
		{
			StorageHelper::setData(ID_FUNKY_JOBS_SORT, static_cast<unsigned int>(sort));
			StorageHelper::setData(ID_FUNKY_JOBS_ASCENDING, ascending);
		}
		resort();
		m_page = 0;
		refresh();
	}

	void JobsFolder::pressSort()
	{
		const JobsList::Sort next = JobsList::NextSort(m_sort);
		setSort(next, JobsList::DefaultAscending(next));
	}

	void JobsFolder::pressDirection(bool ascending)
	{
		if (ascending != m_ascending)
		{
			setSort(m_sort, ascending);
		}
	}

	void JobsFolder::pressPage(int dir)
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

	void JobsFolder::pressHome()
	{
		m_popup.close();
		home();
	}

	void JobsFolder::pressStop()
	{
		LOG_WARN("JobsFolder: STOP sticker pressed, emergency stop");
		Comm::DUET.Estop();
	}

	void JobsFolder::pressDrawer(size_t index)
	{
		/* Back to the workshop, where the drawer slides out and opens its page */
		m_popup.close();
		home();
		HomeView::instance().getWorkshop().openDrawer(index);
	}

	void JobsFolder::clicked(lv_event_t* e)
	{
		auto* self = static_cast<JobsFolder*>(lv_event_get_user_data(e));
		auto* target = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
		if (target == self->m_estop)
		{
			self->pressStop();
		}
		else if (target == self->m_home)
		{
			self->pressHome();
		}
		else if (target == self->m_up || target == self->m_down)
		{
			self->pressPage(target == self->m_up ? -1 : 1);
		}
		else if (target == self->m_sortTap)
		{
			self->pressSort();
		}
		else if (target == self->m_sortUpTap || target == self->m_sortDownTap)
		{
			self->pressDirection(target == self->m_sortUpTap);
		}
		else if (target == self->m_stampTap)
		{
			self->pressPrint();
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
			for (size_t r = 0; r < self->m_rows.size(); r++)
			{
				if (self->m_rows[r].tap == target)
				{
					self->pressRow(r);
					return;
				}
			}
		}
	}
} // namespace UI
