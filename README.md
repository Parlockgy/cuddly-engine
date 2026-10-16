# Creator Toolkit (Geode / Android64)

Editor-side prototype for a level-creator toolbox.

## Included in this starter build
- An entry button is injected into `EditorUI` when the `editor-buttons-menu` node exists.
- The popup describes the planned Object Optimizer, Palette Sync, and 50+ palette preset pages.
- Settings allow disabling the editor button and indicate preview/confirmation preferences.
- The code explicitly reports that no level data is modified by this prototype.

## Important status note
This is **not yet a fully functional optimizer**. It does not merge objects, rewrite trigger chains, alter color channels, or create more native Geometry Dash color channels. A safe implementation must inspect the actual selected-object data, preserve object properties and group/channel references, and be tested on copied levels before changing saved data. The 50+ slots are intended as mod-managed palette presets, not a claim that Geometry Dash's native channel limit has been expanded.

## Build
Push the project to GitHub and run **Actions → Build Creator Toolkit**. The workflow targets Android64. Build output has not been compiled or tested in-game in this environment.
