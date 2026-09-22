#pragma once
#include "kinoko/bitmap.h"

namespace kinoko {
// Owns only pixels. The optional palette and methods identity remain borrowed.
class Bitmap final {
    KinokoBitmap value_{};
public:
    Bitmap() = default;
    ~Bitmap() { kinoko_bitmap_release_pixels(&value_); }
    Bitmap(const Bitmap&) = delete;
    Bitmap& operator=(const Bitmap&) = delete;
    KinokoBitmap* get() noexcept { return &value_; }
    const KinokoBitmap* get() const noexcept { return &value_; }
};
}
