#pragma once
#include <stdbool.h>
#include <stdint.h>

/* Printer state as the UI needs it. Later filled from the RepRapFirmware
 * object model (state.status, heat.heaters[], job.*); for now a fake source. */

typedef enum {
    PRINTER_DISCONNECTED = 0,
    PRINTER_IDLE,
    PRINTER_BUSY,
    PRINTER_PRINTING,      /* RRF: processing, simulating, resuming */
    PRINTER_PAUSED,        /* RRF: paused, pausing */
    PRINTER_HALTED,        /* RRF: halted (after M112) */
} printer_status_t;

typedef struct {
    printer_status_t status;
    char  tool_name[8];      /* "T0" */
    float nozzle_c, nozzle_target_c;
    float bed_c, bed_target_c;
    float chamber_c;
    float progress;          /* 0.0 - 1.0 */
    int   time_left_s;
    char  file_name[64];
    uint32_t job_id;         /* changes whenever a new print starts */
    uint32_t revision;       /* bumped on every change */
} printer_state_t;

void printer_init(void);
const printer_state_t *printer_get(void);

/* Send a G-code line to the machine (stub until the USB link exists). */
void printer_send_gcode(const char *line);

/* Simulator helpers */
void printer_fake_set_printing(bool printing, float progress);
void printer_fake_demo(bool on);   /* cycles idle -> printing -> idle */
