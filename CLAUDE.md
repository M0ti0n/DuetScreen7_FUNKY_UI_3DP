# DuetScreen7 Funky UI — notes for Claude

- Native LVGL 9.2.2 app in C (`lib/lvgl` submodule, config in `config/lv_conf.h`).
- Screen is 1024 × 600. All scene positions and tap zones live in `src/ui/scene_layout.h`.
- `art/00_start_point_clean_desk.png` is the untouched start point: never modify it.
  New things are added as layers on top.
- Keep the illustrated, ink-outlined cartoon style for any new art.
- Use correct English spelling for on-screen labels.
- Verify UI changes with the headless build (`-DFUNKY_PLATFORM=headless`, `--shot`)
  and look at the screenshot before committing.
- Printer data comes from `src/model/printer.h`; it is fake until the USB link to
  RepRapFirmware exists. Map RRF `state.status` to `printer_status_t`.
