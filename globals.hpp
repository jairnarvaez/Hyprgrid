#pragma once

#include <hyprland/src/plugins/PluginAPI.hpp>

inline HANDLE PHANDLE = nullptr;
inline int hyprexpo_grid_size_x = 3;
inline int hyprexpo_grid_size_y = 3;

void setGridSizeX(int x);
void setGridSizeY(int y);
void debugLog(const std::string& message);
