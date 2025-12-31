#include "Hyprgrid.hpp"
#include "globals.hpp"
#include <string>

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/desktop/DesktopTypes.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
#include <hyprland/src/managers/input/InputManager.hpp>
#include <hyprland/src/render/Renderer.hpp>

CHyprgrid::CHyprgrid() :
    m_swipeUseR("gestures:workspace_swipe_use_r"),
    m_swipeCancelRatio("gestures:workspace_swipe_cancel_ratio"),
    m_swipeMinSpeedToForce("gestures:workspace_swipe_min_speed_to_force"),
    m_swipeDirLockThreshold("gestures:workspace_swipe_direction_lock_threshold"),
    m_gapsWorkspaces("general:gaps_workspaces"),
    m_swipeDistance("gestures:workspace_swipe_distance") {
}

void CHyprgrid::begin(const ITrackpadGesture::STrackpadGestureBegin& e)
{
    ITrackpadGesture::begin(e);

    m_lastDelta = 0.F;
    m_firstUpdate = true;
    m_delta = 0;
    m_avgSpeed = 0;
    m_speedPoints = 0;
    m_vertanim = false;

    const auto PWORKSPACE = g_pCompositor->m_lastMonitor->m_activeWorkspace;
    m_workspaceBegin = PWORKSPACE;
    m_monitor = g_pCompositor->m_lastMonitor;
}

