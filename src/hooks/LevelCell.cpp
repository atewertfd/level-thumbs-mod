#include <algorithm>
#include <cmath>

#include <Geode/modify/LevelCell.hpp>
#include <Geode/ui/LoadingSpinner.hpp>

#include "../managers/SettingsManager.hpp"
#include "../managers/ThumbnailManager.hpp"

using namespace geode::prelude;

namespace {
int32_t levelIDFrom(GJGameLevel* level) {
    if (!level) {
        return 0;
    }
    return static_cast<int32_t>(level->m_levelID.value());
}
}

class $modify(ThumbnailLevelCell, LevelCell) {
    struct Fields {
        TaskHolder<ThumbnailManager::FetchResult> fetchTask;
        CCLabelBMFont* progressLabel = nullptr;
        LoadingSpinner* spinner = nullptr;
        CCClippingNode* clipNode = nullptr;
        CCLayerColor* separator = nullptr;
        SEL_SCHEDULE parentCheckSchedule = nullptr;
        uint16_t parentCheckTicks = 0;
        uint64_t requestToken = 0;
        int32_t requestedLevelID = 0;
    };

    bool isLikelyVisibleInViewport(float padding = 60.f) {
        auto director = CCDirector::get();
        if (!director) {
            return true;
        }

        auto winSize = director->getWinSize();
        auto worldBL = this->convertToWorldSpace({0.f, 0.f});
        auto worldTR = this->convertToWorldSpace({this->getScaledContentWidth(), this->getScaledContentHeight()});

        auto minX = std::min(worldBL.x, worldTR.x);
        auto minY = std::min(worldBL.y, worldTR.y);
        auto maxX = std::max(worldBL.x, worldTR.x);
        auto maxY = std::max(worldBL.y, worldTR.y);

        auto visibleRect = CCRect {
            -padding,
            -padding,
            winSize.width + (padding * 2.f),
            winSize.height + (padding * 2.f)
        };
        auto cellRect = CCRect {
            minX,
            minY,
            maxX - minX,
            maxY - minY
        };

        return visibleRect.intersectsRect(cellRect);
    }

    CCPoint loadingSpinnerPos() {
        auto fallback = m_compactView ? ccp(272.f, 25.f) : ccp(334.f, 15.f);

        auto mainLayer = this->getChildByID("main-layer");
        if (!mainLayer) {
            return fallback;
        }

        auto mainMenu = mainLayer->getChildByID("main-menu");
        if (!mainMenu) {
            return fallback;
        }

        auto viewButton = mainMenu->getChildByID("view-button");
        if (!viewButton) {
            return fallback;
        }

        return viewButton->getPosition() + ccp(m_compactView ? -42.f : 20.f, m_compactView ? 0.f : -30.f);
    }

    void stopParentCheck() {
        if (m_fields->parentCheckSchedule) {
            this->unschedule(m_fields->parentCheckSchedule);
        }
        m_fields->parentCheckTicks = 0;
    }

    void startParentCheck() {
        if (!m_fields->parentCheckSchedule) {
            m_fields->parentCheckSchedule = schedule_selector(ThumbnailLevelCell::checkParentForDaily);
        }
        stopParentCheck();
        this->schedule(m_fields->parentCheckSchedule);
    }

    void checkParentForDaily(float) {
        if (!m_fields->clipNode || !m_fields->separator) {
            stopParentCheck();
            return;
        }

        if (auto parent = getParent()) {
            if (typeinfo_cast<DailyLevelNode*>(parent)) {
                applyDailyAdjustments();
            }
            stopParentCheck();
            return;
        }

        m_fields->parentCheckTicks++;
        if (m_fields->parentCheckTicks > 180) {
            stopParentCheck();
        }
    }

    void clearLoadingNodes() {
        if (m_fields->progressLabel) {
            m_fields->progressLabel->removeFromParent();
            m_fields->progressLabel = nullptr;
        }
        if (m_fields->spinner) {
            m_fields->spinner->removeFromParent();
            m_fields->spinner = nullptr;
        }
    }

    void clearThumbNodes() {
        if (m_fields->clipNode) {
            m_fields->clipNode->removeFromParent();
            m_fields->clipNode = nullptr;
        }
        if (m_fields->separator) {
            m_fields->separator->removeFromParent();
            m_fields->separator = nullptr;
        }
    }

    void resetNodes() {
        stopParentCheck();
        clearLoadingNodes();
        clearThumbNodes();
    }

    void updateProgress(float progress) {
        if (progress <= 1.f) {
            progress *= 100.f;
        }
        progress = std::clamp(progress, 0.f, 100.f);

        if (Settings::showDownloadProgressText()) {
            if (!m_fields->progressLabel) {
                auto label = CCLabelBMFont::create("0%", "bigFont.fnt");
                label->setPosition({352.f, 1.f});
                label->setAnchorPoint({1.f, 0.f});
                label->setScale(0.25f);
                label->setOpacity(128);
                label->setID("download-progress"_spr);
                this->addChild(label);
                m_fields->progressLabel = label;
            }

            m_fields->progressLabel->setString(fmt::format("{:.0f}%", std::round(progress)).c_str());
        } else if (m_fields->progressLabel) {
            m_fields->progressLabel->removeFromParent();
            m_fields->progressLabel = nullptr;
        }

        if (!m_fields->spinner) {
            auto spinner = LoadingSpinner::create(18.f);
            spinner->setPosition(loadingSpinnerPos());
            spinner->setID("download-spinner"_spr);
            this->addChild(spinner);
            m_fields->spinner = spinner;
        } else {
            m_fields->spinner->setPosition(loadingSpinnerPos());
        }
    }

