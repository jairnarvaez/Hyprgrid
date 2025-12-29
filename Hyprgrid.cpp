#include "Hyprgrid.hpp"
#include "globals.hpp"
#include <string>

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/desktop/DesktopTypes.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
#include <hyprland/src/managers/input/InputManager.hpp>
#include <hyprland/src/render/Renderer.hpp>

bool isSideWorkspace(int workspaceID, int gridSizeX) {
    if (gridSizeX <= 0) {
        return false;
    }
    return workspaceID % gridSizeX == 1 || workspaceID % gridSizeX == 0;
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

    static auto PSWIPEUSER = CConfigValue<Hyprlang::INT>("gestures:workspace_swipe_use_r");
    static auto PSWIPEDIRLOCKTHRESHOLD = CConfigValue<Hyprlang::INT>("gestures:workspace_swipe_direction_lock_threshold");
    static auto PWORKSPACEGAP = CConfigValue<Hyprlang::INT>("general:gaps_workspaces");

    const auto XDISTANCE = m_monitor->m_size.x + *PWORKSPACEGAP;
    const auto YDISTANCE = m_monitor->m_size.y + *PWORKSPACEGAP;

    // Calcular workspace IDs para el grid
    int workspaceIDLeft = getWorkspaceIDNameFromString((*PSWIPEUSER ? "r-1" : "m-1")).id;
    int workspaceIDRight = getWorkspaceIDNameFromString((*PSWIPEUSER ? "r+1" : "m+1")).id;
    int workspaceIDUp = getWorkspaceIDNameFromString((*PSWIPEUSER ? "r-" + std::to_string(m_gridSizeX) : "m-" + std::to_string(m_gridSizeX))).id;
    int workspaceIDDown = getWorkspaceIDNameFromString((*PSWIPEUSER ? "r+" + std::to_string(m_gridSizeX) : "m+" + std::to_string(m_gridSizeX))).id;

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

    m_delta = std::clamp(m_delta, sc<double>(-100), sc<double>(100));

    // Validar límites del grid
    // Límites horizontales (izquierda/derecha)
    bool hitLeftBorder = (m_workspaceBegin->m_id % m_gridSizeX == 1 && m_delta < 0 && !m_vertanim);
    bool hitRightBorder = (m_workspaceBegin->m_id % m_gridSizeX == 0 && m_delta > 0 && !m_vertanim);

    // Límites verticales (arriba/abajo)
    bool hitTopBorder = (m_workspaceBegin->m_id <= m_gridSizeX && m_delta < 0 && m_vertanim);
    bool hitBottomBorder = (m_workspaceBegin->m_id > (m_gridSizeX * (m_gridSizeY - 1)) && m_delta > 0 && m_vertanim);

    if (hitLeftBorder || hitRightBorder || hitTopBorder || hitBottomBorder) {
        m_delta = 0;
        g_pHyprRenderer->damageMonitor(m_monitor.lock());
        m_workspaceBegin->m_renderOffset->setValueAndWarp(Vector2D(0.0, 0.0));
        return;
    }

    // Determinar workspace objetivo según dirección
    int targetWorkspaceID;
    bool isMovingLeft = false;

    if (e.direction == TRACKPAD_GESTURE_DIR_LEFT || e.direction == TRACKPAD_GESTURE_DIR_UP) {
        targetWorkspaceID = (e.direction == TRACKPAD_GESTURE_DIR_LEFT) ? workspaceIDRight : workspaceIDDown;
        isMovingLeft = true;
    }
    if (e.direction == TRACKPAD_GESTURE_DIR_RIGHT || e.direction == TRACKPAD_GESTURE_DIR_DOWN) {
        targetWorkspaceID = (e.direction == TRACKPAD_GESTURE_DIR_RIGHT) ? workspaceIDLeft : workspaceIDUp;
        isMovingLeft = false;
    }

    // Bloqueo de dirección
    if (m_initialDirection != 0 && m_initialDirection != (m_delta < 0 ? -1 : 1))
        m_delta = 0;
    else if (m_initialDirection == 0 && abs(m_delta) > *PSWIPEDIRLOCKTHRESHOLD)
        m_initialDirection = m_delta < 0 ? -1 : 1;

    // Manejar gesto hacia arriba/izquierda (delta negativo)
    if (m_delta < 0) {
        const auto PWORKSPACE = g_pCompositor->getWorkspaceByID(targetWorkspaceID);

        if (targetWorkspaceID > m_workspaceBegin->m_id || !PWORKSPACE) {
            m_delta = 0;
            return;
        }

        PWORKSPACE->m_forceRendering = true;
        PWORKSPACE->m_alpha->setValueAndWarp(1.f);

        // Ocultar workspace opuesto
        int oppositeWorkspaceID = (e.direction == TRACKPAD_GESTURE_DIR_LEFT) ? workspaceIDLeft : workspaceIDUp;

        if (targetWorkspaceID != oppositeWorkspaceID && oppositeWorkspaceID != m_workspaceBegin->m_id) {
            const auto PWORKSPACEOPPOSITE = g_pCompositor->getWorkspaceByID(oppositeWorkspaceID);

            if (PWORKSPACEOPPOSITE) {
                PWORKSPACEOPPOSITE->m_forceRendering = false;
                PWORKSPACEOPPOSITE->m_alpha->setValueAndWarp(0.f);
            }
        }

        // Aplicar offset según tipo de animación
        if (m_vertanim) {
            PWORKSPACE->m_renderOffset->setValueAndWarp(Vector2D(0.0, ((-m_delta) / 100) * YDISTANCE - YDISTANCE));
            m_workspaceBegin->m_renderOffset->setValueAndWarp(Vector2D(0.0, ((-m_delta) / 100) * YDISTANCE));
        } else {
            PWORKSPACE->m_renderOffset->setValueAndWarp(Vector2D(((-m_delta) / 100) * XDISTANCE - XDISTANCE, 0.0));
            m_workspaceBegin->m_renderOffset->setValueAndWarp(Vector2D(((-m_delta) / 100) * XDISTANCE, 0.0));
        }

        PWORKSPACE->updateWindowDecos();
        debugLog("Gesto a la izquierda/arriba activado");
    }
    // Manejar gesto hacia abajo/derecha (delta positivo)
    else {
        const auto PWORKSPACE = g_pCompositor->getWorkspaceByID(targetWorkspaceID);

        if (targetWorkspaceID < m_workspaceBegin->m_id || !PWORKSPACE) {
            m_delta = 0;
            return;
        }

        PWORKSPACE->m_forceRendering = true;
        PWORKSPACE->m_alpha->setValueAndWarp(1.f);

        // Ocultar workspace opuesto
        int oppositeWorkspaceID = (e.direction == TRACKPAD_GESTURE_DIR_RIGHT) ? workspaceIDRight : workspaceIDDown;

        if (targetWorkspaceID != oppositeWorkspaceID && oppositeWorkspaceID != m_workspaceBegin->m_id) {
            const auto PWORKSPACEOPPOSITE = g_pCompositor->getWorkspaceByID(oppositeWorkspaceID);

            if (PWORKSPACEOPPOSITE) {
                PWORKSPACEOPPOSITE->m_forceRendering = false;
                PWORKSPACEOPPOSITE->m_alpha->setValueAndWarp(0.f);
            }
        }

        // Aplicar offset según tipo de animación
        if (m_vertanim) {
            PWORKSPACE->m_renderOffset->setValueAndWarp(Vector2D(0.0, ((-m_delta) / 100) * YDISTANCE + YDISTANCE));
            m_workspaceBegin->m_renderOffset->setValueAndWarp(Vector2D(0.0, ((-m_delta) / 100) * YDISTANCE));
        } else {
            PWORKSPACE->m_renderOffset->setValueAndWarp(Vector2D(((-m_delta) / 100) * XDISTANCE + XDISTANCE, 0.0));
            m_workspaceBegin->m_renderOffset->setValueAndWarp(Vector2D(((-m_delta) / 100) * XDISTANCE, 0.0));
        }

        PWORKSPACE->updateWindowDecos();
        debugLog("Gesto a la derecha/abajo activado");
    }

    g_pHyprRenderer->damageMonitor(m_monitor.lock());
    m_workspaceBegin->updateWindowDecos();

    debugLog("m_delta: " + std::to_string(m_delta));
}

