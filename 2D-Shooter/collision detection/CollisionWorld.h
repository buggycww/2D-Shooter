#pragma once

#include "AABB.h"
#include "Point.h"
#include "QuadTree.h"

#include <memory>
#include <vector>

struct GameObject;

// Spatial index for the level's solid colliders.
//
// Static tiles never move, so the tree is built once after the map is loaded.
class CollisionWorld {
public:
    void build(std::vector<std::vector<GameObject>>& layers);
    void clear();
    void query(const AABB& area, std::vector<GameObject*>& out) const;
    bool overlaps(const AABB& area) const;

    bool empty() const { return tree == nullptr; }
    int nodeCount() const { return tree ? tree->countNodes() : 0; }
    int entryCount() const { return tree ? tree->countEntries() : 0; }

private:
    std::unique_ptr<QuadTree> tree;

    // Points are handles stored in the quadtree; their ids index owners.
    std::vector<std::unique_ptr<Point>> handles;
    std::vector<GameObject*> owners;
};
