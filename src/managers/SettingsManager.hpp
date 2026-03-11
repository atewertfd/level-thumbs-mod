#pragma once

#include <string>
#include <string_view>

#include <Geode/Geode.hpp>

class Settings {
public:
    static bool levelCellThumbnailsEnabled();
    static bool showThumbnailButton();
    static bool listsLimitEnabled();
    static int64_t levelListsLimit();
    static bool showDownloadProgressText();
    static bool thumbnailTakingEnabled();

    static int64_t thumbnailCacheLimit();
    static int64_t thumbnailFileCacheLimit();
    static int64_t thumbnailFileCacheTTLHours();
    static int64_t thumbnailRequestTimeoutSeconds();
    static int64_t thumbnailRetryCount();
    static int64_t thumbnailRetryBackoffMs();
    static int64_t maxConcurrentDownloads();
    static std::string thumbnailBaseUrl();
    static bool isLegacyAPI();

    static float thumbnailSkewAngle();
    static uint8_t thumbnailOpacity();
    static uint8_t thumbnailSeparatorOpacity();

    static bool showLevelBackground();
    static bool levelBackgroundHighQuality();
    static uint8_t levelBackgroundDarkening();
    static bool levelBackgroundContainMode();
    static bool blurEnabled();
};
