#pragma once

// Axis-aligned bounding box. Non-rotated rectangle.
// Boundary convention: half-open intervals [x, x+w) × [y, y+h).
//   - Left and top edges are inside.
//   - Right and bottom edges are outside.
// two boxes that only touch along an edge are not considered overlapping.

struct AABB {
    float x = 0.f, y = 0.f, w = 0.f, h = 0.f;

    float left() const { return x; }
    float top() const { return y; }
    float right() const { return x + w; }
    float bottom() const { return y + h; }
    float centerX() const { return x + w * 0.5f; }
    float centerY() const { return y + h * 0.5f; }

    bool contains(float px, float py) const {
        return px >= x && px < x + w &&
               py >= y && py < y + h;
    }

    bool contains(const AABB& other) const {
        return other.x >= x && other.right() <= right() &&
               other.y >= y && other.bottom() <= bottom();
    }

    bool intersects(const AABB& other) const {
        return x < other.right() && right() > other.x &&
               y < other.bottom() && bottom() > other.y;
    }

    bool isEmpty() const { return w <= 0.f || h <= 0.f; }
};
