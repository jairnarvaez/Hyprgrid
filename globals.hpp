#pragma once

#include <hyprland/src/plugins/PluginAPI.hpp>

inline HANDLE PHANDLE = nullptr;
inline int hyprgrid_grid_size_x = 3;
inline int hyprgrid_grid_size_y = 3;
inline bool hyprgrid_enable_wrap_around = false;

void setGridSizeX(int x);
void setGridSizeY(int y);
void debugLog(const std::string& message);
