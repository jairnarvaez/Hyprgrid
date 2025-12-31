#pragma once

#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/config/ConfigManager.hpp>  
#include <hyprland/src/desktop/DesktopTypes.hpp>

inline HANDLE PHANDLE = nullptr;
inline int hyprgrid_grid_size_x = 3;
inline int hyprgrid_grid_size_y = 3;
inline bool hyprgrid_enable_wrap_around = false;
inline int g_currentWorkspaceID = 1;
inline PHLMONITORREF g_monitor;

inline  auto  g_swipeUseR = CConfigValue<Hyprlang::INT>("gestures:workspace_swipe_use_r");

void setGridSizeX(int x);
void setGridSizeY(int y);
void debugLog(const std::string& message);
