# FPS Optimizer Pro — Geode Android project

A Geode C++ project targeting Geometry Dash 2.2 Android 64-bit.

## Features
- Live FPS counter in the upper-left during gameplay.
- Green/yellow/red status colors based on measured FPS.
- Configurable counter refresh interval.
- Optional particle-visual reduction mode.
- Long decorated `about.md` description.

## Build
1. Upload the contents of this folder to a GitHub repository.
2. Open **Actions** and run **Build FPS Optimizer Pro for Android**.
3. Download the `FPS-Optimizer-Pro-Android64` artifact.
4. Install the generated `.geode` file using Geode.

## Important
This source project has not been compiled against your exact local Geode SDK or tested in-game yet. The particle option hides particle nodes to reduce visual rendering; it cannot guarantee an FPS increase on every level/device. Test on a copy of your setup and send any compiler log if the workflow fails.
