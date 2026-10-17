/* Placeholder page shown when a drawer opens. Real pages come next. */
#include "ui.h"

static lv_obj_t *page;
static void (*closed_cb)(void);

static void anim_y(void *o, int32_t v) { lv_obj_set_y(o, v); }

static void gone(lv_anim_t *a)
{
    lv_obj_delete(page);
    page = NULL;
    if (closed_cb) closed_cb();
}

void drawer_page_close(void)
{
    if (!page) return;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, page);
    lv_anim_set_exec_cb(&a, anim_y);
    lv_anim_set_values(&a, 0, 600);
    lv_anim_set_duration(&a, 260);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
    lv_anim_set_completed_cb(&a, gone);
    lv_anim_start(&a);
}

static void close_cb(lv_event_t *e) { drawer_page_close(); }

void drawer_page_open(const char *title, void (*on_closed)(void))
{
    closed_cb = on_closed;
    page = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(page);
    lv_obj_set_size(page, 1024, 600);
    lv_obj_set_pos(page, 0, 600);
    lv_obj_set_style_bg_color(page, lv_color_hex(0x140F0B), 0);
    lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
    lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *bar = lv_obj_create(page);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, 1024, 64);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x2B1E15), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(bar, 2, 0);
    lv_obj_set_style_border_color(bar, lv_color_black(), 0);

    lv_obj_t *tag = lv_label_create(bar);
    lv_label_set_text(tag, title);
    lv_obj_set_style_text_font(tag, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(tag, lv_color_hex(0x1C1A17), 0);
    lv_obj_set_style_bg_color(tag, lv_color_hex(0xEFE6CC), 0);
    lv_obj_set_style_bg_opa(tag, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_hor(tag, 14, 0);
    lv_obj_set_style_pad_ver(tag, 4, 0);
    lv_obj_set_style_border_width(tag, 2, 0);
    lv_obj_set_style_border_color(tag, lv_color_hex(0x555B61), 0);
    lv_obj_align(tag, LV_ALIGN_LEFT_MID, 24, 0);

    lv_obj_t *btn = lv_button_create(bar);
    lv_obj_set_size(btn, 200, 46);
    lv_obj_align(btn, LV_ALIGN_RIGHT_MID, -20, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x3A2A1F), 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x5A4636), 0);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_add_event_cb(btn, close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(btn);
    lv_label_set_text(bl, "CLOSE DRAWER");
    lv_obj_center(bl);

    lv_obj_t *body = lv_label_create(page);
    lv_label_set_text(body, "This page is a placeholder.\nThe real controls come next.");
    lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(body, lv_color_hex(0x9A968C), 0);
    lv_obj_center(body);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, page);
    lv_anim_set_exec_cb(&a, anim_y);
    lv_anim_set_values(&a, 600, 0);
    lv_anim_set_duration(&a, 320);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}
