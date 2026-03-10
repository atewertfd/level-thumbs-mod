#include "ThumbnailPopup.hpp"

#include <algorithm>

#include "../managers/ThumbnailManager.hpp"

using namespace geode::prelude;

namespace {
float clipFloat(float n, float lower, float upper) {
    return std::max(lower, std::min(n, upper));
}
}

void ThumbnailPopup::onDownload(CCObject*) {
    CCApplication::sharedApplication()->openURL(ThumbnailManager::buildDirectImageUrl(m_levelID).c_str());
}

void ThumbnailPopup::onOpenFolder(CCObject*) {
    file::openFolder(Mod::get()->getSaveDir());
    clipboard::write(fmt::to_string(m_levelID));
    Notification::create("Copied ID to clipboard.", nullptr)->show();
}

void ThumbnailPopup::openDiscordPopup(CCObject*) {
    if (m_isScreenshotPreview) {
        createQuickPopup(
            "Submit",
            "To <cy>submit a thumbnail</c> you need to join the <cb>Discord</c> server.\nDo you want to <cg>join</c>?",
            "No Thanks",
            "JOIN!",
            [](auto, bool yes) {
                if (yes) {
                    CCApplication::sharedApplication()->openURL("https://discord.gg/GuagJDsqds");
                }
            }
        );
        return;
    }

    createQuickPopup(
        "Uh Oh!",
        "This level seems to not have a <cj>Thumbnail</c>...\n"
        "You can join our <cg>Discord Server</c> and submit one yourself.",
        "No Thanks",
        "JOIN!",
        [](auto, bool yes) {
            if (yes) {
                CCApplication::sharedApplication()->openURL("https://discord.gg/GuagJDsqds");
            }
        }
    );
}

bool ThumbnailPopup::init(int id) {
    if (!Popup::init(395.f, 225.f, "GJ_square05.png")) {
        return false;
    }

    m_levelID = id;
    m_noElasticity = false;
    this->setID("ThumbnailPopup");
    this->setTitle("");

    auto border = NineSlice::create("GJ_square07.png");
    border->setContentSize(m_bgSprite->getContentSize());
    border->setPosition(m_bgSprite->getPosition());
    border->setZOrder(2);
    m_mainLayer->addChild(border);

    auto mask = CCLayerColor::create({255, 255, 255});
    mask->setContentSize({391.f, 220.f});
    mask->setPosition({
        m_bgSprite->getContentSize().width / 2.f - 195.5f,
        m_bgSprite->getContentSize().height / 2.f - 110.f
    });

    m_bgSprite->setColor({50, 50, 50});

    m_clippingNode = CCClippingNode::create();
    m_clippingNode->setContentSize(m_bgSprite->getContentSize());
    m_clippingNode->setStencil(mask);
    m_clippingNode->setZOrder(1);
    m_mainLayer->addChild(m_clippingNode);

    auto downloadSprite = CCSprite::createWithSpriteFrameName("GJ_downloadBtn_001.png");
    m_downloadBtn = CCMenuItemSpriteExtra::create(downloadSprite, this, menu_selector(ThumbnailPopup::onDownload));
    m_downloadBtn->setEnabled(true);
    m_downloadBtn->setVisible(!m_isScreenshotPreview);
    m_downloadBtn->setColor({125, 125, 125});
    m_downloadBtn->setPosition({m_mainLayer->getContentSize().width - 5.f, 5.f});
    m_buttonMenu->addChild(m_downloadBtn);

    auto recenterSprite = CCSprite::createWithSpriteFrameName("GJ_undoBtn_001.png");
    auto recenterBtn = CCMenuItemSpriteExtra::create(recenterSprite, this, menu_selector(ThumbnailPopup::recenter));
    recenterBtn->setPosition({5.f, 5.f});
    m_buttonMenu->addChild(recenterBtn);

#ifdef GEODE_IS_MACOS
    recenterBtn->setVisible(false);
#endif

    auto infoSprite = ButtonSprite::create(m_isScreenshotPreview ? "Submit" : "What's this?");
    m_infoBtn = CCMenuItemSpriteExtra::create(infoSprite, this, menu_selector(ThumbnailPopup::openDiscordPopup));
    m_infoBtn->setPosition({m_isScreenshotPreview ? 293.f : m_mainLayer->getContentSize().width / 2.f, 6.f});
    m_infoBtn->setVisible(m_isScreenshotPreview);
    m_infoBtn->setZOrder(3);
    m_buttonMenu->addChild(m_infoBtn);

    if (m_isScreenshotPreview) {
        auto openFolderSprite = ButtonSprite::create("Open Folder");
        auto openFolderBtn = CCMenuItemSpriteExtra::create(
            openFolderSprite,
            this,
            menu_selector(ThumbnailPopup::onOpenFolder)
        );
        openFolderBtn->setPosition({132.f, 6.f});
        openFolderBtn->setZOrder(3);
        m_buttonMenu->addChild(openFolderBtn);
    }

    m_funnyLabel = CCLabelBMFont::create("OwO", "bigFont.fnt");
    m_funnyLabel->setPosition(m_bgSprite->getPosition());
    m_funnyLabel->setVisible(m_isScreenshotPreview);
    m_funnyLabel->setScale(0.25f);
    m_mainLayer->addChild(m_funnyLabel);

    m_loadingCircle = LoadingCircle::create();
    m_loadingCircle->setParentLayer(m_mainLayer);
    m_loadingCircle->setPosition({m_mainLayer->getContentWidth() / 2.f, m_mainLayer->getContentHeight() / 2.f});
    m_loadingCircle->setScale(1.f);
    m_loadingCircle->show();

    if (m_isScreenshotPreview) {
        auto path = fmt::format("{}/{}.png", Mod::get()->getSaveDir(), m_levelID);
        CCTextureCache::get()->removeTextureForKey(path.c_str());
        if (auto sprite = CCSprite::create(path.c_str())) {
            m_loadingCircle->fadeAndRemove();
            auto texture = Ref<CCTexture2D>(sprite->getTexture());
            onDownloadSuccess(texture);
        } else {
            onDownloadFailed();
        }
    } else {
        if (auto cached = ThumbnailManager::get().getThumbnail(m_levelID)) {
            m_loadingCircle->fadeAndRemove();
            onDownloadSuccess(cached.value());
        } else {
            m_downloadTask.spawn(
                ThumbnailManager::get().fetchThumbnail(m_levelID),
                [self = WeakRef(this)](ThumbnailManager::FetchResult result) {
                    if (auto popup = self.lock()) {
                        if (result.isOk()) {
                            popup->onDownloadSuccess(std::move(result).unwrap());
                        } else {
                            popup->onDownloadFailed();
                        }
                    }
                }
            );
        }
    }

    this->setTouchEnabled(true);
    return true;
}

