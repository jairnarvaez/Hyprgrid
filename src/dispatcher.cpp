#include "globals.hpp"
#include "Hyprgrid.hpp"

#include <hyprland/src/desktop/state/FocusState.hpp>
#include <hyprland/src/config/ConfigManager.hpp>

SDispatchResult onGridDispatcher(std::string arg) {
    CHyprgrid hyprgrid;

    int currentWorkspaceID = g_currentWorkspaceID;
    int targetWorkspaceID = currentWorkspaceID;

    if (arg == "left") {
        g_pConfigManager->parseKeyword("animation", "workspaces, 1, 1, default, slide");
        targetWorkspaceID = hyprgrid.getAdjacentWorkspaceID(HYPRGRID_LEFT);
    } else if (arg == "right") {
        g_pConfigManager->parseKeyword("animation", "workspaces, 1, 1, default, slide");
        targetWorkspaceID = hyprgrid.getAdjacentWorkspaceID(HYPRGRID_RIGHT);
    } else if (arg == "up") {
        g_pConfigManager->parseKeyword("animation", "workspaces, 1, 1, default, slidevert");
        targetWorkspaceID = hyprgrid.getAdjacentWorkspaceID(HYPRGRID_UP);
    } else if (arg == "down") {
        g_pConfigManager->parseKeyword("animation", "workspaces, 1, 1, default, slidevert");
        targetWorkspaceID = hyprgrid.getAdjacentWorkspaceID(HYPRGRID_DOWN);
    }
    g_monitor = Desktop::focusState()->monitor();
    g_monitor->changeWorkspace(targetWorkspaceID);
    return {};
}
