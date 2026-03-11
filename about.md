# Level Thumbnails

Adds thumbnail previews to online level lists and the level info page.

This port targets modern Geode + GD 2.2081 and keeps compatibility with both:
- modern thumbnail API responses
- legacy PNG endpoints

It includes memory + disk caching, retry/backoff networking, and configurable rendering options for stable list performance.
