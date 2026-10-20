# Art

Everything here is source material. The app loads the 1024 × 600 images in `../assets/workshop/`.

| File | What it is |
|---|---|
| `00_start_point_clean_desk.png` | **The start point.** Clean wall + table + 4 drawers. Never edit it. |
| `01_clean_desk_5_drawers.png` | Start point with the 5th drawer (TUNE) added |
| `home_idle_1336x784.png` | Full idle home scene at master size |
| `home_reference_printing_1336x784.png` | Reference of the printing look |
| `ref_original_concept.png` | Original concept the objects were cut from |
| `ref_laptop_screen_dwc.png` | Image shown on the laptop screen |
| `objects/fire_bell.png` | The fire bell cut from the start point. The scenes draw it 20 % bigger (scaled from the top, 5 px to the left) over the original |
| `objects/` | Each scene object as a transparent layer at master size, positions in `objects.json` |
| `drawers/` | Drawer opening animations, 7 frames stacked vertically |
| `layout.json` | Positions on the 1024 × 600 screen |

Master size is 1336 × 784 (same 128:75 ratio as the screen); runtime assets are
scaled to 1024 × 600.

## State layers (`assets/workshop/layers/`)

| Layer | Shown when |
|---|---|
| `print_part.png` | printing — revealed from the bed upward with progress |
| `bed_boat/spinner/bar/cube.png` | printing — a random 1–4 of them per job, on the spare bed |

The printer's little display is blank in the art; its text is drawn live by the app.

## Laptop page (`laptop/`)

| File | What it is |
|---|---|
| `mockups.py` | Draws the laptop mock-ups (all tabs, keyboard) and holds the drawing code |
| `export_laptop.py` | Writes `assets/workshop/laptop/` (screen up, keyboard open, shifted keys) and `keys.json` (key rectangles, copied into `LaptopLayout.h`) |
| `popup_frame.py`, `export_popup.py` | The popup frame and the X / ✓ / trash buttons in `assets/workshop/popup/`, standard (glass) and `_paper` (cardboard, for the folders) |

Run from anywhere: `python3 art/laptop/export_laptop.py` (needs Pillow, numpy, opencv).

## Object cancel folder (`objects/`)

| File | What it is |
|---|---|
| `mockups.py` | Draws the folder mock-ups and holds the drawing code |
| `export_objects.py` | Writes `assets/workshop/objects/`: the background (desk, folder, papers, stickers, cards, drawers) and the box sprites |

## Jobs folder (`jobs/`)

| File | What it is |
|---|---|
| `mockups.py` | Draws the jobs folder mock-ups (uses the drawing code of `objects/mockups.py`) |
| `export_jobs.py` | Writes `assets/workshop/jobs/`: the background, the taped photo frame, the PRINT stamp, row boxes, sort arrows, folder icon and the "no picture" sketch |

## Printer styles (`printers/`)

Mock-ups of the home scene with a choice of printer: none, CoreXY (the current one), delta
and bed slinger. All printers share the CoreXY's display, so the app can draw the same
screen on each.

| File | What it is |
|---|---|
| `source/delta_reference.png`, `source/bedslinger_reference.png` | The reference pictures |
| `source/*_mask.png` | Cut-out masks for the delta (outline, vase, display glass) |
| `bedslinger.py` | Makes `bedslinger.png`: removes the painted checkerboard, grades it to the scene, redraws the filament tube |
| `round_beds.py` | Makes `wall_bed_round.png` and `bed_round.png` (round beds for the delta) |
| `mockups.py` | Composes `mockups/home_*.png` and `mockups/printers_grid.png` (also holds the drawing code) |
| `folder.py` | The folder lying on the spare bed while printing |
| `bake.py` | Writes the runtime layers to `assets/workshop/` (base, beds, printers, prints, folder, coloured parts) and `layout.json` (positions for `WorkshopLayout.h`) |
| `preview.py` | Composes the baked layers as the app does, to check a bake |
| `source/laptop_console.png` | The console shown on the laptop on the home scene |
