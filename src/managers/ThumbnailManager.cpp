#include "ThumbnailManager.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <iterator>
#include <memory>
#include <thread>

#include "SettingsManager.hpp"

using namespace geode::prelude;

namespace {
constexpr auto kMissingTTL = std::chrono::minutes(10);

bool looksLikeWebP(std::vector<uint8_t> const& data) {
    static constexpr std::array<uint8_t, 4> riff = { 'R', 'I', 'F', 'F' };
    static constexpr std::array<uint8_t, 4> webp = { 'W', 'E', 'B', 'P' };
    return data.size() >= 12 &&
        std::equal(riff.begin(), riff.end(), data.begin()) &&
        std::equal(webp.begin(), webp.end(), data.begin() + 8);
}

using FileClock = std::filesystem::file_time_type::clock;
}

ThumbnailManager& ThumbnailManager::get() {
    static ThumbnailManager instance;
    return instance;
}

std::string ThumbnailManager::cacheKey(int32_t levelID, Quality quality) {
    return fmt::format("{}:{}@{}", levelID, static_cast<int>(quality), Settings::thumbnailBaseUrl());
}

std::string ThumbnailManager::qualityPathPart(Quality quality) {
    switch (quality) {
        case Quality::Low:
            return "small";
        case Quality::High:
        default:
            return "";
    }
}

std::string ThumbnailManager::withNoTrailingSlash(std::string value) {
    while (!value.empty() && value.back() == '/') {
        value.pop_back();
    }
    return value;
}

std::filesystem::path ThumbnailManager::cacheDirectoryForCurrentApi() {
    auto hash = std::hash<std::string> {}(Settings::thumbnailBaseUrl());
    auto dir = Mod::get()->getSaveDir() / "cache" / fmt::to_string(hash);

    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        log::warn("Failed to create disk cache directory {}: {}", dir.string(), ec.message());
    }
    return dir;
}

std::filesystem::path ThumbnailManager::cachePathFor(int32_t levelID, Quality quality) {
    auto suffix = quality == Quality::Low ? "low" : "high";
    return cacheDirectoryForCurrentApi() / fmt::format("{}-{}.img", levelID, suffix);
}

std::optional<Ref<CCTexture2D>> ThumbnailManager::getThumbnail(int32_t levelID, Quality quality) {
    auto key = cacheKey(levelID, quality);
    std::lock_guard lock(m_mutex);

    auto it = m_cache.find(key);
    if (it == m_cache.end()) {
        return std::nullopt;
    }

    it->second.lastAccess = std::chrono::steady_clock::now();
    return it->second.texture;
}

void ThumbnailManager::clearMemoryCache() {
    std::lock_guard lock(m_mutex);
    m_cache.clear();
    m_missing.clear();
}

void ThumbnailManager::clearDiskCache() {
    std::lock_guard lock(m_diskMutex);
    auto root = Mod::get()->getSaveDir() / "cache";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    if (ec) {
        log::warn("Failed to clear disk cache {}: {}", root.string(), ec.message());
    }
}

void ThumbnailManager::trimDiskCache() {
    std::lock_guard lock(m_diskMutex);

    auto dir = cacheDirectoryForCurrentApi();
    auto limit = Settings::thumbnailFileCacheLimit();

    if (limit == 0) {
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
        std::filesystem::create_directories(dir, ec);
        return;
    }

    struct DiskEntry {
        std::filesystem::path path;
        std::filesystem::file_time_type lastWrite;
    };
    std::vector<DiskEntry> entries;

    std::error_code ec;
    for (auto const& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (ec || !entry.is_regular_file()) {
            continue;
        }

        auto lastWrite = entry.last_write_time(ec);
        if (ec) {
            continue;
        }

        entries.push_back({ entry.path(), lastWrite });
    }

    auto ttl = std::chrono::hours(std::max<int64_t>(1, Settings::thumbnailFileCacheTTLHours()));
    auto now = FileClock::now();

    std::erase_if(entries, [&](DiskEntry const& entry) {
        if ((now - entry.lastWrite) > ttl) {
            std::filesystem::remove(entry.path, ec);
            return true;
        }
        return false;
    });

    if (limit < 0) {
        return;
    }

    auto maxEntries = static_cast<size_t>(limit);
    if (entries.size() <= maxEntries) {
        return;
    }

    std::sort(entries.begin(), entries.end(), [](DiskEntry const& a, DiskEntry const& b) {
        return a.lastWrite < b.lastWrite;
    });

    for (size_t i = 0; i < (entries.size() - maxEntries); i++) {
        std::filesystem::remove(entries[i].path, ec);
    }
}

