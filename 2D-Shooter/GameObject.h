#pragma once

#include <glm/glm.hpp>
#include "Animation.h"
#include <vector>
#include <SDL3/SDL.h>
#include "GlobalConstants.h"

enum class ObjectType
{
	player, level, enemy
};

enum class PlayerState
{
	idle, running, jumping
};

struct PlayerData
{
	PlayerState state;
	PlayerData() : state(PlayerState::idle) {}
};

struct EnemyData {};
struct LevelData {};

union ObjectData
{
	LevelData level;
	PlayerData player;
	EnemyData enemy;
};

struct GameObject
{
	ObjectType type;
	ObjectData data;
	glm::vec2 position, velocity, acceleration, maxSpeed;
	float direction;
	bool dynamic = false;
	bool grounded = false;
	SDL_FRect collider;

	std::vector<Animation> animations;
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
		texture = nullptr;
	}
};