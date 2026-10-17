/* DuetScreen 7: Linux framebuffer + evdev touch.
 * FUNKY_FB    framebuffer device   (default /dev/fb0)
 * FUNKY_TOUCH touch input device   (default /dev/input/event0) */
#include "lvgl.h"
#include "platform.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static uint32_t tick_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint32_t)(t.tv_sec * 1000 + t.tv_nsec / 1000000);
}

static const char *env_or(const char *k, const char *d)
{
    const char *v = getenv(k);
    return (v && *v) ? v : d;
}

void platform_init(int argc, char **argv)
{
    lv_tick_set_cb(tick_ms);
    lv_display_t *d = lv_linux_fbdev_create();
    lv_linux_fbdev_set_file(d, env_or("FUNKY_FB", "/dev/fb0"));
    lv_evdev_create(LV_INDEV_TYPE_POINTER, env_or("FUNKY_TOUCH", "/dev/input/event0"));
}

int platform_run(void)
{
    for (;;) {
        uint32_t wait = lv_timer_handler();
        usleep((wait > 5 ? 5 : wait) * 1000);
    }
    return 0;
}
