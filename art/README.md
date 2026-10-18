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
