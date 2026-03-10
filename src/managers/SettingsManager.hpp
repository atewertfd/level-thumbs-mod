#pragma once

#include <string>
#include <string_view>

#include <Geode/Geode.hpp>

class Settings {
public:
    static bool showThumbnailButton();
    static bool listsLimitEnabled();
    static int64_t levelListsLimit();
    static bool thumbnailTakingEnabled();

    static int64_t thumbnailCacheLimit();
    static std::string thumbnailBaseUrl();
    static bool isLegacyAPI();

    static bool showLevelBackground();
    static uint8_t levelBackgroundDarkening();
    static bool blurEnabled();
};
