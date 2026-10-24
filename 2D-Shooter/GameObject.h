#pragma once

#include <glm/glm.hpp>
#include "Animation.h"
#include <vector>
#include <SDL3/SDL.h>
#include "GlobalConstants.h"

enum class ObjectType
{
	player, level, enemy, bullet
};

enum class PlayerState
{
	idle, running, jumping
};

enum class BulletState
{
	moving, colliding, inactive
};

struct PlayerData
{
	PlayerState state;
	bool isShooting = false;
	Timer shootTimer;
	PlayerData() : state(PlayerState::idle), shootTimer(0.2) {}
};

struct EnemyData {};
struct LevelData {};

struct BulletData
{
	BulletState state;
	BulletData() : state(BulletState::moving) {}
};

union ObjectData
{
	LevelData level;
	PlayerData player;
	EnemyData enemy;
	BulletData bullet;
};

struct GameObject
{
	ObjectType type;
	ObjectData data;
	glm::vec2 position, velocity, acceleration, maxSpeed;
	float direction;
	bool dynamic = false;
	bool gravityEnabled = false;
	bool grounded = false;
	SDL_FRect collider;

	std::vector<Animation> animations;
	int textureSize;
	int currentAnimation;

	SDL_Texture* texture;

	GameObject() : data{.level = LevelData()}
	{
		type = ObjectType::level;
		direction = 1;
		position = velocity = acceleration = glm::vec2(0);
		maxSpeed = glm::vec2(0);
		collider = { .x = 0, .y = 0, .w = TILE_SIZE, .h = TILE_SIZE };
		currentAnimation = -1;
		textureSize = TILE_SIZE;
		texture = nullptr;
	}
};