#include <algorithm>

#include <Geode/modify/LevelInfoLayer.hpp>

#include "../layers/ThumbnailPopup.hpp"
#include "../managers/SettingsManager.hpp"
#include "../managers/ThumbnailManager.hpp"

using namespace geode::prelude;

class $modify(ThumbnailLevelInfoLayer, LevelInfoLayer) {
    void onThumbnailButton(CCObject*) {
        ThumbnailPopup::create(static_cast<int32_t>(m_level->m_levelID.value()))->show();
    }

    $override bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) {
            return false;
        }

        if (auto menu = getChildByID("left-side-menu"); menu && Settings::showThumbnailButton()) {
            auto sprite = CCSprite::create("thumbnailButton.png"_spr);
            auto button = CCMenuItemSpriteExtra::create(
                sprite,
                this,
                menu_selector(ThumbnailLevelInfoLayer::onThumbnailButton)
            );
            button->setID("thumbnail-button"_spr);
            menu->addChild(button);
            menu->updateLayout();
        }

        if (!Settings::showLevelBackground()) {
            return true;
        }

        auto levelID = static_cast<int32_t>(m_level->m_levelID.value());
        auto cached = ThumbnailManager::get().getThumbnail(levelID);
        if (!cached) {
            return true;
        }

        auto texture = cached.value();
        auto bg = this->getChildByID("background");
        if (bg) {
            bg->setVisible(false);
        }

        auto winSize = CCDirector::get()->getWinSize();
        auto sprite = CCSprite::createWithTexture(texture);
        if (!sprite) {
            return true;
        }

        auto darkening = static_cast<GLubyte>(255 - Settings::levelBackgroundDarkening());
        sprite->setPosition({winSize.width / 2.f, winSize.height / 2.f});
        sprite->setScale(std::max(
            winSize.width / sprite->getContentWidth(),
            winSize.height / sprite->getContentHeight()
        ));
        sprite->setColor({darkening, darkening, darkening});
        sprite->setZOrder(bg ? bg->getZOrder() : -1);
        sprite->setID("level-thumbnail-background"_spr);
        this->addChild(sprite);

        if (Settings::blurEnabled()) {
            auto alert = FLAlertLayer::create(
                nullptr,
                "Coming Soon",
                "<cj>Blur</c> currently is not implemented in this port yet.\nIt has been disabled.",
                "OK",
                nullptr
            );
            alert->m_scene = this;
            alert->show();
            Mod::get()->setSettingValue<bool>("enable-blur", false);
        }

        return true;
    }
};
