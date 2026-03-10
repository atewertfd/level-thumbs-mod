#pragma once

#ifdef GEODE_IS_WINDOWS

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class Zoom {
public:
    static Zoom& get();

    void update(float dt);
    void doZoom(float amount);

    bool m_isTouching = false;

private:
    Zoom() = default;

    CCPoint m_lastMousePos {};
    CCPoint m_deltaMousePos {};
    bool m_isDragging = false;
};

#endif
