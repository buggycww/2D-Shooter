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

enum class EnemyState
{
	idle, shambling, hit, dead
};

struct CharacterData
{
	float health = 100;
	bool isDead = false;
	Timer hitTimer;

	CharacterData() : hitTimer(0.5) {}

	void Damage(float dmg)
	{
		if (isDead) return;

		health = std::max(health - dmg, 0.f);
		if (health == 0) isDead = true;
	}
};

struct PlayerData : CharacterData
{
	PlayerState state;
	bool isShooting = false;
	Timer shootTimer;
	PlayerData() : state(PlayerState::idle), shootTimer(0.2) {}
};

struct EnemyData : CharacterData
{
	EnemyState state;
	EnemyData() : state(EnemyState::idle) {}
};

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
	glm::vec2 impulse;
	float direction;
	bool dynamic = false;
	bool gravityEnabled = false;
	bool grounded = false;
	SDL_FRect collider;

	std::vector<Animation> animations;
	int currentAnimation;
	const SpriteSheet* sprite = nullptr;
	int spriteFrame;

	Timer flashTimer;
	bool shouldFlash = false;

	GameObject() : data{.level = LevelData()}, flashTimer(0.1f)
	{
		type = ObjectType::level;
		direction = 1;
		position = velocity = acceleration = impulse = glm::vec2(0);
		maxSpeed = glm::vec2(0);
		collider = { .x = 0, .y = 0, .w = static_cast<float>(TILE_SIZE), .h = static_cast<float>(TILE_SIZE) };
		currentAnimation = -1;
		spriteFrame = 1;
	}

	void playAnimation(int index)
	{
		currentAnimation = index;
		sprite = animations[index].spriteSheet;
		animations[index].reset();
	}
};