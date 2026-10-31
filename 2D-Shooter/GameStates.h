#pragma once

#include <cassert>
#include <cstddef>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <glm/glm.hpp>

#include "GameObject.h"
#include "collision detection/CollisionWorld.h"

struct SDLState
{
	SDL_Window* window;
	SDL_Renderer* renderer;
	MIX_Mixer* mixer;

	int width, height, logW, logH;
	const bool* keys;
	bool fullscreen = false;

	SDLState()
		: window(nullptr), renderer(nullptr), mixer(nullptr),
		width(0), height(0), logW(0), logH(0),
		keys(SDL_GetKeyboardState(nullptr)) {
	}
};

struct ObjectRef
{
	int layer = -1;
	int index = -1;
};

struct GameState
{
	std::vector<std::vector<GameObject>> layers;
	std::vector<GameObject> bullets;
	int playerLayer, playerIndex;

	// Broad phase for the static level tiles, built once after createTiles().
	CollisionWorld collision;

	// Player / enemies, so collision does not have to walk every tile.
	std::vector<ObjectRef> actors;

	SDL_FRect mapViewport;
	glm::vec2 maxViewportPos;

	float bg2Scroll, bg3Scroll, bg4Scroll;
	bool debugMode = false;

	GameState(const SDLState& state)
	{
		playerLayer = -1; // changed when map is loaded
		playerIndex = -1; // changed when map is loaded
		mapViewport = { .x = 0, .y = 0,
			.w = static_cast<float>(state.logW),
			.h = static_cast<float>(state.logH) };
		maxViewportPos = glm::vec2(0);
		bg2Scroll = bg3Scroll = bg4Scroll = 0;
	}

	bool hasPlayer() const
	{
		return playerLayer >= 0 && static_cast<std::size_t>(playerLayer) < layers.size()
			&& playerIndex >= 0
			&& static_cast<std::size_t>(playerIndex) < layers[playerLayer].size();
	}

	GameObject& getPlayer()
	{
		assert(hasPlayer());
		return layers[playerLayer][playerIndex];
	}

	// Call once after all layers have been created. Static tiles go into the
	// quadtree; moving objects are cached so the update loop can skip tiles.
	void buildCollision()
	{
		collision.build(layers);

		actors.clear();
		for (int l = 0; l < static_cast<int>(layers.size()); ++l)
		{
			for (int i = 0; i < static_cast<int>(layers[l].size()); ++i)
			{
				if (layers[l][i].dynamic) actors.push_back({ l, i });
			}
		}
	}
};