#pragma once

#include "AABB.h"
#include "Point.h"

#include <array>
#include <memory>
#include <vector>

// Quadtree broad phase for axis-aligned boxes.
//
// The original point quadtree could only answer "which points are inside this
// rectangle", which is not enough to find boxes that overlap a collider: a
// large box can overlap the query while its centre lies outside it. Entries
// therefore store their full AABB.
//
// A box is stored exactly once, in the deepest node that fully contains it.
// Boxes that straddle a split stay in the parent node. A query tests the
// entries of every visited node, so straddling boxes are still found.
//
// The Point* is only an identity/handle. Point::id lets the caller map a hit
// back to the owning game object.
class QuadTree {
    public:
        struct Entry {
            AABB bounds;
            Point* point;
        };

        struct QuadNode {
            AABB bounds;
            std::vector<Entry> entries;
            std::array<std::unique_ptr<QuadNode>, 4> children;
            int depth;

            QuadNode(const AABB& _bounds, int _depth);
            bool isLeaf() const { return children[0] == nullptr; }
        };

        QuadTree(const AABB& _bounds, int _capacity, int _maxDepth, float _minW = 1, float _minH = 1);
        ~QuadTree() = default;

        QuadTree(const QuadTree&) = delete;
        QuadTree& operator= (const QuadTree&) = delete;

        bool insert(const AABB& bounds, Point* p);
        bool remove(Point* p);
        void clear();

        // Calls visit(Point*) for every entry overlapping bounds. Return false
        // from the visitor to stop traversal early; query() then returns false.
        template <typename Visitor>
        bool query(const AABB& bounds, Visitor&& visitor) const {
            return queryHelper(root.get(), bounds, visitor);
        }

        const QuadNode* getRoot() const { return root.get(); }
        int getCapacity() const { return capacity; }
        int getMaxDepth() const { return maxDepth; }

        // Diagnostics, useful for the debug overlay.
        int countNodes() const;
        int countEntries() const;

    private:
        std::unique_ptr<QuadNode> root;
        int capacity; // max entries a node can hold before it subdivides
        int maxDepth; // max depth of a node
        float minW;
        float minH; // minimums to guard against dividing an excessively small node

        bool insertHelper(QuadNode* node, const AABB& bounds, Point* p);
        template <typename Visitor>
        bool queryHelper(const QuadNode* node, const AABB& bounds, Visitor& visitor) const;
        bool removeHelper(QuadNode* node, Point* p);
        void subdivide(QuadNode* node);
        int countNodesHelper(const QuadNode* node) const;
        int countEntriesHelper(const QuadNode* node) const;

        // Returns the child index fully containing item, or -1 if the item
        // crosses the node's split and must stay in this node.
        int getQuadrant(const AABB& bounds, const AABB& item) const;
};

template <typename Visitor>
bool QuadTree::queryHelper(const QuadNode* node, const AABB& bounds, Visitor& visitor) const {
    if (node == nullptr) return true;
    if (!node->bounds.intersects(bounds)) return true;

    for (const Entry& entry : node->entries) {
        if (!entry.bounds.intersects(bounds)) continue;
        if (!visitor(entry.point)) return false;
    }

    if (!node->isLeaf()) {
        for (const auto& child : node->children) {
            if (!queryHelper(child.get(), bounds, visitor)) return false;
        }
    }

    return true;
}
