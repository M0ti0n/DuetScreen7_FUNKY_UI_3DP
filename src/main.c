#include "lvgl.h"
#include "platform/platform.h"
#include "model/printer.h"
#include "ui/ui.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char **argv)
{
    lv_init();
    const char *assets = getenv("FUNKY_ASSETS");
    if (assets && *assets) ui_set_asset_root(assets);
    srand((unsigned)time(NULL));

    platform_init(argc, argv);
    printer_init();
    for (int i = 1; i < argc; i++)
        if (!strcmp(argv[i], "--demo")) printer_fake_demo(true);

    home_create();
    for (int i = 1; i < argc; i++)
        if (!strcmp(argv[i], "--zones")) home_show_tap_zones(true);
    return platform_run();
}
