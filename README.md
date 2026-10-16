# LevelTune Importer — Geode Android starter

This is an editor-button prototype, not a finished downloader.

## Build
Upload the folder contents to GitHub and run Actions -> Build LevelTune Importer. Download the Android64 artifact.

## Current status
- Adds a LevelTune button to the editor if the expected editor menu ID exists.
- Shows an information popup.
- Does not yet download/import music.
- Does not extract audio from YouTube watch URLs.

Next implementation work: robust editor-button placement, URL input UI, direct audio downloads with Android-safe networking, format/size validation, and registering/placing the downloaded song using supported Geometry Dash/Geode APIs. Test against the exact Geode SDK and GD version.
