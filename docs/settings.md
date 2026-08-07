---
title: Settings & Keybinds
nav_order: 3
---

# Settings & Keybinds

The support configurations are saved **per player** in multiplayer.

---

## Keybinds

### Rebindable

Some keybindings can be changed, under **Options -> Keybindings**. They sit in two categories:

**Auto Rail Supports**:

| Action | Default |
|---|---|
| Open Configuration Menu | `U` |
| Open ARS Adjustments | `K` |
| Toggle Automatic Supports | `I` |

**Auto Rail Supports - ARS Adjustments** - keybindings for ARS Adjustments in build/dismantle gun:

| Action | Default |
|---|---|
| Cycle to Previous Part | `Comma` |
| Cycle to Next Part | `Period` |
| Cycle Setting Modes | `R` |
| Cycle Rotation Axis | `Z` |

### Fixed

Everything else in ARS Adjustments is fixed and **can't be rebound**:

| Action | Key |
|---|---|
| Move the selected part | `↑` `↓` `←` `→` |
| Move up / down | `Page Up` / `Page Down` |
| Rotate around the selected axis | `Mouse Wheel` |
| Lock / unlock the rail hologram | `H` |
| Apply supports to the rail you're aiming at *(dismantle gun only)* | `LMB` |
| Step size while adjusting | `Ctrl` / `Alt` / `Shift` |

Open ARS Adjustments with `K` while placing a rail with the build gun, or while aiming at an
existing rail with the dismantle gun.

### Step size modifiers

Hold while adjusting:

| Modifier | Position & Tiling | Rotation |
|---|---|---|
| *(none)* | 1 m | 15° |
| `Ctrl` | 0.01 m | 0.1° |
| `Alt` | 0.1 m | 1° |
| `Shift` | 4 m | 45° |

The modifier keys themselves can't be rebound, but **the amounts they step by are**.
See [Mod Options](#mod-options). The unmodified 1 m / 15° step is fixed. 
The same modifiers can be used for the sliders in the `U` menu.

---

## Under-Rail

The blueprint tiled continuously beneath the track.

### Blueprint

Which of your saved blueprints to place under the rail. Leave empty to place nothing under the rail and use pillars
only.

### Tiling Length
{: #tiling-length }

Length of each blueprint tile. Set it shorter than the blueprint's real length to close gaps in curves.

### Cover End Gap

Adds one extra copy centered on the end of the rail when there are gaps at the ends.

### Position Offset

Nudge the blueprint along the track, sideways, and vertically.

### Rotation

Rotates every copy on top of the rail's own direction.

---

## Support Pillars

Up to four pillar blueprints placed at regular intervals. Each slot has its own blueprint and settings.

*(Of course these don't have to be pillar blueprints. That's just what I named them.)*

### Blueprint

Which of your saved blueprints this slot places. Leave empty to disable the slot.

### Pillar Spacing

How many segments apart the pillar repeats.

### Spacing Offset

How many segments to skip before this slot's first pillar.

### Follow Rail Incline

- **Off** - pillars stay vertical regardless of the track's incline.
- **On** - pillars tilt to match the track, same as under-rail segments.

### Prevent Overlap

Prevents placement of this pillar when a higher priority slot lands on the same segment.
Slot 4 is highest priority, slot 1 lowest.

Turn it off if you deliberately want two blueprints on the same segment.

### Position Offset / Rotation

Same as under-rail, per slot.

---

## General

### Enabled

Master toggle, also bound to `I`. When off, rails build like normal.

### Combine Blueprints

Makes the placed under-rail copies and pillars dismantle together as one blueprint. Nothing is saved to your blueprint library.

## Presets

Save the whole configuration as a preset, and load it later. Presets are stored on your computer 
at `FactoryGame/Configs/AutoRailSupports.json`, and are shared across all save games.

---

## Mod Options
{: #mod-options }

**Main Menu -> Mods -> Auto Rail Supports**

Mod settings shared by every world you play.

### Adjustment steps

Adjust how far one key press moves a support while you hold a modifier (in meters and degrees):

| Setting | Default |
|---|---|
| Tiny adjustment step (`Ctrl`) | 0.01 |
| Small adjustment step (`Alt`) | 0.1 |
| Large adjustment step (`Shift`) | 4 |
| Tiny rotation step (`Ctrl`) | 0.1 |
| Small rotation step (`Alt`) | 1 |
| Large rotation step (`Shift`) | 45 |

### Info panel background opacity

The ARS Adjustments info panel on the left has a transparent background by default. 
Raise this to darken the background behind it. `0` is fully transparent, `1` is fully opaque.

---

## Chat Commands

| Command | What it does |
|---|---|
| `/ars` | Open the configuration menu |
| `/ars apply` | Add supports under the rail you're aiming at |
