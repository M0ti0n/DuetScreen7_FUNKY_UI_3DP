/* Desktop simulator: 1024x600 SDL window, mouse = touch. */
#include "lvgl.h"
#include "platform.h"
#include <time.h>
#include <unistd.h>

static uint32_t tick_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint32_t)(t.tv_sec * 1000 + t.tv_nsec / 1000000);
}

void platform_init(int argc, char **argv)
{
    lv_tick_set_cb(tick_ms);
    lv_display_t *d = lv_sdl_window_create(1024, 600);
    lv_sdl_window_set_title(d, "DuetScreen7 Funky UI (simulator)");
    lv_sdl_mouse_create();
}

int platform_run(void)
{
    for (;;) {
        uint32_t wait = lv_timer_handler();
        usleep((wait > 5 ? 5 : wait) * 1000);
    }
    return 0;
}
