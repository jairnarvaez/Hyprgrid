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
hyprgrid_grid_size_x = 3
hyprgrid_grid_size_y = 3
```
