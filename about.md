# <cp>FPS OPTIMIZER PRO</c> <cy>2.0 — PERFORMANCE SUITE</c>

<cg>**MEASURE MORE. REMOVE VISUAL LOAD. TAKE CONTROL.**</c>

Welcome to <cj>FPS Optimizer Pro 2.0</c>, a performance-focused toolkit for Geometry Dash. This major update expands the live performance overlay and adds a configurable low-graphics mode to reduce some visual work during gameplay.

> <cy>IMPORTANT:</c> This mod can reduce certain visual effects, but it cannot guarantee higher FPS on every device or level. Results depend on hardware, thermal conditions, game settings, and the level itself.

---

## <cy>▰ LIVE FPS + FRAME TIME</c>

The counter lives in the upper-left corner during gameplay and measures frame rate over elapsed time.

- <cg>GREEN</c> — 50 FPS or higher
- <cy>YELLOW</c> — 30–49 FPS
- <cr>RED</c> — below 30 FPS

Turn on **Show Frame Time (ms)** to display the estimated time per frame. At 60 FPS, one frame takes about 16.7 ms; at 30 FPS, about 33.3 ms.

## <cp>▰ LOW GRAPHICS PRESET</c>

One switch to hide the background, ground layers, and particle visuals. Use this when you want the simplest visual scene for performance testing.

## <cj>▰ INDIVIDUAL OPTIMIZATION OPTIONS</c>

### <cg>1. Reduce Particles</c>
Hides particle-system visuals, such as some bursts and decorative effects.

### <cg>2. Hide Background</c>
Hides the level background while keeping gameplay objects visible.

### <cg>3. Hide Ground</c>
Hides the main ground layer.

### <cg>4. Hide Second Ground Layer</c>
Hides the secondary ground layer, which some levels use for extra ground decoration.

### <cg>5. Low Graphics Preset</c>
Combines background, ground, secondary ground, and particle reduction.

### <cg>6. Reduce Particles in Practice Mode</c>
Controls whether particle reduction applies while playing in Practice Mode.

### <cg>7. Show FPS Counter</c>
Toggles the live counter on or off.

### <cg>8. Show Frame Time</c>
Adds the estimated milliseconds per frame beside the FPS number.

### <cg>9. Adaptive FPS Colors</c>
Colors the counter green, yellow, or red according to measured performance.

### <cg>10. Performance Status</c>
Optionally displays GOOD, FAIR, or LOW next to the counter.

### <cg>11. Counter Size</c>
Adjusts the counter scale for different screens and readability preferences.

### <cg>12. Counter Update Interval</c>
Controls how often the number refreshes. Faster refreshes react sooner; slower refreshes can look steadier.

### <cg>13. Green FPS Threshold</c>
Choose the FPS value where the counter changes to green.

### <cg>14. Yellow FPS Threshold</c>
Choose the lower threshold where the counter changes from red to yellow.

---

## <cr>▰ ABOUT ANTI-ALIASING</c>

Anti-aliasing smooths jagged edges, but it is not a universal FPS boost. Higher anti-aliasing quality usually requires more GPU work, and changing it at runtime depends on how the game renderer and device were initialized.

**FPS Optimizer Pro does not claim to toggle engine-level anti-aliasing in this version.** A normal Geode mod cannot reliably turn on MSAA after the graphics context has already been created on every Android device. If the device or game build exposes a supported graphics option, use that option; otherwise an AA toggle in this mod would be misleading.

## <cy>▰ HOW TO TEST</c>

1. Play the same section with all optimization options off.
2. Watch FPS and frame time for several seconds.
3. Enable Low Graphics Preset and replay the same section.
4. Compare the readings under similar conditions.
5. If the level looks wrong, disable individual options to identify which layer or effect changed.

## <cr>▰ COMPATIBILITY & SAFETY</c>

- This update targets Geometry Dash 2.2 on Android through Geode.
- Hiding background or ground layers changes the intended appearance of levels.
- Particle reduction may hide visual effects that creators intended players to see.
- FPS measurements can fluctuate; do not judge performance from a single frame.
- This project must be compiled and tested against the target Geode SDK before release.

## <cg>FPS OPTIMIZER PRO 2.0</c>

<cp>MEASURE</c> <cy>•</c> <cg>OPTIMIZE</c> <cy>•</c> <cj>PLAY</c>

Thanks for testing the update and helping make the mod better.