    void applyDailyAdjustments() {
        auto daily = typeinfo_cast<DailyLevelNode*>(getParent());
        if (!daily || !m_fields->clipNode || !m_fields->separator || !m_backgroundLayer) {
            return;
        }

        constexpr float dailyMult = 1.22f;
        m_fields->separator->setScaleX(0.45f * dailyMult);
        m_fields->separator->setScaleY(dailyMult);
        m_fields->separator->setPosition({
            m_backgroundLayer->getContentWidth() - (m_fields->separator->getContentWidth() * dailyMult) / 2.f - 13.f,
            -7.9f
        });
        m_fields->clipNode->setScale(dailyMult);
        m_fields->clipNode->setPosition(m_fields->clipNode->getPosition().x + 7.f, -7.9f);

        if (auto bg = typeinfo_cast<CCScale9Sprite*>(daily->getChildByID("background"))) {
            auto border = NineSlice::create("GJ_square07.png");
            border->setContentSize(bg->getContentSize());
            border->setPosition(bg->getPosition());
            border->setColor(bg->getColor());
            border->setZOrder(5);
            border->setID("border"_spr);
            daily->addChild(border);
        }

        if (auto crown = daily->getChildByID("crown-sprite")) {
            crown->setZOrder(6);
        }
    }

    void applyThumbnail(Ref<CCTexture2D> const& texture) {
        clearThumbNodes();
        clearLoadingNodes();

        if (!m_backgroundLayer) {
            return;
        }

        m_backgroundLayer->setZOrder(-2);

        auto image = CCSprite::createWithTexture(texture);
        if (!image) {
            return;
        }
        image->setID("thumbnail"_spr);

        auto imgScale = m_backgroundLayer->getContentHeight() / image->getContentHeight();
        image->setScale(imgScale);

        float separatorXMul = 1.f;
        if (m_compactView) {
            image->setScale(imgScale * 1.3f);
            separatorXMul = 0.75f;
        }

        auto angle = Settings::thumbnailSkewAngle();

        auto rect = CCLayerColor::create({255, 255, 255});
        auto scaledImageSize = CCSize { image->getScaledContentWidth(), image->getContentHeight() * imgScale };
        rect->setSkewX(angle);
        rect->setContentSize(scaledImageSize);
        rect->setAnchorPoint({1.f, 0.f});

        auto clipNode = CCClippingNode::create();
        clipNode->setStencil(rect);
        clipNode->addChild(image);
        clipNode->setContentSize(scaledImageSize);
        clipNode->setAnchorPoint({1.f, 0.f});
        clipNode->setPosition({m_backgroundLayer->getContentWidth(), 0.3f});
        clipNode->setID("clip-node"_spr);
        m_fields->clipNode = clipNode;

        image->setOpacity(Settings::thumbnailOpacity());
        image->setPosition(clipNode->getContentSize() * 0.5f);

        auto separator = CCLayerColor::create({0, 0, 0});
        separator->setZOrder(-2);
        separator->setOpacity(Settings::thumbnailSeparatorOpacity());
        separator->setScaleX(0.45f);
        separator->ignoreAnchorPointForPosition(false);
        separator->setSkewX(angle * 2.f);
        separator->setContentSize(scaledImageSize);
        separator->setAnchorPoint({1.f, 0.f});
        separator->setPosition({
            m_backgroundLayer->getContentWidth() - separator->getContentWidth() / 2.f - (20.f * separatorXMul),
            0.3f
        });
        separator->setID("separator"_spr);
        m_fields->separator = separator;

        this->addChild(separator);
        clipNode->setZOrder(-1);
        this->addChild(clipNode);

        applyDailyAdjustments();
        startParentCheck();
    }

    $override void loadCustomLevelCell() {
        LevelCell::loadCustomLevelCell();

        resetNodes();
        m_fields->requestToken++;
        m_fields->requestedLevelID = levelIDFrom(m_level);

        if (!m_level || m_fields->requestedLevelID <= 0) {
            return;
        }

        if (!Settings::levelCellThumbnailsEnabled()) {
            return;
        }

        if (Settings::listsLimitEnabled() && m_level->m_listPosition > Settings::levelListsLimit()) {
            return;
        }

        if (auto cached = ThumbnailManager::get().getThumbnail(m_fields->requestedLevelID)) {
            applyThumbnail(cached.value());
            return;
        }

        if (!isLikelyVisibleInViewport()) {
            return;
        }

        updateProgress(0.f);
        auto token = m_fields->requestToken;
        auto levelID = m_fields->requestedLevelID;

        m_fields->fetchTask.spawn(
            ThumbnailManager::get().fetchThumbnail(
                levelID,
                ThumbnailManager::Quality::High,
                [self = WeakRef(this), token](web::WebProgress const& progress) {
                    if (auto cell = self.lock()) {
                        if (cell->m_fields->requestToken != token) {
                            return;
                        }
                        if (!cell->isLikelyVisibleInViewport()) {
                            return;
                        }
                        cell->updateProgress(progress.downloadProgress().value_or(0.f));
                    }
                }
            ),
            [self = WeakRef(this), token, levelID](ThumbnailManager::FetchResult res) {
                if (auto cell = self.lock()) {
                    if (cell->m_fields->requestToken != token) {
                        return;
                    }
                    if (levelIDFrom(cell->m_level) != levelID) {
                        return;
                    }

                    if (res.isOk()) {
                        if (cell->isLikelyVisibleInViewport()) {
                            cell->applyThumbnail(std::move(res).unwrap());
                        } else {
                            cell->clearLoadingNodes();
                        }
                    } else {
                        cell->clearLoadingNodes();
                    }
                }
            }
        );
    }
};
