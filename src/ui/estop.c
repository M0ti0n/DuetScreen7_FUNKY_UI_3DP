/* Fire alarm -> confirm -> M112 */
#include "ui.h"
#include "model/printer.h"

static lv_obj_t *mbox;

static void cancel_cb(lv_event_t *e) { lv_msgbox_close(mbox); mbox = NULL; }
static void stop_cb(lv_event_t *e)
{
    printer_send_gcode("M112");
    lv_msgbox_close(mbox);
    mbox = NULL;
}

void estop_confirm_open(void)
{
    if (mbox) return;
    mbox = lv_msgbox_create(NULL);              /* modal, on the top layer */
    lv_obj_set_width(mbox, 460);
    lv_obj_set_style_bg_color(mbox, lv_color_hex(0x1D1F22), 0);
    lv_obj_set_style_border_color(mbox, lv_color_hex(0xD6402E), 0);
    lv_obj_set_style_border_width(mbox, 3, 0);
    lv_obj_t *t = lv_msgbox_add_title(mbox, "EMERGENCY STOP");
    lv_obj_set_style_text_color(t, lv_color_hex(0xD6402E), 0);
    lv_obj_set_style_text_font(t, &lv_font_montserrat_24, 0);
    lv_obj_t *hdr = lv_msgbox_get_header(mbox);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0x1D1F22), 0);
    lv_obj_t *txt = lv_msgbox_add_text(mbox, "Sends M112. The printer halts immediately and has to be reset before it can move again.");
    lv_obj_set_style_text_color(txt, lv_color_hex(0xCFCBC2), 0);
    lv_obj_t *c = lv_msgbox_add_footer_button(mbox, "CANCEL");
    lv_obj_add_event_cb(c, cancel_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(c, lv_color_hex(0x33363A), 0);
    lv_obj_t *s = lv_msgbox_add_footer_button(mbox, "STOP NOW");
    lv_obj_add_event_cb(s, stop_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(s, lv_color_hex(0xD6402E), 0);
}
