#include "Hyprgrid.hpp"
#include "globals.hpp"
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/managers/input/InputManager.hpp>
#include <hyprland/src/managers/input/trackpad/GestureTypes.hpp>
#include <hyprland/src/managers/input/trackpad/TrackpadGestures.hpp>
#include <hyprland/src/managers/input/trackpad/gestures/ITrackpadGesture.hpp>
#include <hyprutils/string/ConstVarList.hpp>
#include <cstring>

static bool g_unloading = false;

APICALL EXPORT std::string PLUGIN_API_VERSION()
{
    return HYPRLAND_API_VERSION;
}

static SDispatchResult onGridDispatcher(std::string arg) {
    int currentWorkspaceID = g_currentWorkspaceID;
    int targetWorkspaceID = currentWorkspaceID;
    CHyprgrid hyprgrid;
    
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
	HyprlandAPI::addNotification(PHANDLE, "Cambiando a Workspace" + std::to_string(targetWorkspaceID), { 1.0, 0.2, 0.2, 1.0 }, 5000);
    g_monitor = g_pCompositor->m_lastMonitor;
    g_monitor->changeWorkspace(targetWorkspaceID);
    return {};
}

static Hyprlang::CParseResult hyprgridGestureKeyword(const char* LHS, const char* RHS)
{
    Hyprlang::CParseResult result;

    if (g_unloading)
        return result;

    CConstVarList data(RHS);
    size_t fingerCount = 0;
    eTrackpadGestureDirection direction = TRACKPAD_GESTURE_DIR_NONE;

    try {
        fingerCount = std::stoul(std::string { data[0] });
    } catch (...) {
        result.setError(std::format("Invalid value {} for finger count", data[0]).c_str());
        return result;
    }

    if (fingerCount <= 1 || fingerCount >= 10) {
        result.setError(std::format("Invalid value {} for finger count", data[0]).c_str());
        return result;
    }

    int argIndex = 1;
    uint32_t modMask = 0;
    float deltaScale = 1.F;

    for (; argIndex < data.size(); ++argIndex) {
        const auto& arg = data[argIndex];
        if (arg.starts_with("mod:")) {
            modMask = g_pKeybindManager->stringToModMask(std::string { arg.substr(4) });
        } else if (arg.starts_with("scale:")) {
            try {
                deltaScale = std::clamp(std::stof(std::string { arg.substr(6) }), 0.1F, 10.F);
            } catch (...) {
                result.setError(std::format("Invalid delta scale: {}", std::string { arg.substr(6) }).c_str());
                return result;
            }
        } else {
            // Not a mod or scale, so it must be the gesture command (expo/unset)
            break;
        }
    }

    std::expected<void, std::string> resultFromGesture;

    if (data[argIndex] == "vertical"){
        direction = g_pTrackpadGestures->dirForString("vertical");
        if (direction == TRACKPAD_GESTURE_DIR_NONE) {
            result.setError(std::format("Invalid direction: {}", data[1]).c_str());
            return result;
        }
        resultFromGesture = g_pTrackpadGestures->addGesture(makeUnique<CHyprgrid>(), fingerCount, direction, modMask, deltaScale);
    }
    else if (data[argIndex] == "horizontal"){ 
        direction = g_pTrackpadGestures->dirForString("horizontal");
        if (direction == TRACKPAD_GESTURE_DIR_NONE) {
            result.setError(std::format("Invalid direction: {}", data[1]).c_str());
            return result;
        }
        resultFromGesture = g_pTrackpadGestures->addGesture(makeUnique<CHyprgrid>(), fingerCount, direction, modMask, deltaScale);
    }    
    else if (data[argIndex] == "unset")
        resultFromGesture = g_pTrackpadGestures->removeGesture(fingerCount, direction, modMask, deltaScale);
    else {
        result.setError(std::format("Invalid gesture: {}", data[argIndex]).c_str());
        return result;
    }

    if (!resultFromGesture) {
        result.setError(resultFromGesture.error().c_str());
        return result;
    }

    return result;
}

void setGridSizeX(int x) {
    if (x > 0) {
        hyprgrid_grid_size_x = x;
    }
}

void setGridSizeY(int y) {
    if (y > 0) {
        hyprgrid_grid_size_y = y;
    }
}

static Hyprlang::CParseResult gridSizeXKeyword(const char* LHS, const char* RHS)
{
    Hyprlang::CParseResult result;
    try {
        setGridSizeX(std::stoi(RHS));
    } catch (...) {
        result.setError(std::format("Invalid value {} for grid size x", RHS).c_str());
    }
    return result;
}

static Hyprlang::CParseResult gridSizeYKeyword(const char* LHS, const char* RHS)
{
    Hyprlang::CParseResult result;
    try {
        setGridSizeY(std::stoi(RHS));
    } catch (...) {
        result.setError(std::format("Invalid value {} for grid size y", RHS).c_str());
    }
    return result;
}

static Hyprlang::CParseResult wrapAroundKeyword(const char* LHS, const char* RHS)
{
    Hyprlang::CParseResult result;
    
    if (strcmp(RHS, "true") == 0) {
        hyprgrid_enable_wrap_around = true;
    } else if (strcmp(RHS, "false") == 0) {
        hyprgrid_enable_wrap_around = false;
    } else {
        result.setError(std::format("Invalid value '{}' for enable wrap. Expected 'true' or 'false'", RHS).c_str());
    }
    
    return result;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle)
{
    PHANDLE = handle;

    HyprlandAPI::addDispatcherV2(PHANDLE, "hyprgrid:move", ::onGridDispatcher);

    HyprlandAPI::addConfigKeyword(PHANDLE, "hyprgrid-gesture", ::hyprgridGestureKeyword, {});

    HyprlandAPI::addConfigKeyword(PHANDLE, "hyprgrid-grid-size-x", ::gridSizeXKeyword, {});

    HyprlandAPI::addConfigKeyword(PHANDLE, "hyprgrid-grid-size-y", ::gridSizeYKeyword, {});

    HyprlandAPI::addConfigKeyword(PHANDLE, "hyprgrid-grid-wrap-around", ::wrapAroundKeyword, {});

    return {
        "hyprgrid",
        "A plugin to create a grid of workspaces and navigate them with gestures.",
        "jair",
        "1.0"
    };
}
APICALL EXPORT void PLUGIN_EXIT(HANDLE handle)
{
    g_unloading = true;
    g_pConfigManager->reload();
}