geode::Result<std::vector<uint8_t>> ThumbnailManager::readDiskCache(int32_t levelID, Quality quality) {
    std::lock_guard lock(m_diskMutex);

    auto path = cachePathFor(levelID, quality);
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || ec) {
        return Err("Disk cache miss");
    }

    auto lastWrite = std::filesystem::last_write_time(path, ec);
    if (ec) {
        std::filesystem::remove(path, ec);
        return Err("Invalid disk cache entry");
    }

    auto ttl = std::chrono::hours(std::max<int64_t>(1, Settings::thumbnailFileCacheTTLHours()));
    if ((FileClock::now() - lastWrite) > ttl) {
        std::filesystem::remove(path, ec);
        return Err("Disk cache entry expired");
    }

    auto read = file::readBinary(path);
    if (!read) {
        std::filesystem::remove(path, ec);
        return Err("Failed to read disk cache entry");
    }

    auto data = std::move(read).unwrap();
    if (data.empty()) {
        std::filesystem::remove(path, ec);
        return Err("Disk cache entry was empty");
    }

    std::filesystem::last_write_time(path, FileClock::now(), ec);
    return Ok(std::move(data));
}

void ThumbnailManager::writeDiskCache(int32_t levelID, Quality quality, std::vector<uint8_t> const& data) {
    if (Settings::thumbnailFileCacheLimit() == 0 || data.empty()) {
        return;
    }

    std::lock_guard lock(m_diskMutex);

    auto path = cachePathFor(levelID, quality);
    auto write = file::writeBinary(path, data);
    if (!write) {
        log::warn("Failed to write disk cache {}: {}", path.string(), write.unwrapErr());
        return;
    }

    std::error_code ec;
    std::filesystem::last_write_time(path, FileClock::now(), ec);
}

arc::Future<void> ThumbnailManager::acquireRequestSlot() {
    std::optional<arc::oneshot::Receiver<bool>> waiter;
    {
        std::lock_guard lock(m_slotMutex);

        auto limit = static_cast<size_t>(std::clamp<int64_t>(Settings::maxConcurrentDownloads(), 1, 12));
        if (m_waitQueue.empty() && m_activeRequests < limit) {
            m_activeRequests++;
            co_return;
        }

        auto [tx, rx] = arc::oneshot::channel<bool>();
        m_waitQueue.push_back(std::move(tx));
        waiter.emplace(std::move(rx));
    }

    if (waiter) {
        (void) co_await waiter->recv();
    }
}

void ThumbnailManager::releaseRequestSlot() {
    std::optional<arc::oneshot::Sender<bool>> nextWaiter;
    {
        std::lock_guard lock(m_slotMutex);

        if (!m_waitQueue.empty()) {
            nextWaiter.emplace(std::move(m_waitQueue.front()));
            m_waitQueue.pop_front();
        } else if (m_activeRequests > 0) {
            m_activeRequests--;
        }
    }

    if (nextWaiter) {
        (void) nextWaiter->send(true);
    }
}