void ThumbnailPopup::recenter(CCObject*) {
    auto node = m_clippingNode ? m_clippingNode->getChildByID("thumbnail") : nullptr;
    if (!node) {
        return;
    }

    node->setPosition({m_mainLayer->getContentWidth() / 2.f, m_mainLayer->getContentHeight() / 2.f});
    node->stopAllActions();
    auto scale = m_maxHeight / node->getContentSize().height;
    node->setUserObject("new-scale", CCFloat::create(scale));
    node->setScale(scale);
    node->setAnchorPoint({0.5f, 0.5f});
}

void ThumbnailPopup::onDownloadSuccess(Ref<CCTexture2D> const& texture) {
    m_downloadBtn->setEnabled(true);
    m_downloadBtn->setColor({255, 255, 255});

    auto image = CCSprite::createWithTexture(texture);
    if (!image) {
        onDownloadFailed();
        return;
    }

    auto scale = m_maxHeight / image->getContentSize().height;
    image->setScale(scale);
    image->setUserObject("scale", CCFloat::create(scale));
    image->setPosition({m_mainLayer->getContentWidth() / 2.f, m_mainLayer->getContentHeight() / 2.f});
    image->setID("thumbnail");
    m_clippingNode->addChild(image);

    if (m_loadingCircle) {
        m_loadingCircle->fadeAndRemove();
    }
}

void ThumbnailPopup::onDownloadFailed() {
    auto image = CCSprite::create("noThumb.png"_spr);
    auto scale = m_maxHeight / image->getContentSize().height;
    image->setScale(scale);
    image->setUserObject("scale", CCFloat::create(scale));
    image->setPosition({m_mainLayer->getContentWidth() / 2.f, m_mainLayer->getContentHeight() / 2.f});
    image->setID("thumbnail");
    m_clippingNode->addChild(image);

    if (m_infoBtn) {
        m_infoBtn->setVisible(true);
    }
    if (m_funnyLabel) {
        m_funnyLabel->setVisible(true);
    }
    if (m_loadingCircle) {
        m_loadingCircle->fadeAndRemove();
    }
}

