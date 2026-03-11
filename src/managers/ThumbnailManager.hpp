#pragma once

#include <chrono>
#include <deque>
#include <filesystem>
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
    enum class Quality {
        Low,
        High
    };

    using FetchResult = geode::Result<geode::Ref<cocos2d::CCTexture2D>>;
    using FetchFuture = arc::Future<FetchResult>;
    using ProgressCallback = geode::Function<void(geode::utils::web::WebProgress const&)>;

    static ThumbnailManager& get();

    std::optional<geode::Ref<cocos2d::CCTexture2D>> getThumbnail(
        int32_t levelID,
        Quality quality = Quality::High
    );
    FetchFuture fetchThumbnail(
        int32_t levelID,
        Quality quality = Quality::High,
        ProgressCallback progress = nullptr
    );

    void clearMemoryCache();
    void clearDiskCache();
    void trimDiskCache();

    static std::string buildRequestUrl(int32_t levelID, Quality quality = Quality::High);
    static std::string buildDirectImageUrl(int32_t levelID, Quality quality = Quality::High);

private:
    ThumbnailManager() = default;

    struct CacheEntry {
        geode::Ref<cocos2d::CCTexture2D> texture;
        std::chrono::steady_clock::time_point lastAccess;
    };

    struct RequestSlotGuard {
        ThumbnailManager* manager = nullptr;
        bool acquired = false;

        ~RequestSlotGuard() {
            if (acquired && manager) {
                manager->releaseRequestSlot();
            }
        }
    };

    static std::string cacheKey(int32_t levelID, Quality quality);
    static std::string qualityPathPart(Quality quality);
    static std::string withNoTrailingSlash(std::string value);
    static std::filesystem::path cacheDirectoryForCurrentApi();
    static std::filesystem::path cachePathFor(int32_t levelID, Quality quality);

    static cocos2d::CCImage* decodeImage(std::vector<uint8_t> data);
    void evictMemoryIfNeededLocked();
    void rememberMissingLocked(std::string const& key);
    bool isMissingRecentlyLocked(std::string const& key);

    geode::Result<std::vector<uint8_t>> readDiskCache(int32_t levelID, Quality quality);
    void writeDiskCache(int32_t levelID, Quality quality, std::vector<uint8_t> const& data);

    arc::Future<void> acquireRequestSlot();
    void releaseRequestSlot();

private:
    std::unordered_map<std::string, CacheEntry> m_cache;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_missing;
    std::mutex m_mutex;

    std::mutex m_diskMutex;
    std::mutex m_slotMutex;
    std::deque<arc::oneshot::Sender<bool>> m_waitQueue;
    size_t m_activeRequests = 0;
};