void CHyprgrid::update(const ITrackpadGesture::STrackpadGestureUpdate& e)
{
    if (m_firstUpdate) {
        m_firstUpdate = false;
        return;
    }

    const auto XDISTANCE = m_monitor->m_size.x + *m_gapsWorkspaces;
    const auto YDISTANCE = m_monitor->m_size.y + *m_gapsWorkspaces;

    // Calcular workspace IDs para el grid
    const std::string mode = *m_swipeUseR ? "r" : "m";
    const int gridWidth = hyprgrid_grid_size_x;

    int workspaceIDLeft, workspaceIDRight, workspaceIDUp, workspaceIDDown;

    if (hyprgrid_enable_wrap_around) {
        // Get current workspace position in grid
        int currentWorkspaceID = m_workspaceBegin->m_id;
        
        // Calculate position in grid (assuming 1-indexed workspaces)
        int row = (currentWorkspaceID - 1) / gridWidth;
        int col = (currentWorkspaceID - 1) % gridWidth;
        
        // Wrap-around logic for horizontal navigation
        int leftCol = (col == 0) ? gridWidth - 1 : col - 1;
        int rightCol = (col == gridWidth - 1) ? 0 : col + 1;
        
        // Wrap-around logic for vertical navigation
        int upRow = (row == 0) ? gridWidth - 1 : row - 1;
        int downRow = (row == gridWidth - 1) ? 0 : row + 1;
        
        // Convert back to workspace IDs
        int leftWorkspace = row * gridWidth + leftCol + 1;
        int rightWorkspace = row * gridWidth + rightCol + 1;
        int upWorkspace = upRow * gridWidth + col + 1;
        int downWorkspace = downRow * gridWidth + col + 1;
        
        workspaceIDLeft = getWorkspaceIDNameFromString(mode + "~" + std::to_string(leftWorkspace)).id;
        workspaceIDRight = getWorkspaceIDNameFromString(mode + "~" + std::to_string(rightWorkspace)).id;
        workspaceIDUp = getWorkspaceIDNameFromString(mode + "~" + std::to_string(upWorkspace)).id;
        workspaceIDDown = getWorkspaceIDNameFromString(mode + "~" + std::to_string(downWorkspace)).id;
    } else {
        // Original behavior without wrap-around
        workspaceIDLeft = getWorkspaceIDNameFromString(mode + "-1").id;
        workspaceIDRight = getWorkspaceIDNameFromString(mode + "+1").id;
        workspaceIDUp = getWorkspaceIDNameFromString(mode + "-" + std::to_string(gridWidth)).id;
        workspaceIDDown = getWorkspaceIDNameFromString(mode + "+" + std::to_string(gridWidth)).id;
    }
    // Validar workspaces
    if (workspaceIDLeft == WORKSPACE_INVALID || workspaceIDRight == WORKSPACE_INVALID || workspaceIDUp == WORKSPACE_INVALID || workspaceIDDown == WORKSPACE_INVALID || workspaceIDLeft == m_workspaceBegin->m_id || workspaceIDRight == m_workspaceBegin->m_id || workspaceIDUp == m_workspaceBegin->m_id || workspaceIDDown == m_workspaceBegin->m_id) {
        m_workspaceBegin = nullptr;
        HyprlandAPI::addNotification(PHANDLE, "Workspace invalido", { 1.0, 0.2, 0.2, 1.0 }, 5000);
        return;
    }

    // Calcular delta del gesto
    double currentDelta = distance(e);
    if (e.direction == TRACKPAD_GESTURE_DIR_LEFT || e.direction == TRACKPAD_GESTURE_DIR_UP) {
        currentDelta = -currentDelta;
    }

    // Configurar animación según dirección del gesto
    if (e.direction == TRACKPAD_GESTURE_DIR_LEFT || e.direction == TRACKPAD_GESTURE_DIR_RIGHT) {
        m_vertanim = false;
        g_pConfigManager->parseKeyword("animation", "workspaces, 1, 1, default, slide");
    }

    if (e.direction == TRACKPAD_GESTURE_DIR_UP || e.direction == TRACKPAD_GESTURE_DIR_DOWN) {
        m_vertanim = true;
        g_pConfigManager->parseKeyword("animation", "workspaces, 1, 1, default, slidevert");
    }

    m_workspaceBegin->m_forceRendering = true;

    // Actualizar delta y velocidad promedio
    const double D = m_delta - currentDelta;
    const double d = m_delta - D;
    m_delta = D;

    m_avgSpeed = (m_avgSpeed * m_speedPoints + abs(d)) / (m_speedPoints + 1);
    m_speedPoints++;

    m_delta = std::clamp(m_delta, sc<double>(-*m_swipeDistance), sc<double>(*m_swipeDistance));

    // Validar límites del grid
    // Límites horizontales (izquierda/derecha)
    bool hitLeftBorder = (m_workspaceBegin->m_id % hyprgrid_grid_size_x == 1 && m_delta < 0 && !m_vertanim);
    bool hitRightBorder = (m_workspaceBegin->m_id % hyprgrid_grid_size_x == 0 && m_delta > 0 && !m_vertanim);

    // Límites verticales (arriba/abajo)
    bool hitTopBorder = (m_workspaceBegin->m_id <= hyprgrid_grid_size_x && m_delta < 0 && m_vertanim);
    bool hitBottomBorder = (m_workspaceBegin->m_id > (hyprgrid_grid_size_x * (hyprgrid_grid_size_y - 1)) && m_delta > 0 && m_vertanim);

    bool hitBorder = hitLeftBorder || hitRightBorder || hitTopBorder || hitBottomBorder;

    if(!hyprgrid_enable_wrap_around){
        if (hitBorder) {
            m_delta = 0;
            g_pHyprRenderer->damageMonitor(m_monitor.lock());
            m_workspaceBegin->m_renderOffset->setValueAndWarp(Vector2D(0.0, 0.0));
            return;
        }
    }

    // Determinar workspace objetivo y opuesto según la dirección del gesto
    int targetWorkspaceID, oppositeWorkspaceID;
    if (e.direction == TRACKPAD_GESTURE_DIR_LEFT) {
        targetWorkspaceID = workspaceIDRight;
        oppositeWorkspaceID = workspaceIDLeft;
    } else if (e.direction == TRACKPAD_GESTURE_DIR_UP) {
        targetWorkspaceID = workspaceIDDown;
        oppositeWorkspaceID = workspaceIDUp;
    } else if (e.direction == TRACKPAD_GESTURE_DIR_RIGHT) {
        targetWorkspaceID = workspaceIDLeft;
        oppositeWorkspaceID = workspaceIDRight;
    } else { // TRACKPAD_GESTURE_DIR_DOWN
        targetWorkspaceID = workspaceIDUp;
        oppositeWorkspaceID = workspaceIDDown;
    }

    //HyprlandAPI::addNotification(PHANDLE,  "targetWorkspaceID: " + std::to_string(targetWorkspaceID), { 1.0, 0.2, 0.2, 1.0 }, 5000);
    //HyprlandAPI::addNotification(PHANDLE,  "oppositeWorkspaceID: " + std::to_string(oppositeWorkspaceID), { 1.0, 0.2, 0.2, 1.0 }, 5000);


    // Bloqueo de dirección
    if (m_initialDirection != 0 && m_initialDirection != (m_delta < 0 ? -1 : 1))
        m_delta = 0;
    else if (m_initialDirection == 0 && abs(m_delta) > *m_swipeDirLockThreshold)
        m_initialDirection = m_delta < 0 ? -1 : 1;

    // Validar y procesar el gesto
    const auto PWORKSPACE = g_pCompositor->getWorkspaceByID(targetWorkspaceID);

    bool isInvalidMove = (!PWORKSPACE) || (m_delta < 0 && targetWorkspaceID > m_workspaceBegin->m_id) || (m_delta >= 0 && targetWorkspaceID < m_workspaceBegin->m_id);

    if(!hyprgrid_enable_wrap_around){
        if (isInvalidMove) {
            m_delta = 0;
            return;
        }
    }

    PWORKSPACE->m_forceRendering = true;
    PWORKSPACE->m_alpha->setValueAndWarp(1.f);

    // Ocultar workspace opuesto
    if (targetWorkspaceID != oppositeWorkspaceID && oppositeWorkspaceID != m_workspaceBegin->m_id) {
        const auto PWORKSPACEOPPOSITE = g_pCompositor->getWorkspaceByID(oppositeWorkspaceID);
        if (PWORKSPACEOPPOSITE) {
            PWORKSPACEOPPOSITE->m_forceRendering = false;
            PWORKSPACEOPPOSITE->m_alpha->setValueAndWarp(0.f);
        }
    }

    // Calcular y aplicar offset de renderizado
    const double sign = m_delta < 0 ? -1.0 : 1.0;
    const double renderPerc = -m_delta / *m_swipeDistance;
    
    Vector2D targetOffset, beginOffset;

    if (m_vertanim) {
        targetOffset = {0.0, (renderPerc + sign) * YDISTANCE};
        beginOffset  = {0.0, renderPerc * YDISTANCE};
    } else {
        targetOffset = {(renderPerc + sign) * XDISTANCE, 0.0};
        beginOffset  = {renderPerc * XDISTANCE, 0.0};
    }

    PWORKSPACE->m_renderOffset->setValueAndWarp(targetOffset);
    m_workspaceBegin->m_renderOffset->setValueAndWarp(beginOffset);

    PWORKSPACE->updateWindowDecos();
    g_pHyprRenderer->damageMonitor(m_monitor.lock());
    m_workspaceBegin->updateWindowDecos();
}

