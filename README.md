# Hyprland 2D Grid Gestures

A Hyprland plugin that enables 2D grid-based navigation using touch gestures. This project allows for intuitive workspace switching across both horizontal and vertical axes.

https://github.com/user-attachments/assets/c7d20ddf-8fe6-4315-8763-5e50833bbc40

## Requirements

- Hyprland headers and development files
- C++ compiler (GCC or Clang)
- Make

## Installation

To compile the plugin, run the following command in the root directory:

```bash
make
```

This will generate the myplugin.so file required by Hyprland.

## Usage
### Loading the plugin

Use hyprctl to load the compiled plugin into your current session:

```
hyprctl plugin load /path/to/plugin/myplugin.so
```

### Unloading the plugin

To remove the plugin from the current session:

```
hyprctl plugin unload /path/to/plugin/myplugin.so
```

### Configuration

Add the plugin to your hyprland.conf to ensure it loads on startup:

```
exec-once = hyprctl plugin load /path/to/plugin/myplugin.so
hyprgrid-gesture-horizontal = 4, horizontal, expo
hyprgrid-gesture-vertical = 4, vertical, expo
hyprgrid-grid-size-x = 3
hyprgrid-grid-size-y = 3
hyprgrid-grid-wrap-around = false
```

```
gestures {
    workspace_swipe_distance = 100
    workspace_swipe_cancel_ratio = 0.15
    workspace_swipe_min_speed_to_force = 5
    workspace_swipe_direction_lock = true
    workspace_swipe_direction_lock_threshold = 10
    workspace_swipe_create_new = true
}
```

### TODO

[x] **Configurable Wrap-Around Navigation**: Add an option to allow "wrapping around" the grid, so swiping right on the
   last workspace of a row moves to the first, and swiping left on the first moves to the last (and similarly for
   vertical navigation).
```
   hyprgrid-grid-wrap-around = true
```
https://github.com/user-attachments/assets/7271fc46-41db-43b2-86c1-9d698cd699f5
   
[ ] **Keyboard Shortcut Navigation**: Implement keybindings to navigate the workspace grid (e.g., using arrow keys with a
   modifier), making the plugin accessible without a trackpad.
   
[ ] **Move Window with Focus**: Add a feature to move the currently focused window to the new workspace when navigating
   the grid. This could be a separate set of keybindings or a configurable option for gestures.
   
[ ] **On-Screen Display (OSD) for Grid**: Implement a visual overlay that briefly shows the workspace grid and highlights
   the active workspace during navigation for better user feedback.
