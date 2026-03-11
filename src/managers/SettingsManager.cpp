#include "SettingsManager.hpp"

#include <algorithm>

using namespace geode::prelude;

bool Settings::levelCellThumbnailsEnabled() {
    static bool value = (
        listenForSettingChanges<bool>("level-cell-thumbnails", [](bool val) {
            value = val;
        }),
        getMod()->getSettingValue<bool>("level-cell-thumbnails")
    );
    return value;
}

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

bool Settings::showDownloadProgressText() {
    static bool value = (
        listenForSettingChanges<bool>("show-download-progress", [](bool val) {
            value = val;
        }),
        getMod()->getSettingValue<bool>("show-download-progress")
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

int64_t Settings::thumbnailFileCacheLimit() {
    static int64_t value = (
        listenForSettingChanges<int64_t>("file-cache-limit", [](int64_t val) {
            value = std::clamp<int64_t>(val, -1, 2000);
        }),
        std::clamp<int64_t>(getMod()->getSettingValue<int64_t>("file-cache-limit"), -1, 2000)
    );
    return value;
}

int64_t Settings::thumbnailFileCacheTTLHours() {
    static int64_t value = (
        listenForSettingChanges<int64_t>("file-cache-ttl-hours", [](int64_t val) {
            value = std::clamp<int64_t>(val, 1, 720);
        }),
        std::clamp<int64_t>(getMod()->getSettingValue<int64_t>("file-cache-ttl-hours"), 1, 720)
    );
    return value;
}

int64_t Settings::thumbnailRequestTimeoutSeconds() {
    static int64_t value = (
        listenForSettingChanges<int64_t>("request-timeout-seconds", [](int64_t val) {
            value = std::clamp<int64_t>(val, 3, 120);
        }),
        std::clamp<int64_t>(getMod()->getSettingValue<int64_t>("request-timeout-seconds"), 3, 120)
    );
    return value;
}

int64_t Settings::thumbnailRetryCount() {
    static int64_t value = (
        listenForSettingChanges<int64_t>("request-retries", [](int64_t val) {
            value = std::clamp<int64_t>(val, 0, 6);
        }),
        std::clamp<int64_t>(getMod()->getSettingValue<int64_t>("request-retries"), 0, 6)
    );
    return value;
}

int64_t Settings::thumbnailRetryBackoffMs() {
    static int64_t value = (
        listenForSettingChanges<int64_t>("retry-backoff-ms", [](int64_t val) {
            value = std::clamp<int64_t>(val, 50, 5000);
        }),
        std::clamp<int64_t>(getMod()->getSettingValue<int64_t>("retry-backoff-ms"), 50, 5000)
    );
    return value;
}

int64_t Settings::maxConcurrentDownloads() {
    static int64_t value = (
        listenForSettingChanges<int64_t>("max-concurrent-downloads", [](int64_t val) {
            value = std::clamp<int64_t>(val, 1, 12);
        }),
        std::clamp<int64_t>(getMod()->getSettingValue<int64_t>("max-concurrent-downloads"), 1, 12)
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

float Settings::thumbnailSkewAngle() {
    static float value = (
        listenForSettingChanges<int64_t>("thumbnail-skew-angle", [](int64_t val) {
            value = static_cast<float>(std::clamp<int64_t>(val, 0, 35));
        }),
        static_cast<float>(std::clamp<int64_t>(getMod()->getSettingValue<int64_t>("thumbnail-skew-angle"), 0, 35))
    );
    return value;
}

uint8_t Settings::thumbnailOpacity() {
    static uint8_t value = (
        listenForSettingChanges<int64_t>("thumbnail-opacity", [](int64_t val) {
            value = static_cast<uint8_t>(std::clamp<int64_t>(val, 20, 255));
        }),
        static_cast<uint8_t>(std::clamp<int64_t>(getMod()->getSettingValue<int64_t>("thumbnail-opacity"), 20, 255))
    );
    return value;
}

uint8_t Settings::thumbnailSeparatorOpacity() {
    static uint8_t value = (
        listenForSettingChanges<int64_t>("separator-opacity", [](int64_t val) {
            value = static_cast<uint8_t>(std::clamp<int64_t>(val, 0, 255));
        }),
        static_cast<uint8_t>(std::clamp<int64_t>(getMod()->getSettingValue<int64_t>("separator-opacity"), 0, 255))
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

bool Settings::levelBackgroundHighQuality() {
    static std::string value = (
        listenForSettingChanges<std::string>("level-bg-quality", [](std::string val) {
            value = std::move(val);
        }),
        getMod()->getSettingValue<std::string>("level-bg-quality")
    );
    return value == "High";
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

bool Settings::levelBackgroundContainMode() {
    static std::string value = (
        listenForSettingChanges<std::string>("level-bg-scale-mode", [](std::string val) {
            value = std::move(val);
        }),
        getMod()->getSettingValue<std::string>("level-bg-scale-mode")
    );
    return value == "contain";
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
