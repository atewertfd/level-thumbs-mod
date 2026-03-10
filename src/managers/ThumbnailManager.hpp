#pragma once

#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>

#ifndef MOD_VERSION
#define MOD_VERSION "dev"
#endif

#define LEVEL_THUMBS_USER_AGENT "LevelThumbnails/" GEODE_PLATFORM_NAME "/" MOD_VERSION

class ThumbnailManager {
public:
    using FetchResult = geode::Result<geode::Ref<cocos2d::CCTexture2D>>;
    using FetchFuture = arc::Future<FetchResult>;
    using ProgressCallback = geode::Function<void(geode::utils::web::WebProgress const&)>;

    static ThumbnailManager& get();

    std::optional<geode::Ref<cocos2d::CCTexture2D>> getThumbnail(int32_t levelID);
    FetchFuture fetchThumbnail(int32_t levelID, ProgressCallback progress = nullptr);

    static std::string buildRequestUrl(int32_t levelID);
    static std::string buildDirectImageUrl(int32_t levelID);

private:
    ThumbnailManager() = default;

    struct CacheEntry {
        geode::Ref<cocos2d::CCTexture2D> texture;
        std::chrono::steady_clock::time_point lastAccess;
    };

    static std::string cacheKey(int32_t levelID);
    static cocos2d::CCImage* decodeImage(std::vector<uint8_t> data);
    void evictIfNeededLocked();
    void rememberMissing(std::string const& key);
    bool isMissingRecently(std::string const& key);

private:
    std::unordered_map<std::string, CacheEntry> m_cache;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_missing;
    std::mutex m_mutex;
};
