#pragma once
/* Positions on the 1024x600 home scene. Source: art/layout.json */

typedef struct { const char *id; const char *label; int tap_x, tap_y, tap_w, tap_h; int spr_x, spr_y, spr_w, spr_h; } drawer_def_t;

#define DRAWER_FRAMES 7
#define DRAWER_COUNT  5
static const drawer_def_t DRAWERS[DRAWER_COUNT] = {
    /*                         tap zone (marked by hand)   drawer sprite        */
    { "tools",    "TOOLS",    100, 509, 167, 86,  93, 512, 180, 88 },
    { "motion",   "MOTION",   274, 507, 155, 90, 258, 512, 180, 88 },
    { "extruder", "EXTRUDER", 434, 504, 158, 93, 423, 512, 180, 88 },
    { "tune",     "TUNE",     595, 505, 164, 91, 587, 512, 181, 88 },
    { "macros",   "MACROS",   761, 505, 167, 89, 752, 512, 181, 88 },
};

/* Fire alarm = emergency stop (M112) */
#define ESTOP_X 0
#define ESTOP_Y 0
#define ESTOP_W 134
#define ESTOP_H 114

/* Other tappable things in the scene (tap zones marked by hand).
 * Page titles are provisional until each page is designed. */
typedef struct { const char *id; const char *title; int x, y, w, h; } hotspot_def_t;
#define HOTSPOT_COUNT 6
static const hotspot_def_t HOTSPOTS[HOTSPOT_COUNT] = {
    { "filament", "FILAMENT",       1, 134, 185, 305 },   /* spool pile        */
    { "bed_mesh", "BED LEVELLING", 250,  13, 208, 181 },  /* wall bed + note   */
    { "console",  "CONSOLE",       218, 216, 275, 267 },  /* laptop            */
    { "status",   "PRINT STATUS",  507,  71, 293, 385 },  /* printer           */
    { "jobs",     "JOBS",          884,  24, 140, 354 },  /* folder rack       */
    { "spare_bed","FINISHED PRINTS",808, 395, 212, 101 }, /* spare bed (table) */
};

/* Print on the printer's bed (revealed bottom-up with progress) */
#define PART_X 576
#define PART_Y 223
#define PART_W 197
#define PART_H 127

/* Printed parts on the spare bed (random subset per job) */
typedef struct { const char *file; int x, y; } layer_def_t;
#define BED_OBJ_COUNT 4
static const layer_def_t BED_OBJS[BED_OBJ_COUNT] = {
    { "layers/bed_boat.png",    869, 418 },
    { "layers/bed_spinner.png", 921, 427 },
    { "layers/bed_bar.png",     941, 437 },
    { "layers/bed_cube.png",    965, 419 },
};

/* Printer's own little display (slightly rotated quad on the scene):
 * TL(593,380) TR(711,383) BR(702,439) BL(585,435) */
#define PDISP_X 589
#define PDISP_Y 382
#define PDISP_W 118
#define PDISP_H 54
#define PDISP_ROT_DECIDEG 15     /* 1.5 degrees clockwise */
