#pragma once

#include <cstddef>
#include <string>

#include <windows.h>

namespace kinoko {

struct RuntimeView {
    bool mounted = false;
    std::size_t archive_count = 0;
    std::size_t entry_count = 0;
    std::string data_directory;
    std::string error;
};

class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual void paint(HDC dc, const RECT& client, const RuntimeView& view) = 0;
};

class GdiRenderer final : public IRenderer {
public:
    void paint(HDC dc, const RECT& client, const RuntimeView& view) override;
};

} // namespace kinoko

