/* Home screen: workshop scene + drawers + printer-state layers. */
#include "ui.h"
#include "scene_layout.h"
#include "model/printer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- asset paths ---------- */
static char asset_root[256] = FUNKY_DEFAULT_ASSETS;
void ui_set_asset_root(const char *root) { snprintf(asset_root, sizeof asset_root, "%s", root); }

/* Paths are kept alive for LVGL (image sources are referenced, not copied). */
const char *ui_asset(const char *rel)
{
    size_t n = strlen(asset_root) + strlen(rel) + 4;
    char *p = lv_malloc(n);
    snprintf(p, n, "A:%s/%s", asset_root, rel);
    return p;
}

/* ---------- objects ---------- */
static lv_obj_t *scr;
static lv_obj_t *drawer_img[DRAWER_COUNT];
static const void *drawer_frames[DRAWER_COUNT][DRAWER_FRAMES];
static int open_drawer = -1;
static bool busy;

static lv_obj_t *part_clip, *part_img;
static lv_obj_t *bed_obj[BED_OBJ_COUNT];
static uint8_t bed_mask;                 /* which spare-bed parts this job shows */

static lv_obj_t *pd_box, *pd_idle, *pd_print;
static lv_obj_t *pd_idle_val[3];
static lv_obj_t *pd_title_r, *pd_pct, *pd_temp, *pd_left, *pd_bar;

/* ---------- drawer animation ---------- */
static void frame_exec(void *obj, int32_t v)
{
    int i = (int)(intptr_t)lv_obj_get_user_data(obj);
    lv_image_set_src(obj, drawer_frames[i][v]);
}

static void drawer_closed(void) { busy = false; }

static void anim_drawer(int i, bool opening, lv_anim_completed_cb_t done)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, drawer_img[i]);
    lv_anim_set_exec_cb(&a, frame_exec);
    lv_anim_set_values(&a, opening ? 0 : DRAWER_FRAMES - 1, opening ? DRAWER_FRAMES - 1 : 0);
    lv_anim_set_duration(&a, 260);
    lv_anim_set_completed_cb(&a, done);
    lv_anim_start(&a);
}

static void page_closed_cb(void)
{
    int i = open_drawer;
    open_drawer = -1;
    if (i >= 0) anim_drawer(i, false, (lv_anim_completed_cb_t)drawer_closed);
}

static void drawer_opened(lv_anim_t *a)
{
    int i = (int)(intptr_t)lv_obj_get_user_data(a->var);
    drawer_page_open(DRAWERS[i].label, page_closed_cb);
}

void home_open_drawer(int i)
{
    if (busy || open_drawer >= 0 || i < 0 || i >= DRAWER_COUNT) return;
    busy = true;
    open_drawer = i;
    anim_drawer(i, true, drawer_opened);
}

static void drawer_click_cb(lv_event_t *e)
{
    home_open_drawer((int)(intptr_t)lv_event_get_user_data(e));
}

static lv_obj_t *hotspot(int x, int y, int w, int h, lv_event_cb_t cb, void *ud)
{
    lv_obj_t *b = lv_obj_create(scr);
    lv_obj_remove_style_all(b);
    lv_obj_set_pos(b, x, y);
    lv_obj_set_size(b, w, h);
    lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, ud);
    return b;
}

static void estop_click_cb(lv_event_t *e) { estop_confirm_open(); }

static void page_only_closed(void) { busy = false; }
void home_open_hotspot(int i)
{
    if (busy || open_drawer >= 0 || i < 0 || i >= HOTSPOT_COUNT) return;
    busy = true;
    drawer_page_open(HOTSPOTS[i].title, page_only_closed);
}
static void spot_click_cb(lv_event_t *e) { home_open_hotspot((int)(intptr_t)lv_event_get_user_data(e)); }

/* ---------- printer display ---------- */
static lv_obj_t *mk_label(lv_obj_t *p, const lv_font_t *f, lv_color_t c, const char *txt)
{
    lv_obj_t *l = lv_label_create(p);
    lv_obj_set_style_text_font(l, f, 0);
    lv_obj_set_style_text_color(l, c, 0);
    lv_label_set_text(l, txt);
    return l;
}

