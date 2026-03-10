#include "SettingsManager.hpp"

#include <algorithm>

using namespace geode::prelude;

bool Settings::showThumbnailButton() {
    static bool value = (
        listenForSettingChanges<bool>("thumbnailButton", [](bool val) {
            value = val;
        }),
        getMod()->getSettingValue<bool>("thumbnailButton")
    );
    return value;
}

bool Settings::listsLimitEnabled() {
    static bool value = (
        listenForSettingChanges<bool>("lists-limit-enabled", [](bool val) {
            value = val;
        }),
        getMod()->getSettingValue<bool>("lists-limit-enabled")
    );
    return value;
}

int64_t Settings::levelListsLimit() {
    static int64_t value = (
        listenForSettingChanges<int64_t>("level-lists-limit", [](int64_t val) {
            value = val;
        }),
        getMod()->getSettingValue<int64_t>("level-lists-limit")
    );
    return value;
}

bool Settings::thumbnailTakingEnabled() {
    static bool value = (
        listenForSettingChanges<bool>("enable-thumbnail-taking", [](bool val) {
            value = val;
        }),
        getMod()->getSettingValue<bool>("enable-thumbnail-taking")
    );
    return value;
}

int64_t Settings::thumbnailCacheLimit() {
    static int64_t value = (
        listenForSettingChanges<int64_t>("cache-limit", [](int64_t val) {
            value = val;
        }),
        getMod()->getSettingValue<int64_t>("cache-limit")
    );
    return value;
}

std::string Settings::thumbnailBaseUrl() {
    static std::string value = (
        listenForSettingChanges<std::string>("level-thumbnails-url-new", [](std::string val) {
            value = std::move(val);
        }),
        getMod()->getSettingValue<std::string>("level-thumbnails-url-new")
    );
    static std::string legacyValue = (
        listenForSettingChanges<std::string>("level-thumbnails-url", [](std::string val) {
            legacyValue = std::move(val);
        }),
        getMod()->getSettingValue<std::string>("level-thumbnails-url")
    );
    if (isLegacyAPI()) {
        return legacyValue;
    }
    return value;
}

bool Settings::isLegacyAPI() {
    static bool value = (
        listenForSettingChanges<bool>("legacy-url", [](bool val) {
            value = val;
        }),
        getMod()->getSettingValue<bool>("legacy-url")
    );
    return value;
}

bool Settings::showLevelBackground() {
    static bool value = (
        listenForSettingChanges<bool>("level-bg", [](bool val) {
            value = val;
        }),
        getMod()->getSettingValue<bool>("level-bg")
    );
    return value;
}

uint8_t Settings::levelBackgroundDarkening() {
    static uint8_t value = (
        listenForSettingChanges<int64_t>("darkening", [](int64_t val) {
            value = static_cast<uint8_t>(std::clamp<int64_t>(val, 1, 255));
        }),
        static_cast<uint8_t>(std::clamp<int64_t>(getMod()->getSettingValue<int64_t>("darkening"), 1, 255))
    );
    return value;
}

bool Settings::blurEnabled() {
    static bool value = (
        listenForSettingChanges<bool>("enable-blur", [](bool val) {
            value = val;
        }),
        getMod()->getSettingValue<bool>("enable-blur")
    );
    return value;
}