void CHyprgrid::end(const ITrackpadGesture::STrackpadGestureEnd& e)
{
    static auto PSWIPEPERC = CConfigValue<Hyprlang::FLOAT>("gestures:workspace_swipe_cancel_ratio");
    static auto PSWIPEUSER = CConfigValue<Hyprlang::INT>("gestures:workspace_swipe_use_r");
    static auto PSWIPEFORC = CConfigValue<Hyprlang::INT>("gestures:workspace_swipe_min_speed_to_force");
    static auto PWORKSPACEGAP = CConfigValue<Hyprlang::INT>("general:gaps_workspaces");

    // Obtener workspace IDs del grid
    auto workspaceIDLeft = getWorkspaceIDNameFromString((*PSWIPEUSER ? "r-1" : "m-1")).id;
    auto workspaceIDRight = getWorkspaceIDNameFromString((*PSWIPEUSER ? "r+1" : "m+1")).id;
    auto workspaceIDUp = getWorkspaceIDNameFromString((*PSWIPEUSER ? "r-" + std::to_string(m_gridSizeX) : "m-" + std::to_string(m_gridSizeX))).id;
    auto workspaceIDDown = getWorkspaceIDNameFromString((*PSWIPEUSER ? "r+" + std::to_string(m_gridSizeX) : "m+" + std::to_string(m_gridSizeX))).id;

    const auto SWIPEDISTANCE = 100;

    // Obtener punteros a todos los workspaces del grid
    auto PWORKSPACER = g_pCompositor->getWorkspaceByID(workspaceIDRight);
    auto PWORKSPACEL = g_pCompositor->getWorkspaceByID(workspaceIDLeft);
    auto PWORKSPACEU = g_pCompositor->getWorkspaceByID(workspaceIDUp);
    auto PWORKSPACED = g_pCompositor->getWorkspaceByID(workspaceIDDown);

    const auto RENDEROFFSETMIDDLE = m_workspaceBegin->m_renderOffset->value();
    const auto XDISTANCE = m_monitor->m_size.x + *PWORKSPACEGAP;
    const auto YDISTANCE = m_monitor->m_size.y + *PWORKSPACEGAP;

    PHLWORKSPACE pSwitchedTo = nullptr;

    // Validar límites del grid
    bool hitLeftBorder = (m_workspaceBegin->m_id % m_gridSizeX == 1 && m_delta == 0 && !m_vertanim);
    bool hitRightBorder = (m_workspaceBegin->m_id % m_gridSizeX == 0 && m_delta > 0 && !m_vertanim);
    bool hitTopBorder = (m_workspaceBegin->m_id <= m_gridSizeX && m_delta == 0 && m_vertanim);
    bool hitBottomBorder = (m_workspaceBegin->m_id > (m_gridSizeX * (m_gridSizeY - 1)) && m_delta > 0 && m_vertanim);

    debugLog(">>===========Gesto finalizado==========<<");
    debugLog("ID Workspace: " + std::to_string(m_workspaceBegin->m_id));
    debugLog("m_delta: " + std::to_string(m_delta));
    debugLog("m_vertanim: " + std::to_string(m_vertanim));

    if (hitTopBorder) {
        HyprlandAPI::addNotification(PHANDLE, "Se ha alcanzado el borde superior, cambiando workspace cancelado", { 1.0, 0.2, 0.2, 1.0 }, 5000);
    }

    if (hitBottomBorder) {
        HyprlandAPI::addNotification(PHANDLE, "Se ha alcanzado el borde inferior, cambiando workspace cancelado", { 1.0, 0.2, 0.2, 1.0 }, 5000);
    }
    // Determinar si cancelar o completar el gesto
    bool shouldCancel = (abs(m_delta) < SWIPEDISTANCE * *PSWIPEPERC && (*PSWIPEFORC == 0 || (*PSWIPEFORC != 0 && m_avgSpeed < *PSWIPEFORC))) || abs(m_delta) < 2 || hitLeftBorder || hitRightBorder || hitTopBorder || hitBottomBorder;

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
    } else if (m_delta < 0) {
        // Completar gesto hacia arriba o izquierda
        PHLWORKSPACE PTARGETWORKSPACE = m_vertanim ? PWORKSPACEU : PWORKSPACEL;
        int targetWorkspaceID = m_vertanim ? workspaceIDUp : workspaceIDLeft;

        const auto RENDEROFFSET = PTARGETWORKSPACE ? PTARGETWORKSPACE->m_renderOffset->value() : Vector2D();

        if (PTARGETWORKSPACE) {
            m_monitor->changeWorkspace(targetWorkspaceID);
        } else {
            m_monitor->changeWorkspace(g_pCompositor->createNewWorkspace(targetWorkspaceID, m_monitor->m_id));
            PTARGETWORKSPACE = g_pCompositor->getWorkspaceByID(targetWorkspaceID);
            PTARGETWORKSPACE->rememberPrevWorkspace(m_workspaceBegin);
        }

        PTARGETWORKSPACE->m_renderOffset->setValue(RENDEROFFSET);
        PTARGETWORKSPACE->m_alpha->setValueAndWarp(1.f);

        m_workspaceBegin->m_renderOffset->setValue(RENDEROFFSETMIDDLE);
        if (m_vertanim)
            *m_workspaceBegin->m_renderOffset = Vector2D(0.0, YDISTANCE);
        else
            *m_workspaceBegin->m_renderOffset = Vector2D(XDISTANCE, 0.0);
        m_workspaceBegin->m_alpha->setValueAndWarp(1.f);

        g_pInputManager->unconstrainMouse();

        Debug::log(LOG, m_vertanim ? "Ended swipe UP" : "Ended swipe to the left");

        pSwitchedTo = PTARGETWORKSPACE;
    } else {
        // Completar gesto hacia abajo o derecha
        PHLWORKSPACE PTARGETWORKSPACE = m_vertanim ? PWORKSPACED : PWORKSPACER;
        int targetWorkspaceID = m_vertanim ? workspaceIDDown : workspaceIDRight;

        const auto RENDEROFFSET = PTARGETWORKSPACE ? PTARGETWORKSPACE->m_renderOffset->value() : Vector2D();

        if (PTARGETWORKSPACE) {
            m_monitor->changeWorkspace(targetWorkspaceID);
        } else {
            m_monitor->changeWorkspace(g_pCompositor->createNewWorkspace(targetWorkspaceID, m_monitor->m_id));
            PTARGETWORKSPACE = g_pCompositor->getWorkspaceByID(targetWorkspaceID);
            PTARGETWORKSPACE->rememberPrevWorkspace(m_workspaceBegin);
        }

        PTARGETWORKSPACE->m_renderOffset->setValue(RENDEROFFSET);
        PTARGETWORKSPACE->m_alpha->setValueAndWarp(1.f);

        m_workspaceBegin->m_renderOffset->setValue(RENDEROFFSETMIDDLE);
        if (m_vertanim)
            *m_workspaceBegin->m_renderOffset = Vector2D(0.0, -YDISTANCE);
        else
            *m_workspaceBegin->m_renderOffset = Vector2D(-XDISTANCE, 0.0);
        m_workspaceBegin->m_alpha->setValueAndWarp(1.f);

        g_pInputManager->unconstrainMouse();

        Debug::log(LOG, m_vertanim ? "Ended swipe DOWN" : "Ended swipe to the right");

        pSwitchedTo = PTARGETWORKSPACE;
    }

    m_initialDirection = 0;
}
