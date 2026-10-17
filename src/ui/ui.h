#pragma once
#include "lvgl.h"

/* Build an "A:<assets>/<rel>" path for LVGL's stdio file system. */
const char *ui_asset(const char *rel);
void ui_set_asset_root(const char *root);

void home_create(void);
void home_open_drawer(int index);      /* also used by tests / headless shots */
void home_open_hotspot(int index);
void home_show_tap_zones(bool on);

/* Drawer page: slides up over the scene. on_closed runs when it is gone. */
void drawer_page_open(const char *title, void (*on_closed)(void));
void drawer_page_close(void);

void estop_confirm_open(void);
