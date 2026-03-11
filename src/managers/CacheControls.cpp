#include <Geode/Geode.hpp>

#include "ThumbnailManager.hpp"

using namespace geode::prelude;

namespace {
void onClearMemoryCacheSetting(bool value) {
    if (!value) {
        return;
    }

    ThumbnailManager::get().clearMemoryCache();
    Notification::create("Thumbnail memory cache cleared.", nullptr)->show();
    Mod::get()->setSettingValue<bool>("clear-memory-cache-now", false);
}

void onClearDiskCacheSetting(bool value) {
    if (!value) {
        return;
    }

    ThumbnailManager::get().clearDiskCache();
    Notification::create("Thumbnail disk cache cleared.", nullptr)->show();
    Mod::get()->setSettingValue<bool>("clear-disk-cache-now", false);
}

void onDiskCachePolicyChanged(int64_t) {
    ThumbnailManager::get().trimDiskCache();
}
}

$execute {
    (void) listenForSettingChanges<bool>("clear-memory-cache-now", &onClearMemoryCacheSetting);
    (void) listenForSettingChanges<bool>("clear-disk-cache-now", &onClearDiskCacheSetting);
    (void) listenForSettingChanges<int64_t>("file-cache-limit", &onDiskCachePolicyChanged);
    (void) listenForSettingChanges<int64_t>("file-cache-ttl-hours", &onDiskCachePolicyChanged);
}
