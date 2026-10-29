#pragma once

#include <glm/glm.hpp>
#include "Animation.h"
#include <vector>
#include <SDL3/SDL.h>
#include "GlobalConstants.h"
#include "HealthBar.h"
#include <iostream>

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
	float maxHealth = 100;
	float health;
	bool isDead = false;
	Timer hitTimer;

	CharacterData() : hitTimer(0.5), health(maxHealth) {}

	void Damage(float dmg)
	{
		if (isDead) return;

		health = std::max(health - dmg, 0.f);
		if (health <= 0) isDead = true;
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
	int spriteFrame;

	Timer flashTimer;
	bool shouldFlash = false;

	GameObject(const SpriteSheet* sprite) : data{.level = LevelData()}, flashTimer(0.1f), sprite(sprite)
	{
		type = ObjectType::level;
		direction = 1;
		position = velocity = acceleration = impulse = glm::vec2(0);
		maxSpeed = glm::vec2(0);
		collider = { .x = 0, .y = 0, .w = static_cast<float>(TILE_SIZE), .h = static_cast<float>(TILE_SIZE) };
		currentAnimation = -1;
		spriteFrame = 1;
	}

	const SpriteSheet* getSpriteSheet() const
	{
		return currentAnimation != -1 ? animations[currentAnimation].spriteSheet : sprite;
	}

	void playAnimation(int index, bool preservePhase = false, bool restart = false)
	{
		if (index < 0 || index >= static_cast<int>(animations.size()))
			return;

		// if already playing this clip: do not reset it every frame
		if (index == currentAnimation && !restart)
			return;

		float phase = 0.f;
		if (preservePhase && currentAnimation >= 0 && !restart)
			phase = animations[currentAnimation].normalisedTime();
		
		currentAnimation = index;
		animations[index].setNormalisedTime(phase);
	}

private:
	const SpriteSheet* sprite = nullptr;
};