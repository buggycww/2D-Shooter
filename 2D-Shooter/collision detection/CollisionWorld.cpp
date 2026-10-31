#include "CollisionWorld.h"

#include "../GameObject.h"

#include <algorithm>
#include <cstddef>

namespace {
AABB colliderBounds(const GameObject& obj) {
    return {
        obj.position.x + obj.collider.x,
        obj.position.y + obj.collider.y,
        obj.collider.w,
        obj.collider.h
    };
}

bool isSolidStatic(const GameObject& obj) {
    return obj.type == ObjectType::level &&
           obj.collider.w > 0.f && obj.collider.h > 0.f;
}
}

void CollisionWorld::clear() {
    tree.reset();
    handles.clear();
    owners.clear();
}

void CollisionWorld::build(std::vector<std::vector<GameObject>>& layers) {
    clear();

    // Fit the root to the level geometry. Padding keeps boxes on the map's
    // right/bottom edge inside the root despite floating-point rounding.
    float minX = 0.f, minY = 0.f, maxX = 0.f, maxY = 0.f;
    bool found = false;

    for (const auto& layer : layers) {
        for (const GameObject& obj : layer) {
            if (!isSolidStatic(obj)) continue;

            const AABB bounds = colliderBounds(obj);
            if (!found) {
                minX = bounds.x;
                minY = bounds.y;
                maxX = bounds.right();
                maxY = bounds.bottom();
                found = true;
            }
            else {
                minX = std::min(minX, bounds.x);
                minY = std::min(minY, bounds.y);
                maxX = std::max(maxX, bounds.right());
                maxY = std::max(maxY, bounds.bottom());
            }
        }
    }

    if (!found) return;

    constexpr float padding = 1.f;
    const AABB world{
        minX - padding,
        minY - padding,
        (maxX - minX) + padding * 2.f,
        (maxY - minY) + padding * 2.f
    };

    constexpr int capacity = 8;
    constexpr int maxDepth = 10;
    tree = std::make_unique<QuadTree>(world, capacity, maxDepth);

    for (auto& layer : layers) {
        for (GameObject& obj : layer) {
            if (!isSolidStatic(obj)) continue;

            const AABB bounds = colliderBounds(obj);

            auto handle = std::make_unique<Point>(bounds.centerX(), bounds.centerY());
            handle->id = static_cast<int>(owners.size());
            Point* point = handle.get();

            if (!tree->insert(bounds, point)) continue; // outside the root

            owners.push_back(&obj);
            handles.push_back(std::move(handle));
        }
    }
}

void CollisionWorld::query(const AABB& area, std::vector<GameObject*>& out) const {
    if (!tree) return;

    tree->query(area, [&out, this](Point* point) {
        if (point->id >= 0 && static_cast<std::size_t>(point->id) < owners.size()) {
            out.push_back(owners[point->id]);
        }
        return true;
    });
}

bool CollisionWorld::overlaps(const AABB& area) const {
    if (!tree) return false;

    bool found = false;
    tree->query(area, [&found](Point*) {
        found = true;
        return false; // one hit is enough
    });
    return found;
}