ThumbnailManager::FetchFuture ThumbnailManager::fetchThumbnail(
    int32_t levelID,
    Quality quality,
    ProgressCallback progress
) {
    auto key = cacheKey(levelID, quality);

    {
        std::lock_guard lock(m_mutex);
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            it->second.lastAccess = std::chrono::steady_clock::now();
            co_return Ok(it->second.texture);
        }

        if (isMissingRecentlyLocked(key)) {
            co_return Err("Thumbnail not found");
        }
    }

    auto diskData = co_await async::runtime().spawnBlocking<geode::Result<std::vector<uint8_t>>>(
        [this, levelID, quality] {
            return readDiskCache(levelID, quality);
        }
    );

    if (diskData.isOk()) {
        auto image = co_await async::runtime().spawnBlocking<CCImage*>(
            [data = std::move(diskData).unwrap()]() mutable {
                return decodeImage(std::move(data));
            }
        );

        if (image) {
            auto [tx, rx] = arc::oneshot::channel<FetchResult>();
            queueInMainThread([this, image, key, tx = std::move(tx)]() mutable {
                std::unique_ptr<CCImage> imageOwner(image);

                auto texture = new CCTexture2D();
                if (!texture->initWithImage(image)) {
                    delete texture;
                    (void) tx.send(Err("Failed to create texture"));
                    return;
                }

                {
                    std::lock_guard lock(m_mutex);
                    if (Settings::thumbnailCacheLimit() > 0) {
                        m_cache[key] = CacheEntry {
                            texture,
                            std::chrono::steady_clock::now()
                        };
                        evictMemoryIfNeededLocked();
                    }
                }

                texture->release();
                (void) tx.send(Ok(texture));
            });

            auto recv = co_await rx.recv();
            if (!recv) {
                co_return Err("Thumbnail task cancelled");
            }
            co_return std::move(recv).unwrap();
        }

        co_await async::runtime().spawnBlocking<void>([levelID, quality] {
            std::error_code ec;
            std::filesystem::remove(cachePathFor(levelID, quality), ec);
        });
    }

    RequestSlotGuard requestSlot { this, false };
    co_await acquireRequestSlot();
    requestSlot.acquired = true;

    auto retries = std::clamp<int64_t>(Settings::thumbnailRetryCount(), 0, 6);
    auto baseBackoff = std::clamp<int64_t>(Settings::thumbnailRetryBackoffMs(), 50, 5000);
    std::string lastError = "Failed to download thumbnail";
    std::vector<uint8_t> responseData;
    auto progressHandler = progress
        ? std::make_shared<ProgressCallback>(std::move(progress))
        : nullptr;

    for (int64_t attempt = 0; attempt <= retries; attempt++) {
        auto req = web::WebRequest()
            .userAgent(LEVEL_THUMBS_USER_AGENT)
            .timeout(std::chrono::seconds(Settings::thumbnailRequestTimeoutSeconds()));
        if (progressHandler) {
            req.onProgress([progressHandler](web::WebProgress const& state) {
                if (*progressHandler) {
                    (*progressHandler)(state);
                }
            });
        }

        auto response = co_await req.get(buildRequestUrl(levelID, quality));
        if (response.ok()) {
            responseData = std::move(response).data();
            break;
        }

        if (response.code() == 404) {
            std::lock_guard lock(m_mutex);
            rememberMissingLocked(key);
            co_return Err("Thumbnail not found");
        }

        lastError = response.errorMessage();
        if (lastError.empty()) {
            lastError = fmt::format("HTTP {}", response.code());
        }

        if (attempt < retries) {
            auto delay = baseBackoff * (1LL << attempt);
            co_await async::runtime().spawnBlocking<void>([delay] {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            });
        }
    }

    if (responseData.empty()) {
        co_return Err(lastError);
    }

    co_await async::runtime().spawnBlocking<void>(
        [this, levelID, quality, data = responseData] {
            writeDiskCache(levelID, quality, data);
            trimDiskCache();
        }
    );

    auto image = co_await async::runtime().spawnBlocking<CCImage*>(
        [data = std::move(responseData)]() mutable {
            return decodeImage(std::move(data));
        }
    );
    if (!image) {
        co_return Err("Failed to decode thumbnail");
    }

    auto [tx, rx] = arc::oneshot::channel<FetchResult>();
    queueInMainThread([this, image, key, tx = std::move(tx)]() mutable {
        std::unique_ptr<CCImage> imageOwner(image);

        auto texture = new CCTexture2D();
        if (!texture->initWithImage(image)) {
            delete texture;
            (void) tx.send(Err("Failed to create texture"));
            return;
        }

        {
            std::lock_guard lock(m_mutex);
            if (Settings::thumbnailCacheLimit() > 0) {
                m_cache[key] = CacheEntry {
                    texture,
                    std::chrono::steady_clock::now()
                };
                evictMemoryIfNeededLocked();
            }
        }

        texture->release();
        (void) tx.send(Ok(texture));
    });

    auto recv = co_await rx.recv();
    if (!recv) {
        co_return Err("Thumbnail task cancelled");
    }
    co_return std::move(recv).unwrap();
}

