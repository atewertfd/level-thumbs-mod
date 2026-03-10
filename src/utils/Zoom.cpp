#ifdef GEODE_IS_WINDOWS

#include "Zoom.hpp"

#include <algorithm>

#include <Geode/modify/CCEGLView.hpp>
#include <Geode/modify/CCMouseDispatcher.hpp>
#include <Geode/modify/CCScheduler.hpp>

using namespace geode::prelude;

namespace {
bool isInsideLayer(CCLayer* layer, CCPoint worldPos) {
    auto x = layer->getPositionX() - layer->getContentSize().width * layer->getAnchorPoint().x;
    auto y = layer->getPositionY() - layer->getContentSize().height * layer->getAnchorPoint().y;
    auto rect = CCRect { x, y, layer->getContentSize().width, layer->getContentSize().height };
    auto local = CCScene::get()->convertToNodeSpace(worldPos);
    return rect.containsPoint(local);
}
}

Zoom& Zoom::get() {
    static Zoom instance;
    return instance;
}

void Zoom::doZoom(float amount) {
    auto popup = CCScene::get()->getChildByID("ThumbnailPopup");
    if (!popup) {
        return;
    }

    auto popupLayer = popup->getChildByType<CCLayer>(0);
    auto thumbnail = popup->getChildByIDRecursive("thumbnail");
    if (!popupLayer || !thumbnail) {
        return;
    }

    auto mousePos = getMousePos();
    if (!isInsideLayer(popupLayer, mousePos)) {
        return;
    }

    auto zoomDelta = 0.1f * (amount > 0 ? -1.f : 1.f);
    auto oldScale = thumbnail->getScale();
    if (auto val = typeinfo_cast<CCFloat*>(thumbnail->getUserObject("new-scale"))) {
        oldScale = val->getValue();
    }

    auto baseScaleObj = typeinfo_cast<CCFloat*>(thumbnail->getUserObject("scale"));
    auto baseScale = baseScaleObj ? baseScaleObj->getValue() : oldScale;

    float newScale = oldScale;
    if (zoomDelta < 0.f) {
        newScale = oldScale / (1.f - zoomDelta);
    } else {
        newScale = oldScale * (1.f + zoomDelta);
    }

    newScale = std::clamp(newScale, baseScale * 0.75f, baseScale * 100.f);

    auto currentScale = thumbnail->getScale();
    auto previousPos = thumbnail->convertToNodeSpace(mousePos);
    thumbnail->setScale(newScale);
    auto newPos = thumbnail->convertToWorldSpace(previousPos);
    thumbnail->setScale(currentScale);
    thumbnail->setUserObject("new-scale", CCFloat::create(newScale));

    thumbnail->stopAllActions();
    thumbnail->runAction(CCMoveTo::create(0.1f, thumbnail->getPosition() + mousePos - newPos));
    thumbnail->runAction(CCScaleTo::create(0.1f, newScale));
}

void Zoom::update(float dt) {
    auto popup = CCScene::get()->getChildByID("ThumbnailPopup");
    if (!popup) {
        return;
    }

    auto popupLayer = popup->getChildByType<CCLayer>(0);
    auto thumbnail = popup->getChildByIDRecursive("thumbnail");
    if (!popupLayer || !thumbnail) {
        return;
    }

    auto mousePos = getMousePos();
    auto last = m_lastMousePos;
    m_deltaMousePos = { mousePos.x - last.x, mousePos.y - last.y };
    m_lastMousePos = mousePos;

    if (!isInsideLayer(popupLayer, mousePos) && !m_isDragging) {
        return;
    }

    if (m_isTouching) {
        m_isDragging = true;
        auto delta = popup->convertToNodeSpace(m_deltaMousePos);
        thumbnail->setPosition(thumbnail->getPosition() + delta);
    } else {
        m_isDragging = false;
    }
}

class $modify(ZoomScheduler, CCScheduler) {
    $override void update(float dt) {
        Zoom::get().update(dt);
        CCScheduler::update(dt);
    }
};

class $modify(ZoomMouseDispatcher, CCMouseDispatcher) {
    $override bool dispatchScrollMSG(float y, float x) {
        Zoom::get().doZoom(y);
        return CCMouseDispatcher::dispatchScrollMSG(y, x);
    }
};

class $modify(ZoomEGLView, CCEGLView) {
    $override void onGLFWMouseCallBack(GLFWwindow* window, int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                Zoom::get().m_isTouching = true;
            } else if (action == GLFW_RELEASE) {
                Zoom::get().m_isTouching = false;
            }
        }
        CCEGLView::onGLFWMouseCallBack(window, button, action, mods);
    }
};

#endif
