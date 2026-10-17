#include "printer.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>

static printer_state_t st;
static lv_timer_t *demo_timer;
static uint32_t demo_tick;

static void touch(void) { st.revision++; }

void printer_init(void)
{
    memset(&st, 0, sizeof st);
    st.status = PRINTER_IDLE;
    snprintf(st.tool_name, sizeof st.tool_name, "T0");
    st.nozzle_c = 24; st.bed_c = 23; st.chamber_c = 21;
    touch();
}

const printer_state_t *printer_get(void) { return &st; }

void printer_send_gcode(const char *line)
{
    /* TODO: USB serial link to the board */
    printf("[gcode] %s\n", line);
    if (strncmp(line, "M112", 4) == 0) { st.status = PRINTER_HALTED; touch(); }
}

void printer_fake_set_printing(bool printing, float progress)
{
    if (printing && st.status != PRINTER_PRINTING) {
        st.job_id++;
        snprintf(st.file_name, sizeof st.file_name, "engine_block_v8.gcode");
    }
    st.status = printing ? PRINTER_PRINTING : PRINTER_IDLE;
    st.progress = progress < 0 ? 0 : progress > 1 ? 1 : progress;
    st.nozzle_target_c = printing ? 215 : 0;  st.nozzle_c = printing ? 215 : 24;
    st.bed_target_c    = printing ? 60 : 0;   st.bed_c    = printing ? 60 : 23;
    st.chamber_c = printing ? 38 : 21;
    st.time_left_s = (int)((1.0f - st.progress) * 5.0f * 3600.0f);
    touch();
}

static void demo_cb(lv_timer_t *t)
{
    demo_tick++;
    uint32_t phase = demo_tick % 60;            /* 1 tick = 0.5 s */
    if (phase < 10) printer_fake_set_printing(false, 0);
    else printer_fake_set_printing(true, (phase - 10) / 49.0f);
}

void printer_fake_demo(bool on)
{
    if (on && !demo_timer) demo_timer = lv_timer_create(demo_cb, 500, NULL);
    if (!on && demo_timer) { lv_timer_delete(demo_timer); demo_timer = NULL; }
}
