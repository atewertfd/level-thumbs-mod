# 1.5.0
- Added disk cache with configurable size limit and TTL.
- Added automatic retry + backoff for failed thumbnail requests.
- Added global concurrent-download cap for smoother scrolling.
- Added LevelInfo background quality mode (`Low` / `High`).
- Added cache maintenance actions in settings (`Clear Memory Cache Now`, `Clear Disk Cache Now`).
- Added off-screen request guardrail so list cells avoid unnecessary fetches.

# 1.4.0
- Reorganized settings into clean sections (`General`, `Appearance`, `Network & Performance`, `Experimental`).
- Added controls for list thumbnail visibility, progress text, skew, opacity, separator opacity, background scale mode, and request timeout.
- Updated runtime hooks to apply the new settings immediately.

# 1.3.0
- Updated thumbnail decoding flow for modern GD/Geode image handling.
- Added robust fallback decoding for WebP/unknown image payloads.
- Improved request handling and missing-thumbnail suppression.

# 1.2.0
- Added LevelInfo thumbnail background integration.
- Added thumbnail popup improvements and safer async completion handling.
- Added in-game screenshot preview flow groundwork.

# 1.1.0
- Added thumbnail rendering in level list cells.
- Added list limit controls to reduce list rendering load.
- Added base memory cache with LRU-style eviction.

# 1.0.0
- Initial Geode template port of Level Thumbnails core behavior.
