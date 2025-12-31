#pragma once

#include "globals.hpp"
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/desktop/DesktopTypes.hpp>
#include <hyprland/src/managers/input/trackpad/gestures/ITrackpadGesture.hpp>

enum eHyprgridDirection {
    HYPRGRID_LEFT,
    HYPRGRID_RIGHT,
    HYPRGRID_UP,
    HYPRGRID_DOWN
};


class CHyprgrid : public ITrackpadGesture {
public:
    CHyprgrid();
    virtual ~CHyprgrid() = default;

    virtual void begin(const ITrackpadGesture::STrackpadGestureBegin& e);
    virtual void update(const ITrackpadGesture::STrackpadGestureUpdate& e);
    virtual void end(const ITrackpadGesture::STrackpadGestureEnd& e);

    void calculateWorkspaceIDs(int currentWorkspaceID, int& workspaceIDLeft, int& workspaceIDRight, 
                               int& workspaceIDUp, int& workspaceIDDown);
    void handleGesture(const ITrackpadGesture::STrackpadGestureUpdate& e, 
                       int workspaceIDLeft, int workspaceIDRight, 
                       int workspaceIDUp, int workspaceIDDown);
    void finalizeGesture(const ITrackpadGesture::STrackpadGestureEnd& e, 
                         int workspaceIDLeft, int workspaceIDRight, 
                         int workspaceIDUp, int workspaceIDDown);
    int getAdjacentWorkspaceID(eHyprgridDirection direction);
    
private:
    PHLWORKSPACE m_workspaceBegin = nullptr;
    PHLMONITORREF m_monitor;

    double m_delta = 0;
    int m_initialDirection = 0;
    float m_avgSpeed = 0;
    int m_speedPoints = 0;
    bool m_vertanim = false;

    float m_lastDelta = 0.F;
    bool m_firstUpdate = false;

    // Config values
    CConfigValue<Hyprlang::FLOAT> m_swipeCancelRatio;
    CConfigValue<Hyprlang::INT>   m_swipeMinSpeedToForce;
    CConfigValue<Hyprlang::INT>   m_swipeDirLockThreshold;
    CConfigValue<Hyprlang::INT>   m_gapsWorkspaces;
    CConfigValue<Hyprlang::INT>   m_swipeDistance;
};
