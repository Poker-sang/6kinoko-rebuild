#include "kinoko/renderer.hpp"

#include <algorithm>
#include <cstdio>

namespace kinoko {
namespace {

void fill_rect(HDC dc, const RECT& rect, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    if (brush != nullptr) {
        FillRect(dc, &rect, brush);
        DeleteObject(brush);
    }
}

void draw_text(HDC dc, int x, int y, const char* text, COLORREF color) {
    SetTextColor(dc, color);
    SetBkMode(dc, TRANSPARENT);
    TextOutA(dc, x, y, text, static_cast<int>(std::char_traits<char>::length(text)));
}

} // namespace

void GdiRenderer::paint(HDC dc, const RECT& client, const RuntimeView& view) {
    const int width = client.right - client.left;
    const int height = client.bottom - client.top;
    fill_rect(dc, client, RGB(91, 169, 219));

    const RECT cloud_left{42, 46, 178, 87};
    const RECT cloud_right{438, 72, 594, 112};
    HBRUSH cloud_brush = CreateSolidBrush(RGB(250, 250, 238));
    if (cloud_brush != nullptr) {
        HGDIOBJ old_brush = SelectObject(dc, cloud_brush);
        Ellipse(dc, cloud_left.left, cloud_left.top, cloud_left.right, cloud_left.bottom);
        Ellipse(dc, cloud_right.left, cloud_right.top, cloud_right.right, cloud_right.bottom);
        SelectObject(dc, old_brush);
        DeleteObject(cloud_brush);
    }

    const int ground_top = std::max(300, height - 124);
    RECT ground{0, ground_top, width, height};
    fill_rect(dc, ground, RGB(67, 151, 67));
    RECT soil{0, ground_top + 20, width, height};
    fill_rect(dc, soil, RGB(137, 84, 46));

    HBRUSH block_brush = CreateSolidBrush(RGB(237, 190, 71));
    HPEN block_pen = CreatePen(PS_SOLID, 1, RGB(131, 81, 37));
    if (block_brush != nullptr && block_pen != nullptr) {
        HGDIOBJ old_brush = SelectObject(dc, block_brush);
        HGDIOBJ old_pen = SelectObject(dc, block_pen);
        for (int x = 24; x < width; x += 48) {
            Rectangle(dc, x, ground_top - 36, x + 36, ground_top);
        }
        SelectObject(dc, old_pen);
        SelectObject(dc, old_brush);
    }
    if (block_pen != nullptr) {
        DeleteObject(block_pen);
    }
    if (block_brush != nullptr) {
        DeleteObject(block_brush);
    }

    draw_text(dc, 24, 18, "6kinoko rebuild", RGB(25, 42, 59));
    char status[128]{};
    std::snprintf(status, sizeof(status), "archives: %zu    assets: %zu",
                  view.archive_count, view.entry_count);
    draw_text(dc, 24, height - 48, status, RGB(255, 255, 255));

    if (view.mounted) {
        draw_text(dc, 24, height - 27, "ready", RGB(218, 255, 218));
    } else {
        draw_text(dc, 24, height - 27, "data unavailable", RGB(255, 230, 190));
    }
}

} // namespace kinoko

