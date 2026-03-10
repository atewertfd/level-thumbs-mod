#if !defined(GEODE_IS_MACOS) && !defined(GEODE_IS_IOS)

#include <vector>

#include <Geode/modify/PauseLayer.hpp>

#include "../layers/ThumbnailPopup.hpp"
#include "../managers/SettingsManager.hpp"

using namespace geode::prelude;

class $modify(ThumbnailPauseLayer, PauseLayer) {
    struct Fields {
        std::vector<CCNode*> shiftedNodes;
        CCPoint oldScenePos {};
        float oldSceneScale = 1.f;
        bool uiVisible = true;
    };

    void prepareScreenshotScene(CCScene* scene, PlayLayer* playLayer) {
        auto fields = m_fields.self();
        fields->shiftedNodes.clear();
        fields->oldScenePos = scene->getPosition();
        fields->oldSceneScale = scene->getScale();

        auto sceneContentSize = scene->getContentSize();
        auto scaleFactor = CCDirector::get()->getContentScaleFactor();
        scene->setAnchorPoint({0.f, 0.f});
        scene->setScale((1080.f / sceneContentSize.height) / scaleFactor);

        auto scaledSize = scene->getScaledContentSize();
        scene->setPosition(((1920.f - (scaledSize.width * scaleFactor)) / 2.f / scaleFactor), 0.f);

        if (auto uiLayer = playLayer->getChildByType<UILayer>(0)) {
            fields->uiVisible = uiLayer->isVisible();
            uiLayer->setVisible(false);
        }

        for (auto* obj : CCArrayExt<CCNode*>(playLayer->getChildren())) {
            if (obj->getID() == "main-node" || obj == playLayer->m_shaderLayer) {
                continue;
            }
            fields->shiftedNodes.push_back(obj);
            obj->setPosition(obj->getPosition() + ccp(10000.f, 10000.f));
        }
    }

    void restoreScreenshotScene(CCScene* scene, PlayLayer* playLayer) {
        auto fields = m_fields.self();
        scene->setScale(fields->oldSceneScale);
        scene->setPosition(fields->oldScenePos);

        if (auto uiLayer = playLayer->getChildByType<UILayer>(0)) {
            uiLayer->setVisible(fields->uiVisible);
        }

        for (auto* obj : fields->shiftedNodes) {
            if (obj) {
                obj->setPosition(obj->getPosition() - ccp(10000.f, 10000.f));
            }
        }
        fields->shiftedNodes.clear();
    }

    void doScreenshot() {
        auto playLayer = PlayLayer::get();
        if (!playLayer) {
            return;
        }

        auto scene = CCDirector::sharedDirector()->getRunningScene();
        if (!scene) {
            return;
        }

        auto scaleFactor = CCDirector::get()->getContentScaleFactor();
        auto renderTexture = CCRenderTexture::create(1920 / scaleFactor, 1080 / scaleFactor);
        if (!renderTexture) {
            return;
        }

        this->setVisible(false);
        prepareScreenshotScene(scene, playLayer);

        renderTexture->begin();
        scene->visit();
        renderTexture->end();

        restoreScreenshotScene(scene, playLayer);
        this->setVisible(true);

        auto image = renderTexture->newCCImage();
        if (!image) {
            return;
        }

        auto levelID = static_cast<int32_t>(playLayer->m_level->m_levelID.value());
        auto path = fmt::format("{}/{}.png", Mod::get()->getSaveDir(), levelID);
        image->saveToFile(path.c_str());
        image->release();

        ThumbnailPopup::create(levelID, true)->show();
    }

    void onScreenshot(CCObject*) {
        auto playLayer = PlayLayer::get();
        if (!playLayer || !playLayer->m_level) {
            return;
        }

        if (playLayer->m_shaderLayer && playLayer->m_shaderLayer->getParent()) {
            FLAlertLayer::create(
                "Oops!",
                "Taking a thumbnail while a <cy>shader</c> is present on screen is <cr>not yet supported.</c>",
                "OK"
            )->show();
            return;
        }

        doScreenshot();
    }

    $override void customSetup() {
        PauseLayer::customSetup();

        if (!Settings::thumbnailTakingEnabled()) {
            return;
        }

        auto playLayer = PlayLayer::get();
        if (!playLayer || !playLayer->m_level || playLayer->m_level->m_levelID.value() <= 0) {
            return;
        }

        auto menu = this->getChildByID("right-button-menu");
        if (!menu) {
            return;
        }

        auto sprite = CCSprite::create("thumbnailButton.png"_spr);
        sprite->setScale(0.6f);
        auto button = CCMenuItemSpriteExtra::create(sprite, this, menu_selector(ThumbnailPauseLayer::onScreenshot));
        menu->addChild(button);
        menu->updateLayout();
    }
};

#endif