ThumbnailPopup* ThumbnailPopup::create(int id, bool screenshotPreview) {
    auto ret = new ThumbnailPopup();
    ret->m_isScreenshotPreview = screenshotPreview;
    if (ret->init(id)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool ThumbnailPopup::ccTouchBegan(CCTouch* touch, CCEvent* event) {
    if (m_touches.size() == 1) {
        auto firstTouch = *m_touches.begin();
        auto firstLoc = firstTouch->getLocation();
        auto secondLoc = touch->getLocation();

        m_touchMidPoint = (firstLoc + secondLoc) / 2.f;

        auto thumbnail = this->getChildByIDRecursive("thumbnail");
        if (!thumbnail) {
            return Popup::ccTouchBegan(touch, event);
        }

        m_initialScale = thumbnail->getScale();
        m_initialDistance = firstLoc.getDistance(secondLoc);

        auto oldAnchor = thumbnail->getAnchorPoint();
        auto worldPos = thumbnail->convertToWorldSpace({0, 0});
        auto newAnchorX = (m_touchMidPoint.x - worldPos.x) / thumbnail->getScaledContentWidth();
        auto newAnchorY = (m_touchMidPoint.y - worldPos.y) / thumbnail->getScaledContentHeight();
        auto clampedAnchor = CCPoint { clipFloat(newAnchorX, 0.f, 1.f), clipFloat(newAnchorY, 0.f, 1.f) };

        thumbnail->setAnchorPoint(clampedAnchor);
        thumbnail->setPosition({
            thumbnail->getPositionX() + thumbnail->getScaledContentWidth() * -(oldAnchor.x - clampedAnchor.x),
            thumbnail->getPositionY() + thumbnail->getScaledContentHeight() * -(oldAnchor.y - clampedAnchor.y)
        });
    }

    m_touches.insert(touch);
    return Popup::ccTouchBegan(touch, event);
}

void ThumbnailPopup::ccTouchMoved(CCTouch* touch, CCEvent* event) {
#ifndef GEODE_IS_WINDOWS
    if (m_touches.size() == 1) {
        if (auto thumbnail = this->getChildByIDRecursive("thumbnail")) {
            thumbnail->setPosition(thumbnail->getPosition() + touch->getDelta());
        }
    }
#endif

    if (m_touches.size() == 2) {
        m_wasZooming = true;
        auto thumbnail = this->getChildByIDRecursive("thumbnail");
        if (!thumbnail) {
            return Popup::ccTouchMoved(touch, event);
        }

        auto it = m_touches.begin();
        auto firstTouch = *it;
        ++it;
        auto secondTouch = *it;

        auto firstLoc = firstTouch->getLocation();
        auto secondLoc = secondTouch->getLocation();
        auto center = (firstLoc + secondLoc) / 2.f;
        auto distNow = firstLoc.getDistance(secondLoc);
        if (distNow <= 0.01f) {
            return Popup::ccTouchMoved(touch, event);
        }

        auto zoom = clipFloat(m_initialScale / (m_initialDistance / distNow), 0.2f, 6.5f);
        thumbnail->setScale(zoom);

        auto centerDiff = m_touchMidPoint - center;
        thumbnail->setPosition(thumbnail->getPosition() - centerDiff);
        m_touchMidPoint = center;
    }

    Popup::ccTouchMoved(touch, event);
}

void ThumbnailPopup::ccTouchEnded(CCTouch* touch, CCEvent* event) {
    m_touches.erase(touch);
    if (m_wasZooming && m_touches.size() == 1) {
        if (auto thumbnail = this->getChildByIDRecursive("thumbnail")) {
            auto scale = thumbnail->getScale();
            if (scale < 0.25f) {
                thumbnail->runAction(CCEaseSineInOut::create(CCScaleTo::create(0.5f, 0.25f)));
            }
            if (scale > 4.0f) {
                thumbnail->runAction(CCEaseSineInOut::create(CCScaleTo::create(0.5f, 4.0f)));
            }
        }
        m_wasZooming = false;
    }

    Popup::ccTouchEnded(touch, event);
}
