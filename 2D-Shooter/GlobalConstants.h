#pragma once

inline constexpr size_t LAYER_IDX_LEVEL = 0;
inline constexpr size_t LAYER_IDX_CHARACTERS = 1;

// level constants
inline constexpr int MAP_ROWS = 5;
inline constexpr int MAP_COLUMNS = 50;

// tile constants
inline int TILE_SIZE = 32;

// game constants
inline constexpr float GRAVITY = 500;
inline constexpr float DRAG = 500;
inline constexpr float KNOCKBACK = 300;
inline constexpr float JUMP_FORCE = -200;
inline constexpr float BULLET_DMG = 20;
