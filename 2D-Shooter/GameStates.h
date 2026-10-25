#pragma once

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

struct GameState
{
	std::array<std::vector<GameObject>, 2> layers;
	std::vector<GameObject> backgroundTiles;
	std::vector<GameObject> foregroundTiles;
	std::vector<GameObject> bullets;
	int playerIndex;
	SDL_FRect mapViewport;
	float bg2Scroll, bg3Scroll, bg4Scroll;
	bool debugMode = false;

	GameState(const SDLState& state)
	{
		playerIndex = -1; // changed when map is loaded
		mapViewport = { .x = 0, .y = 0,
			.w = static_cast<float>(state.logW),
			.h = static_cast<float>(state.logH) };
		bg2Scroll = bg3Scroll = bg4Scroll = 0;
	}

	GameObject& getPlayer() { return layers[LAYER_IDX_CHARACTERS][playerIndex]; }
};