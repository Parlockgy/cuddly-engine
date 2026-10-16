# Creator Toolkit

A toolbox concept for Geometry Dash level creators.

### Planned features
- **Object Optimizer:** analyze selected static objects, identify identical candidates, and preview possible merges before applying changes.
- **Trigger Audit:** flag repeated or potentially redundant trigger setups without deleting anything automatically.
- **Palette Sync:** derive coordinated highlight and shadow suggestions from a chosen base color while keeping existing channel IDs intact.
- **Palette Pages:** mod-managed saved color presets with pages, names, and quick-apply workflows; these are presets, not extra native GD channels.

### Safety
The current build is a UI prototype only. It does not modify level data, merge objects, or rewrite color channels. Never run experimental object transformations on the only copy of a level.
