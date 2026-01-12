#include "config.hpp"
#include "globals.hpp"
#include "Hyprgrid.hpp"

#include <cstring>
#include <format>

#include <hyprutils/string/ConstVarList.hpp>
#include <hyprland/src/managers/input/trackpad/GestureTypes.hpp>
#include <hyprland/src/managers/input/trackpad/TrackpadGestures.hpp>

Hyprlang::CParseResult gridSizeKeyword(const char* LHS, const char* RHS, int& target, const char* name) {
    Hyprlang::CParseResult result;
    try {
        int value = std::stoi(RHS);
         if (value > 0) {
            target = value;
         }
    } catch (...) {
        result.setError(std::format("Invalid value '{}' for {}", RHS, name).c_str());
    }
    return result;
}

Hyprlang::CParseResult gridSizeXKeyword(const char* LHS, const char* RHS) {
    return gridSizeKeyword(LHS, RHS, hyprgrid_grid_size_x, "grid size x");
}

Hyprlang::CParseResult gridSizeYKeyword(const char* LHS, const char* RHS) {
    return gridSizeKeyword(LHS, RHS, hyprgrid_grid_size_y, "grid size y");
}

Hyprlang::CParseResult wrapAroundKeyword(const char* LHS, const char* RHS)
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


Hyprlang::CParseResult hyprgridGestureKeyword(const char* LHS, const char* RHS)
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
