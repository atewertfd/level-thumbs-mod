# Level Thumbnails (Geode)

Adds level thumbnail previews to online level cells and the level info screen.

## Features
- List-cell thumbnail rendering with visual customization
- Level info thumbnail popup and background support
- Memory cache + disk cache (TTL and size limit)
- Retry/backoff networking and concurrent request cap
- Compatibility with modern and legacy thumbnail API endpoints

## Build
```sh
cmake -S . -B build
cmake --build build --config RelWithDebInfo
```

The built package is generated at:
- `build/bnanadude6.level-thumbs.geode`
