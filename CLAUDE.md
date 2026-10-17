# DuetScreen7 Funky UI — notes for Claude

- This repo is Duet3D/DuetScreen (`upstream`, branch `dev`) plus our workshop home
  screen. Read FUNKY.md first. Keep edits to upstream files minimal so merges stay easy;
  put new code under `src/UI/Widgets/Workshop/` (or new folders) where possible.
- Upstream conventions apply: C++26, gcc-15, LVGL 9.5 (`libraries/lvgl`), tabs,
  clang-format, commit messages `<type>(<scope>): <description>`.
- Screen is 1024 × 600. Scene positions and tap zones live in
  `src/UI/Widgets/Workshop/WorkshopLayout.h`.
- `art/00_start_point_clean_desk.png` is the untouched start point: never modify it.
  New things are added as layers on top. Keep the ink-outlined cartoon style.
- Use correct English spelling for on-screen labels.
- The local sandbox cannot build this project (needs gcc-15, SDL2, libusb, ffmpeg...).
  Push to a branch and let the "C/C++ CI" workflow build and run the tests; it uploads
  the `workshop-screenshots` artifact for visual checks.