std::string ThumbnailManager::buildRequestUrl(int32_t levelID, Quality quality) {
    auto base = withNoTrailingSlash(Settings::thumbnailBaseUrl());

    if (Settings::isLegacyAPI()) {
        return fmt::format("{}/{}.png", base, levelID);
    }

    auto lowPath = qualityPathPart(quality);
    auto baseIncludesThumbnail = base.ends_with("/thumbnail");
    if (baseIncludesThumbnail) {
        if (quality == Quality::High) {
            return fmt::format("{}/{}", base, levelID);
        }
        return fmt::format("{}/{}/{}", base, levelID, lowPath);
    }

    if (quality == Quality::High) {
        return fmt::format("{}/thumbnail/{}", base, levelID);
    }
    return fmt::format("{}/thumbnail/{}/{}", base, levelID, lowPath);
}

std::string ThumbnailManager::buildDirectImageUrl(int32_t levelID, Quality quality) {
    return buildRequestUrl(levelID, quality);
}

CCImage* ThumbnailManager::decodeImage(std::vector<uint8_t> data) {
    auto image = new CCImage();
    bool decoded = false;

    if (looksLikeWebP(data)) {
        decoded = image->initWithImageData(
            data.data(),
            static_cast<int>(data.size()),
            CCImage::kFmtWebp
        );
    } else {
        decoded = image->initWithImageData(data.data(), static_cast<int>(data.size()), CCImage::kFmtUnKnown);
        if (!decoded) {
            // Fallback for endpoints that return WebP but without expected headers.
            decoded = image->initWithImageData(
                data.data(),
                static_cast<int>(data.size()),
                CCImage::kFmtWebp
            );
        }
    }

    if (!decoded) {
        delete image;
        return nullptr;
    }
    return image;
}

void ThumbnailManager::evictMemoryIfNeededLocked() {
    auto limit = Settings::thumbnailCacheLimit();
    if (limit <= 0) {
        m_cache.clear();
        return;
    }

    while (m_cache.size() > static_cast<size_t>(limit)) {
        auto oldest = m_cache.begin();
        for (auto it = std::next(m_cache.begin()); it != m_cache.end(); ++it) {
            if (it->second.lastAccess < oldest->second.lastAccess) {
                oldest = it;
            }
        }
        m_cache.erase(oldest);
    }
}

void ThumbnailManager::rememberMissingLocked(std::string const& key) {
    m_missing[key] = std::chrono::steady_clock::now();
}

bool ThumbnailManager::isMissingRecentlyLocked(std::string const& key) {
    auto it = m_missing.find(key);
    if (it == m_missing.end()) {
        return false;
    }

    if ((std::chrono::steady_clock::now() - it->second) <= kMissingTTL) {
        return true;
    }

    m_missing.erase(it);
    return false;
}
