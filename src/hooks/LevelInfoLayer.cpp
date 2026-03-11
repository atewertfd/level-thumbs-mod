#include <algorithm>

#include <Geode/modify/LevelInfoLayer.hpp>

#include "../layers/ThumbnailPopup.hpp"
#include "../managers/SettingsManager.hpp"
#include "../managers/ThumbnailManager.hpp"

using namespace geode::prelude;

class $modify(ThumbnailLevelInfoLayer, LevelInfoLayer) {
    struct Fields {
        TaskHolder<ThumbnailManager::FetchResult> backgroundTask;
        uint64_t backgroundRequestToken = 0;
    };

    ThumbnailManager::Quality preferredBackgroundQuality() const {
        return Settings::levelBackgroundHighQuality()
            ? ThumbnailManager::Quality::High
            : ThumbnailManager::Quality::Low;
    }

    void applyBackgroundTexture(Ref<CCTexture2D> const& texture) {
        auto bg = this->getChildByID("background");
        if (bg) {
            bg->setVisible(false);
        }

        if (auto existing = this->getChildByID("level-thumbnail-background"_spr)) {
            existing->removeFromParent();
        }

        auto winSize = CCDirector::get()->getWinSize();
        auto sprite = CCSprite::createWithTexture(texture);
        if (!sprite) {
            return;
        }

        auto darkening = static_cast<GLubyte>(255 - Settings::levelBackgroundDarkening());
        sprite->setPosition({winSize.width / 2.f, winSize.height / 2.f});
        auto coverScale = std::max(
            winSize.width / sprite->getContentWidth(),
            winSize.height / sprite->getContentHeight()
        );
        auto containScale = std::min(
            winSize.width / sprite->getContentWidth(),
            winSize.height / sprite->getContentHeight()
        );
        sprite->setScale(Settings::levelBackgroundContainMode() ? containScale : coverScale);
        sprite->setColor({darkening, darkening, darkening});
        sprite->setZOrder(bg ? bg->getZOrder() : -1);
        sprite->setID("level-thumbnail-background"_spr);
        this->addChild(sprite);
    }

    void applyBlurFallbackNotice() {
        if (!Settings::blurEnabled()) {
            return;
        }

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
        auto preferredQuality = preferredBackgroundQuality();
        auto fallbackQuality = preferredQuality == ThumbnailManager::Quality::High
            ? ThumbnailManager::Quality::Low
            : ThumbnailManager::Quality::High;

        auto cached = ThumbnailManager::get().getThumbnail(levelID, preferredQuality);
        if (!cached) {
            cached = ThumbnailManager::get().getThumbnail(levelID, fallbackQuality);
        }
        if (cached) {
            applyBackgroundTexture(cached.value());
        } else {
            m_fields->backgroundRequestToken++;
            auto requestToken = m_fields->backgroundRequestToken;

            m_fields->backgroundTask.spawn(
                ThumbnailManager::get().fetchThumbnail(levelID, preferredQuality),
                [self = WeakRef(this), requestToken, levelID](ThumbnailManager::FetchResult result) {
                    if (auto layer = self.lock()) {
                        if (layer->m_fields->backgroundRequestToken != requestToken) {
                            return;
                        }
                        if (!layer->m_level || static_cast<int32_t>(layer->m_level->m_levelID.value()) != levelID) {
                            return;
                        }
                        if (result.isOk()) {
                            layer->applyBackgroundTexture(std::move(result).unwrap());
                        }
                    }
                }
            );
        }

        applyBlurFallbackNotice();

        return true;
    }
};
