/* No window: renders the UI into memory and writes a screenshot.
 *   funky_ui --shot out.ppm [--printing 42] [--drawer 3] [--spot 2] [--estop] [--zones] [--ms 600]
 * Used for CI checks and quick previews without a display. */
#include "lvgl.h"
#include "platform.h"
#include "model/printer.h"
#include "ui/ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 1024
#define H 600
static uint32_t fb[W * H];
static uint32_t now_ms;
static const char *shot = "home.ppm";
static int ms_after = 600, drawer = -1, spot = -1, printing = -1, estop;

static uint32_t tick_ms(void) { return now_ms; }

static void flush_cb(lv_display_t *d, const lv_area_t *a, uint8_t *px)
{
    int w = lv_area_get_width(a);
    for (int y = a->y1; y <= a->y2; y++)
        memcpy(&fb[y * W + a->x1], px + (size_t)(y - a->y1) * w * 4, (size_t)w * 4);
    lv_display_flush_ready(d);
}

void platform_init(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--shot") && i + 1 < argc) shot = argv[++i];
        else if (!strcmp(argv[i], "--ms") && i + 1 < argc) ms_after = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--drawer") && i + 1 < argc) drawer = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--printing") && i + 1 < argc) printing = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--spot") && i + 1 < argc) spot = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--estop")) estop = 1;
    }
    lv_tick_set_cb(tick_ms);
    lv_display_t *d = lv_display_create(W, H);
    static uint8_t buf[W * H * 4];
    lv_display_set_buffers(d, buf, NULL, sizeof buf, LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_color_format(d, LV_COLOR_FORMAT_XRGB8888);
    lv_display_set_flush_cb(d, flush_cb);
}

static void run_for(int ms)
{
    for (int t = 0; t < ms; t += 5) { now_ms += 5; lv_timer_handler(); }
}

int platform_run(void)
{
    run_for(50);
    if (printing >= 0) printer_fake_set_printing(true, printing / 100.0f);
    run_for(300);
    if (drawer >= 0) home_open_drawer(drawer);
    if (spot >= 0) home_open_hotspot(spot);
    if (estop) estop_confirm_open();
    run_for(ms_after);
    lv_refr_now(NULL);

    FILE *f = fopen(shot, "wb");
    if (!f) { perror(shot); return 1; }
    fprintf(f, "P6\n%d %d\n255\n", W, H);
    for (int i = 0; i < W * H; i++) {
        uint32_t p = fb[i];
        unsigned char rgb[3] = { (p >> 16) & 0xFF, (p >> 8) & 0xFF, p & 0xFF };
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
    printf("wrote %s\n", shot);
    return 0;
}
