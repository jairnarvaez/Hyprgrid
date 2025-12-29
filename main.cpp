#include "Hyprgrid.hpp"
#include "globals.hpp"
#include <any>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/managers/input/InputManager.hpp>
#include <hyprland/src/managers/input/trackpad/GestureTypes.hpp>
#include <hyprland/src/managers/input/trackpad/TrackpadGestures.hpp>
#include <hyprland/src/managers/input/trackpad/gestures/ITrackpadGesture.hpp>
#include <hyprutils/string/ConstVarList.hpp>

#include <chrono>
#include <cxxabi.h>
#include <fstream>
#include <typeinfo>

static bool g_unloading = false;

void debugLog(const std::string& message)
{
    std::ofstream logFile("/tmp/mi_plugin_hyprland.log", std::ios::app);
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    logFile << std::ctime(&time) << ": " << message << std::endl;
    logFile.close();
}

std::string getTypeName(const std::type_info& type)
{
    int status;
    char* demangled = abi::__cxa_demangle(type.name(), nullptr, nullptr, &status);
    std::string result = (status == 0) ? demangled : type.name();
    free(demangled);
    return result;
}

APICALL EXPORT std::string PLUGIN_API_VERSION()
{
    return HYPRLAND_API_VERSION;
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

    direction = g_pTrackpadGestures->dirForString(data[1]);

    if (direction == TRACKPAD_GESTURE_DIR_NONE) {
        result.setError(std::format("Invalid direction: {}", data[1]).c_str());
        return result;
    }

    int startDataIdx = 2;
    uint32_t modMask = 0;
    float deltaScale = 1.F;

    while (true) {

        if (data[startDataIdx].starts_with("mod:")) {
            modMask = g_pKeybindManager->stringToModMask(std::string { data[startDataIdx].substr(4) });
            startDataIdx++;
            continue;
        } else if (data[startDataIdx].starts_with("scale:")) {
            try {
                deltaScale = std::clamp(std::stof(std::string { data[startDataIdx].substr(6) }), 0.1F, 10.F);
                startDataIdx++;
                continue;
            } catch (...) {
                result.setError(std::format("Invalid delta scale: {}", std::string { data[startDataIdx].substr(6) }).c_str());
                return result;
            }
        }

        break;
    }

    std::expected<void, std::string> resultFromGesture;

    if (data[startDataIdx] == "expo")
        resultFromGesture = g_pTrackpadGestures->addGesture(makeUnique<CHyprgrid>(), fingerCount, direction, modMask, deltaScale);
    else if (data[startDataIdx] == "unset")
        resultFromGesture = g_pTrackpadGestures->removeGesture(fingerCount, direction, modMask, deltaScale);
    else {
        result.setError(std::format("Invalid gesture: {}", data[startDataIdx]).c_str());
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

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle)
{
    PHANDLE = handle;

    HyprlandAPI::addConfigKeyword(PHANDLE, "hyprgrid-gesture-horizontal", ::hyprgridGestureKeyword, {});

    HyprlandAPI::addConfigKeyword(PHANDLE, "hyprgrid-gesture-vertical", ::hyprgridGestureKeyword, {});

    HyprlandAPI::addConfigKeyword(PHANDLE, "hyprgrid-grid-size-x", ::gridSizeXKeyword, {});

    HyprlandAPI::addConfigKeyword(PHANDLE, "hyprgrid-grid-size-y", ::gridSizeYKeyword, {});

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
