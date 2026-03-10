#include "ThumbnailManager.hpp"

#include <algorithm>
#include <array>
#include <iterator>
#include <memory>

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
}

ThumbnailManager& ThumbnailManager::get() {
    static ThumbnailManager instance;
    return instance;
}

std::string ThumbnailManager::cacheKey(int32_t levelID) {
    return fmt::format("{}@{}", levelID, Settings::thumbnailBaseUrl());
}

std::optional<Ref<CCTexture2D>> ThumbnailManager::getThumbnail(int32_t levelID) {
    auto key = cacheKey(levelID);
    std::lock_guard lock(m_mutex);

    auto it = m_cache.find(key);
    if (it == m_cache.end()) {
        return std::nullopt;
    }

    it->second.lastAccess = std::chrono::steady_clock::now();
    return it->second.texture;
}

ThumbnailManager::FetchFuture ThumbnailManager::fetchThumbnail(int32_t levelID, ProgressCallback progress) {
    auto key = cacheKey(levelID);

    {
        std::lock_guard lock(m_mutex);
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            it->second.lastAccess = std::chrono::steady_clock::now();
            co_return Ok(it->second.texture);
        }

        if (isMissingRecently(key)) {
            co_return Err("Thumbnail not found");
        }
    }

    auto req = web::WebRequest().userAgent(LEVEL_THUMBS_USER_AGENT);
    if (progress) {
        req.onProgress(std::move(progress));
    }

    auto response = co_await req.get(buildRequestUrl(levelID));
    if (!response.ok()) {
        if (response.code() == 404) {
            std::lock_guard lock(m_mutex);
            rememberMissing(key);
            co_return Err("Thumbnail not found");
        }
        co_return Err(response.errorMessage());
    }

    auto image = co_await async::runtime().spawnBlocking<CCImage*>(
        [data = std::move(response).data()]() mutable {
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
                evictIfNeededLocked();
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

std::string ThumbnailManager::buildRequestUrl(int32_t levelID) {
    if (Settings::isLegacyAPI()) {
        return fmt::format("{}/{}.png", Settings::thumbnailBaseUrl(), levelID);
    }
    return fmt::format("{}/{}", Settings::thumbnailBaseUrl(), levelID);
}

std::string ThumbnailManager::buildDirectImageUrl(int32_t levelID) {
    return buildRequestUrl(levelID);
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

void ThumbnailManager::evictIfNeededLocked() {
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

void ThumbnailManager::rememberMissing(std::string const& key) {
    m_missing[key] = std::chrono::steady_clock::now();
}

bool ThumbnailManager::isMissingRecently(std::string const& key) {
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
