#pragma once

#include <unordered_set>

#include <Geode/Geode.hpp>

#include "../managers/ThumbnailManager.hpp"

using namespace geode::prelude;

class ThumbnailPopup : public Popup {
protected:
    std::unordered_set<Ref<CCTouch>> m_touches;
    float m_initialDistance = 0.f;
    float m_initialScale = 0.f;
    bool m_wasZooming = false;
    CCPoint m_touchMidPoint {};

    int m_levelID = 0;
    bool m_isScreenshotPreview = false;
    float m_maxHeight = 220.f;

    TaskHolder<ThumbnailManager::FetchResult> m_downloadTask;

    LoadingCircle* m_loadingCircle = nullptr;
    CCMenuItemSpriteExtra* m_downloadBtn = nullptr;
    CCMenuItemSpriteExtra* m_infoBtn = nullptr;
    CCClippingNode* m_clippingNode = nullptr;
    CCLabelBMFont* m_funnyLabel = nullptr;

    bool init(int id);
    void onDownloadSuccess(Ref<CCTexture2D> const& texture);
    void onDownloadFailed();

    void onDownload(CCObject*);
    void onOpenFolder(CCObject*);
    void openDiscordPopup(CCObject*);
    void recenter(CCObject*);

    bool ccTouchBegan(CCTouch* touch, CCEvent* event) override;
    void ccTouchMoved(CCTouch* touch, CCEvent* event) override;
    void ccTouchEnded(CCTouch* touch, CCEvent* event) override;

public:
    static ThumbnailPopup* create(int id, bool screenshotPreview = false);
};