void CHyprgrid::end(const ITrackpadGesture::STrackpadGestureEnd& e)
{
    // Calcular workspace IDs para el grid
    const std::string mode = *m_swipeUseR ? "r" : "m";
    const int gridWidth = hyprgrid_grid_size_x;

    int workspaceIDLeft, workspaceIDRight, workspaceIDUp, workspaceIDDown;

    if (hyprgrid_enable_wrap_around) {
        // Get current workspace position in grid
        int currentWorkspaceID = m_workspaceBegin->m_id;
        
        // Calculate position in grid (assuming 1-indexed workspaces)
        int row = (currentWorkspaceID - 1) / gridWidth;
        int col = (currentWorkspaceID - 1) % gridWidth;
        
        // Wrap-around logic for horizontal navigation
        int leftCol = (col == 0) ? gridWidth - 1 : col - 1;
        int rightCol = (col == gridWidth - 1) ? 0 : col + 1;
        
        // Wrap-around logic for vertical navigation
        int upRow = (row == 0) ? gridWidth - 1 : row - 1;
        int downRow = (row == gridWidth - 1) ? 0 : row + 1;
        
        // Convert back to workspace IDs
        int leftWorkspace = row * gridWidth + leftCol + 1;
        int rightWorkspace = row * gridWidth + rightCol + 1;
        int upWorkspace = upRow * gridWidth + col + 1;
        int downWorkspace = downRow * gridWidth + col + 1;
        
        workspaceIDLeft = getWorkspaceIDNameFromString(mode + "~" + std::to_string(leftWorkspace)).id;
        workspaceIDRight = getWorkspaceIDNameFromString(mode + "~" + std::to_string(rightWorkspace)).id;
        workspaceIDUp = getWorkspaceIDNameFromString(mode + "~" + std::to_string(upWorkspace)).id;
        workspaceIDDown = getWorkspaceIDNameFromString(mode + "~" + std::to_string(downWorkspace)).id;
    } else {
        // Original behavior without wrap-around
        workspaceIDLeft = getWorkspaceIDNameFromString(mode + "-1").id;
        workspaceIDRight = getWorkspaceIDNameFromString(mode + "+1").id;
        workspaceIDUp = getWorkspaceIDNameFromString(mode + "-" + std::to_string(gridWidth)).id;
        workspaceIDDown = getWorkspaceIDNameFromString(mode + "+" + std::to_string(gridWidth)).id;
    }

    // Obtener punteros a todos los workspaces del grid
    auto PWORKSPACER = g_pCompositor->getWorkspaceByID(workspaceIDRight);
    auto PWORKSPACEL = g_pCompositor->getWorkspaceByID(workspaceIDLeft);
    auto PWORKSPACEU = g_pCompositor->getWorkspaceByID(workspaceIDUp);
    auto PWORKSPACED = g_pCompositor->getWorkspaceByID(workspaceIDDown);

    const auto RENDEROFFSETMIDDLE = m_workspaceBegin->m_renderOffset->value();
    const auto XDISTANCE = m_monitor->m_size.x + *m_gapsWorkspaces;
    const auto YDISTANCE = m_monitor->m_size.y + *m_gapsWorkspaces;

    PHLWORKSPACE pSwitchedTo = nullptr;

    // Validar límites del grid
    bool hitLeftBorder = (m_workspaceBegin->m_id % hyprgrid_grid_size_x == 1 && m_delta == 0 && !m_vertanim);
    bool hitRightBorder = (m_workspaceBegin->m_id % hyprgrid_grid_size_x == 0 && m_delta > 0 && !m_vertanim);
    bool hitTopBorder = (m_workspaceBegin->m_id <= hyprgrid_grid_size_x && m_delta == 0 && m_vertanim);
    bool hitBottomBorder = (m_workspaceBegin->m_id > (hyprgrid_grid_size_x * (hyprgrid_grid_size_y - 1)) && m_delta > 0 && m_vertanim);

    bool hitBorder = hitLeftBorder || hitRightBorder || hitTopBorder || hitBottomBorder;
    
    if(hyprgrid_enable_wrap_around){
        hitBorder = false;
    }
    // Determinar si cancelar o completar el gesto
    bool shouldCancel = (abs(m_delta) < *m_swipeDistance * *m_swipeCancelRatio && (*m_swipeMinSpeedToForce == 0 || (*m_swipeMinSpeedToForce != 0 && m_avgSpeed < *m_swipeMinSpeedToForce))) || abs(m_delta) < 2 || hitBorder;

    if (shouldCancel) {
        // Cancelar gesto - revertir a workspace original
        if (m_delta <= 0) {
            // Revertir gesto hacia arriba o izquierda
            if (m_vertanim && PWORKSPACEU) {
                *PWORKSPACEU->m_renderOffset = Vector2D { 0.0, -YDISTANCE };
            } else if (PWORKSPACEL) {
                *PWORKSPACEL->m_renderOffset = Vector2D { -XDISTANCE, 0.0 };
            }
        } else {
            // Revertir gesto hacia abajo o derecha
            if (m_vertanim && PWORKSPACED) {
                *PWORKSPACED->m_renderOffset = Vector2D { 0.0, YDISTANCE };
            } else if (PWORKSPACER) {
                *PWORKSPACER->m_renderOffset = Vector2D { XDISTANCE, 0.0 };
            }
        }

        *m_workspaceBegin->m_renderOffset = Vector2D();
        pSwitchedTo = m_workspaceBegin;
    } else {
        // Completar gesto
        bool isMovingUpOrLeft = m_delta < 0;
        
        PHLWORKSPACE pTargetWorkspace = m_vertanim ? (isMovingUpOrLeft ? PWORKSPACEU : PWORKSPACED) : (isMovingUpOrLeft ? PWORKSPACEL : PWORKSPACER);
        int targetWorkspaceID = m_vertanim ? (isMovingUpOrLeft ? workspaceIDUp : workspaceIDDown) : (isMovingUpOrLeft ? workspaceIDLeft : workspaceIDRight);
        
        Vector2D finalBeginOffset;
        if (m_vertanim) {
            finalBeginOffset = {0.0, isMovingUpOrLeft ? YDISTANCE : -YDISTANCE};
        } else {
            finalBeginOffset = {isMovingUpOrLeft ? XDISTANCE : -XDISTANCE, 0.0};
        }

        const auto RENDEROFFSET = pTargetWorkspace ? pTargetWorkspace->m_renderOffset->value() : Vector2D();

        if (pTargetWorkspace) {
            m_monitor->changeWorkspace(targetWorkspaceID);
        } else {
            m_monitor->changeWorkspace(g_pCompositor->createNewWorkspace(targetWorkspaceID, m_monitor->m_id));
            pTargetWorkspace = g_pCompositor->getWorkspaceByID(targetWorkspaceID);
            pTargetWorkspace->rememberPrevWorkspace(m_workspaceBegin);
        }

        pTargetWorkspace->m_renderOffset->setValue(RENDEROFFSET);
        pTargetWorkspace->m_alpha->setValueAndWarp(1.f);

        m_workspaceBegin->m_renderOffset->setValue(RENDEROFFSETMIDDLE);
        *m_workspaceBegin->m_renderOffset = finalBeginOffset;
        m_workspaceBegin->m_alpha->setValueAndWarp(1.f);

        g_pInputManager->unconstrainMouse();

        const char* logMessage = m_vertanim ? (isMovingUpOrLeft ? "Ended swipe UP" : "Ended swipe DOWN") : (isMovingUpOrLeft ? "Ended swipe to the left" : "Ended swipe to the right");
        Debug::log(LOG, logMessage);

        pSwitchedTo = pTargetWorkspace;
    }

    m_initialDirection = 0;
}
