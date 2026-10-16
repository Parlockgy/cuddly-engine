# Creator Toolkit v1.3.0 (Geode, Android 2.2081)

**Status: written, not compiled, not tested in-game.** Build with the GitHub Actions workflow and fix errors from the log. See `docs/API_STATUS.md`.

- **Palette Lab**: 6 harmony modes, 10 presets, hue/sat/brightness/contrast, 6 roles (primary, secondary, background, highlight, light, dark), RGB+HEX, regenerate + history, custom palettes (persisted), HEX swatch editor. Apply: *Paint Channel* sets an existing channel's color; *Assign + Paint* also points selected objects' primary/secondary channel to it. *Undo Apply* restores.
- **Optimizer / Duplicate Cleaner**: analyze selection or level, report, preview (selects dupes), confirm, apply via the editor's own delete, verified undo. Removes only byte-identical duplicates of Solid/Hazard/Decoration objects and Color/Alpha/Toggle triggers. Never fuses objects; makes no FPS claims.
- **Object Inspector**: per-object or aggregate selection report, copyable.
- **Micro Grid / Grid Presets**: set editor grid step (unverified API), persisted, plus Snap Selection with revert.
