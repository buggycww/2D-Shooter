#include "QuadTree.h"
#include <algorithm>

QuadTree::QuadNode::QuadNode(const AABB& _bounds, int _depth)
    :bounds(_bounds), depth(_depth)
{
}

QuadTree::QuadTree(const AABB& bounds, int cap, int maxD, float _minW, float _minH)
    : capacity(std::max(cap, 1)), maxDepth(std::max(maxD, 0)), minW(_minW), minH(_minH)
{
    root = std::make_unique<QuadNode>(bounds, 0);
}

bool QuadTree::insert(const AABB& bounds, Point* p) {
    if (p == nullptr) return false;
    if (bounds.isEmpty()) return false;

    // Entries outside the root can never be found by a query.
    if (!root->bounds.contains(bounds)) return false;

    return insertHelper(root.get(), bounds, p);
}

bool QuadTree::remove(Point* p) {
    if (p == nullptr) return false;

    return removeHelper(root.get(), p);
}

void QuadTree::clear() {
    AABB bounds = root->bounds;
    root = std::make_unique<QuadNode>(bounds, 0);
}

int QuadTree::countNodes() const {
    return countNodesHelper(root.get());
}

int QuadTree::countEntries() const {
    return countEntriesHelper(root.get());
}

bool QuadTree::insertHelper(QuadNode* node, const AABB& bounds, Point* p) {
    if (!node->isLeaf()) {
        int q = getQuadrant(node->bounds, bounds);
        if (q >= 0) return insertHelper(node->children[q].get(), bounds, p);

        // The entry spans a split; keep it here so it is found from both sides.
        node->entries.push_back({ bounds, p });
        return true;
    }

    node->entries.push_back({ bounds, p });
    if ((int)node->entries.size() > capacity) subdivide(node);
    return true;
}

bool QuadTree::removeHelper(QuadNode* node, Point* p) {
    if (node == nullptr) return false;

    auto it = std::find_if(node->entries.begin(), node->entries.end(),
        [p](const Entry& entry) { return entry.point == p; });

    if (it != node->entries.end()) {
        node->entries.erase(it);
        return true;
    }

    if (!node->isLeaf()) {
        for (const auto& child : node->children) {
            if (removeHelper(child.get(), p)) return true;
        }
    }

    return false;
}

void QuadTree::subdivide(QuadNode* node) {
    if (node->depth >= maxDepth) return;

    const float halfW = node->bounds.w / 2.f;
    const float halfH = node->bounds.h / 2.f;
    if (halfW < minW || halfH < minH) return;

    const float xPos = node->bounds.x;
    const float middleXPos = node->bounds.x + halfW;
    const float yPos = node->bounds.y;
    const float middleYPos = node->bounds.y + halfH;

    // y grows downwards
    const AABB childrenBounds[4] = {
        { xPos,        yPos,        halfW, halfH }, // NW
        { middleXPos,  yPos,        halfW, halfH }, // NE
        { xPos,        middleYPos,  halfW, halfH }, // SW
        { middleXPos,  middleYPos,  halfW, halfH }  // SE
    };

    for (int i = 0; i < 4; i++) {
        node->children[i] = std::make_unique<QuadNode>(childrenBounds[i], node->depth + 1);
    }

    // Move entries that fit entirely inside one child down. Straddlers stay.
    std::vector<Entry> entries;
    entries.swap(node->entries);

    for (const Entry& entry : entries) {
        int q = getQuadrant(node->bounds, entry.bounds);
        if (q >= 0) {
            insertHelper(node->children[q].get(), entry.bounds, entry.point);
        }
        else {
            node->entries.push_back(entry);
        }
    }
}

int QuadTree::countNodesHelper(const QuadNode* node) const {
    if (node == nullptr) return 0;

    int count = 1;
    if (!node->isLeaf()) {
        for (const auto& child : node->children) {
            count += countNodesHelper(child.get());
        }
    }
    return count;
}

int QuadTree::countEntriesHelper(const QuadNode* node) const {
    if (node == nullptr) return 0;

    int count = static_cast<int>(node->entries.size());
    if (!node->isLeaf()) {
        for (const auto& child : node->children) {
            count += countEntriesHelper(child.get());
        }
    }
    return count;
}

int QuadTree::getQuadrant(const AABB& bounds, const AABB& item) const {
    const float middleX = bounds.x + bounds.w / 2.f;
    const float middleY = bounds.y + bounds.h / 2.f;

    // A box that crosses a split belongs to the parent.
    if (item.x < middleX && item.right() > middleX) return -1;
    if (item.y < middleY && item.bottom() > middleY) return -1;

    const bool left  = item.right() <= middleX;
    const bool above = item.bottom() <= middleY;

    if (left  && above)  return 0;  // NW
    if (!left && above)  return 1;  // NE
    if (left  && !above) return 2;  // SW
    return 3;                       // SE
}
