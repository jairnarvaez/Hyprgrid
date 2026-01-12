#include "globals.hpp"
#include "config.hpp"
#include "dispatcher.hpp"

APICALL EXPORT std::string PLUGIN_API_VERSION()
{
    return HYPRLAND_API_VERSION;
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
