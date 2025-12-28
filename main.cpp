#include "ExpoGesture.hpp"
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

static Hyprlang::CParseResult expoGestureKeyword(const char* LHS, const char* RHS)
{
    Hyprlang::CParseResult result;

    if (g_unloading)
        return result;

    CConstVarList data(RHS);

    debugLog("Función llamada desde expoGestureKeyword");

    debugLog("Tipo de data[1]: " + getTypeName(typeid(data[0])));
    debugLog("Contenido de data[1] " + std::string(data[0]));

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
        resultFromGesture = g_pTrackpadGestures->addGesture(makeUnique<CExpoGesture>(), fingerCount, direction, modMask, deltaScale);
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

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle)
{
    PHANDLE = handle;

    debugLog("=======================================================");

    // static auto mouseButton = HyprlandAPI::registerCallbackDynamic(PHANDLE, "mouseButton", [](void* self, SCallbackInfo& info, std::any data) {
    //     auto e = std::any_cast<IPointer::SButtonEvent>(data);
    //     if (e.button == BTN_LEFT) {
    //         if (e.state) { // clicked
    //             HyprlandAPI::addNotification(
    //                 PHANDLE,
    //                 "hola",
    //                 { 1.0, 0.2, 0.2, 1.0 },
    //                 5000);
    //         }
    //     }
    // });

    debugLog("Función llamada desde el main");

    HyprlandAPI::addConfigKeyword(PHANDLE, "hyprexpo-gesture-horizontal", ::expoGestureKeyword, {});

    HyprlandAPI::addConfigKeyword(PHANDLE, "hyprexpo-gesture-vertical", ::expoGestureKeyword, {});

    return {
        "left-swipe-gesture",
        "Un plugin que detecta swipes hacia la izquierda con 3 dedos.",
        "jair",
        "1.0"
    };
}
APICALL EXPORT void PLUGIN_EXIT(HANDLE handle)
{
    g_unloading = true;
    g_pConfigManager->reload();
}