static void build_printer_display(void)
{
    const lv_color_t W  = lv_color_hex(0xE8F1FB);
    const lv_color_t DIM = lv_color_hex(0x9FB7D4);
    const lv_color_t ACC = lv_color_hex(0x7FD0F2);

    pd_box = lv_obj_create(scr);
    lv_obj_remove_style_all(pd_box);
    lv_obj_set_pos(pd_box, PDISP_X, PDISP_Y);
    lv_obj_set_size(pd_box, PDISP_W, PDISP_H);
    lv_obj_set_style_transform_pivot_x(pd_box, PDISP_W / 2, 0);
    lv_obj_set_style_transform_pivot_y(pd_box, PDISP_H / 2, 0);
    lv_obj_set_style_transform_rotation(pd_box, PDISP_ROT_DECIDEG, 0);
    lv_obj_remove_flag(pd_box, LV_OBJ_FLAG_CLICKABLE);

    /* idle: READY + three temperatures */
    pd_idle = lv_obj_create(pd_box);
    lv_obj_remove_style_all(pd_idle);
    lv_obj_set_size(pd_idle, LV_PCT(100), LV_PCT(100));
    lv_obj_t *t = mk_label(pd_idle, &lv_font_montserrat_12, ACC, "READY");
    lv_obj_set_pos(t, 4, 1);
    t = mk_label(pd_idle, &lv_font_montserrat_12, DIM, "C");
    lv_obj_align(t, LV_ALIGN_TOP_RIGHT, -4, 1);
    lv_obj_t *deg = lv_obj_create(pd_idle);         /* the ° of °C (font has no degree sign) */
    lv_obj_remove_style_all(deg);
    lv_obj_set_size(deg, 4, 4);
    lv_obj_set_style_radius(deg, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(deg, 1, 0);
    lv_obj_set_style_border_color(deg, DIM, 0);
    lv_obj_align(deg, LV_ALIGN_TOP_RIGHT, -14, 3);
    static const char *names[3] = { "NOZ", "BED", "CHMB" };
    for (int i = 0; i < 3; i++) {
        pd_idle_val[i] = mk_label(pd_idle, &lv_font_montserrat_20, W, "--");
        lv_obj_set_pos(pd_idle_val[i], 6 + i * 35, 16);
        lv_obj_t *n = mk_label(pd_idle, &lv_font_montserrat_12, DIM, names[i]);
        lv_obj_set_pos(n, 6 + i * 35, 38);
    }

    /* printing: % big, nozzle temp, time left, bar */
    pd_print = lv_obj_create(pd_box);
    lv_obj_remove_style_all(pd_print);
    lv_obj_set_size(pd_print, LV_PCT(100), LV_PCT(100));
    t = mk_label(pd_print, &lv_font_montserrat_12, DIM, "PRINTING");
    lv_obj_set_pos(t, 4, 1);
    pd_title_r = mk_label(pd_print, &lv_font_montserrat_12, DIM, "");
    lv_obj_align(pd_title_r, LV_ALIGN_TOP_RIGHT, -4, 1);
    pd_pct = mk_label(pd_print, &lv_font_montserrat_24, W, "0%");
    lv_obj_set_pos(pd_pct, 4, 14);
    pd_temp = mk_label(pd_print, &lv_font_montserrat_14, W, "");
    lv_obj_align(pd_temp, LV_ALIGN_TOP_RIGHT, -4, 16);
    pd_left = mk_label(pd_print, &lv_font_montserrat_12, DIM, "");
    lv_obj_align(pd_left, LV_ALIGN_TOP_RIGHT, -4, 31);
    pd_bar = lv_bar_create(pd_print);
    lv_obj_set_size(pd_bar, PDISP_W - 8, 5);
    lv_obj_set_pos(pd_bar, 4, PDISP_H - 7);
    lv_bar_set_range(pd_bar, 0, 1000);
    lv_obj_set_style_radius(pd_bar, 0, 0);
    lv_obj_set_style_radius(pd_bar, 0, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(pd_bar, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(pd_bar, LV_OPA_20, 0);
    lv_obj_set_style_bg_color(pd_bar, ACC, LV_PART_INDICATOR);
}

/* ---------- state -> scene ---------- */
static void set_part_progress(float f)
{
    int h = (int)(PART_H * f + 0.5f);
    if (h <= 0) { lv_obj_add_flag(part_clip, LV_OBJ_FLAG_HIDDEN); return; }
    lv_obj_remove_flag(part_clip, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_y(part_clip, PART_Y + PART_H - h);
    lv_obj_set_height(part_clip, h);
    lv_obj_set_y(part_img, -(PART_H - h));      /* keep the part anchored to the bed */
}

static void apply_state(const printer_state_t *s)
{
    static uint32_t last_job = 0;
    bool printing = s->status == PRINTER_PRINTING || s->status == PRINTER_PAUSED;

    if (s->job_id != last_job) {                 /* new job: pick 1-4 random parts */
        last_job = s->job_id;
        int n = 1 + rand() % BED_OBJ_COUNT;
        bed_mask = 0;
        while (__builtin_popcount(bed_mask) < n) bed_mask |= 1u << (rand() % BED_OBJ_COUNT);
    }
    for (int i = 0; i < BED_OBJ_COUNT; i++) {
        if (printing && (bed_mask & (1u << i))) lv_obj_remove_flag(bed_obj[i], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(bed_obj[i], LV_OBJ_FLAG_HIDDEN);
    }
    set_part_progress(printing ? s->progress : 0);

    if (printing) {
        lv_obj_add_flag(pd_idle, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(pd_print, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text_fmt(pd_pct, "%d%%", (int)(s->progress * 100 + 0.5f));
        lv_label_set_text_fmt(pd_temp, "%dC", (int)(s->nozzle_c + 0.5f));
        int m = s->time_left_s / 60;
        lv_label_set_text_fmt(pd_left, "%dh %02dm", m / 60, m % 60);
        lv_label_set_text(pd_title_r, s->status == PRINTER_PAUSED ? "PAUSED" : s->tool_name);
        lv_bar_set_value(pd_bar, (int32_t)(s->progress * 1000), LV_ANIM_OFF);
    } else {
        lv_obj_remove_flag(pd_idle, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(pd_print, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text_fmt(pd_idle_val[0], "%d", (int)(s->nozzle_c + 0.5f));
        lv_label_set_text_fmt(pd_idle_val[1], "%d", (int)(s->bed_c + 0.5f));
        lv_label_set_text_fmt(pd_idle_val[2], "%d", (int)(s->chamber_c + 0.5f));
    }
}

static void poll_cb(lv_timer_t *t)
{
    static uint32_t rev = UINT32_MAX;
    const printer_state_t *s = printer_get();
    if (s->revision != rev) { rev = s->revision; apply_state(s); }
}

/* ---------- build ---------- */
void home_create(void)
{
    scr = lv_screen_active();
    lv_obj_remove_style_all(scr);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *bg = lv_image_create(scr);
    lv_image_set_src(bg, ui_asset("home_idle.png"));
    lv_obj_set_pos(bg, 0, 0);

    /* print on the printer's bed */
    part_clip = lv_obj_create(scr);
    lv_obj_remove_style_all(part_clip);
    lv_obj_set_pos(part_clip, PART_X, PART_Y);
    lv_obj_set_size(part_clip, PART_W, PART_H);
    lv_obj_remove_flag(part_clip, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    part_img = lv_image_create(part_clip);
    lv_image_set_src(part_img, ui_asset("layers/print_part.png"));
    lv_obj_set_pos(part_img, 0, 0);

    for (int i = 0; i < BED_OBJ_COUNT; i++) {
        bed_obj[i] = lv_image_create(scr);
        lv_image_set_src(bed_obj[i], ui_asset(BED_OBJS[i].file));
        lv_obj_set_pos(bed_obj[i], BED_OBJS[i].x, BED_OBJS[i].y);
        lv_obj_add_flag(bed_obj[i], LV_OBJ_FLAG_HIDDEN);
    }

    build_printer_display();

    for (int i = 0; i < DRAWER_COUNT; i++) {
        char rel[64];
        for (int f = 0; f < DRAWER_FRAMES; f++) {
            snprintf(rel, sizeof rel, "drawers/%s_%d.png", DRAWERS[i].id, f);
            drawer_frames[i][f] = ui_asset(rel);
        }
        drawer_img[i] = lv_image_create(scr);
        lv_obj_set_user_data(drawer_img[i], (void *)(intptr_t)i);
        lv_image_set_src(drawer_img[i], drawer_frames[i][0]);
        lv_obj_set_pos(drawer_img[i], DRAWERS[i].spr_x, DRAWERS[i].spr_y);
        hotspot(DRAWERS[i].tap_x, DRAWERS[i].tap_y, DRAWERS[i].tap_w, DRAWERS[i].tap_h,
                drawer_click_cb, (void *)(intptr_t)i);
    }

    for (int i = 0; i < HOTSPOT_COUNT; i++)
        hotspot(HOTSPOTS[i].x, HOTSPOTS[i].y, HOTSPOTS[i].w, HOTSPOTS[i].h, spot_click_cb, (void *)(intptr_t)i);
    hotspot(ESTOP_X, ESTOP_Y, ESTOP_W, ESTOP_H, estop_click_cb, NULL);

    apply_state(printer_get());
    lv_timer_create(poll_cb, 200, NULL);
}

/* Debug overlay: outline every tap zone (simulator --zones, headless --zones). */
void home_show_tap_zones(bool on)
{
    if (!on) return;
    lv_obj_t *child;
    uint32_t n = lv_obj_get_child_count(scr);
    for (uint32_t i = 0; i < n; i++) {
        child = lv_obj_get_child(scr, i);
        if (!lv_obj_has_flag(child, LV_OBJ_FLAG_CLICKABLE)) continue;
        lv_obj_set_style_border_width(child, 2, 0);
        lv_obj_set_style_border_color(child, lv_color_hex(0xFFE14D), 0);
        lv_obj_set_style_border_opa(child, LV_OPA_COVER, 0);
    }
}
