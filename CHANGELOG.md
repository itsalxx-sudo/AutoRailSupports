# Changelog

All notable changes to Auto Rail Supports are documented here.

## [1.1.0]

### Added

- **Double Rails Mode**
  - Track Separation sets the distance between the tracks. Negative puts the second track on your left, positive on your right.
  - Per-slot placement - Centered, Both sides mirrored, Both sides same facing, Left side only, Right side only.
  - Pillar auto roll, tilts when the two rails sit at different heights through a inclined curve.
  - Each tracks under-rail segments are tiled along that tracks own length. Pillar spacing still counts on the main rail.
  - Split Under-Rail sides, gives the second track its own adjustments.

- **Rebuilt configuration menu**
  - Support pillars are now a list you add to, default up to 32 (configurable to a max of 100), instead of four fixed slots.
  - Reorder pillars with the arrows on each row.
  - Split Under-Rail sides now shows both rails as two panels, each with its own blueprint.
  - New presets dropdown with more control of each preset.

- **Max Pillar Slots**
  - A world setting under Mod Savegame Settings. Sets how many pillar slots players can configure in this world, from 1 to 100, defaults to 32.

### Changed

- Blueprints now anchor by the middle of their own contents rather than the Blueprint Designers origin.
- Pillar priority is now the order of the list, top wins. Existing configurations are reordered on load to keep placing what they placed before.

[1.1.0]: https://github.com/itsalxx-sudo/AutoRailSupports/releases/tag/v1.1.0

## [1.0.2]

### Added

- Chat confirmations can now be turned off, under Main Menu -> Mods -> Auto Rail Supports.

### Fixed

- Supports built inside a Blueprint Designer are now part of that blueprint.
- Fixed save compatibility with the Satisfactory Calculator interactive map.

[1.0.2]: https://github.com/itsalxx-sudo/AutoRailSupports/releases/tag/v1.0.2

## [1.0.1]
- Fixed game crash on 1.2.4 dedicated servers running CL#491125 ARS build - Rebuilt for 1.2.4.

[1.0.1]: https://github.com/itsalxx-sudo/AutoRailSupports/releases/tag/v1.0.1

## [1.0.0]

Initial release.

### Features

- Automatically stamps saved in-game blueprints beneath train rails as you build them.
- One configurable under-rail segment that follows the rails inclines and curves.
- Up to four support pillar blueprints placed at configurable intervals along the rail.
- In-world ARS Adjustments (`K` while holding build/dismantle gun) for adjusting supports 
  without opening a menu or losing the hologram.
- Configuration menu (`U`) with all the mods possible adjustments.
- Global JSON presets, saved to `FactoryGame/Configs/AutoRailSupports.json`.
- Rebindable keybinds through Options -> Keybindings.
- Mod options under Main Menu -> Mods -> Auto Rail Supports: adjustable Ctrl/Alt/Shift step sizes for
  position and rotation, and a background opacity slider for the in-world adjustments info panel.

[1.0.0]: https://github.com/itsalxx-sudo/AutoRailSupports/releases/tag/v1.0.0
