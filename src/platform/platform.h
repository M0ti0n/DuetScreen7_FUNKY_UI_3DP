#pragma once
/* Each platform creates the LVGL display + input and runs the main loop. */
void platform_init(int argc, char **argv);
int  platform_run(void);
