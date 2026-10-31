#pragma once

// just for identity handles, not for geometry
struct Point {
    float x, y;
    int id;

    Point(float _x = 0, float _y = 0) : x(_x), y(_y) {}
};