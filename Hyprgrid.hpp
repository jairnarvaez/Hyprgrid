#pragma once

#include "globals.hpp"
#include <hyprland/src/desktop/DesktopTypes.hpp>
#include <hyprland/src/managers/input/trackpad/gestures/ITrackpadGesture.hpp>

class CHyprgrid : public ITrackpadGesture {
public:
    CHyprgrid() {
        m_gridSizeX = hyprgrid_grid_size_x;
        m_gridSizeY = hyprgrid_grid_size_y;
    }
    virtual ~CHyprgrid() = default;

    virtual void begin(const ITrackpadGesture::STrackpadGestureBegin& e);
    virtual void update(const ITrackpadGesture::STrackpadGestureUpdate& e);
    virtual void end(const ITrackpadGesture::STrackpadGestureEnd& e);

    int m_gridSizeX;
    int m_gridSizeY;

private:
    PHLWORKSPACE m_workspaceBegin = nullptr;
    PHLMONITORREF m_monitor;

    double m_delta = 0;
    int m_initialDirection = 0;
    float m_avgSpeed = 0;
    int m_speedPoints = 0;
    int m_touchID = 0;
    bool m_vertanim = false;

    float m_lastDelta = 0.F;
    bool m_firstUpdate = false;
};
