// 2D-Shooter.cpp : Defines the entry point for the application.
//

#include "2D-Shooter.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>
#include <vector>
#include <array>

#include "AssetPaths.h"
#include "GameObject.h"

struct SDLState
{
	SDL_Window* window;
	SDL_Renderer* renderer;
	int width, height, logW, logH;
	const bool* keys;

	SDLState() : keys(SDL_GetKeyboardState(nullptr)) {}
};

const size_t LAYER_IDX_LEVEL = 0;
const size_t LAYER_IDX_CHARACTERS = 1;

const int TILE_SIZE = 32;

struct GameState
{
	std::array<std::vector<GameObject>, 2> layers;
	int playerIndex;

	GameState()
	{
		playerIndex = 0; // changed when map is loaded
	}
};

struct Resources
{
	const int ANIM_PLAYER_IDLE = 0;
	const int ANIM_PLAYER_RUN = 1;

	std::vector<Animation> playerAnims;

	std::vector<SDL_Texture*> textures;
	SDL_Texture *idleTex, *runTex;

	SDL_Texture* loadTexture(SDL_Renderer* renderer, const char* filePath)
	{
		SDL_Texture* tex = IMG_LoadTexture(renderer, filePath);
		SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
		textures.push_back(tex);
		return tex;
	}

	void load(SDLState& state)
	{
		playerAnims.resize(2);
		playerAnims[ANIM_PLAYER_IDLE] = Animation(8, 1.6f);
		playerAnims[ANIM_PLAYER_RUN] = Animation(4, 0.5f);
		idleTex = loadTexture(state.renderer, idleTexFilePath);
		runTex = loadTexture(state.renderer, runTexFilePath);
	}

	void unload(SDLState& state)
	{
		for (SDL_Texture* tex : textures)
		{
			SDL_DestroyTexture(tex);
		}
	}
};

void cleanupSDL(SDLState& state);
bool initialiseSDL(SDLState& state);
void drawObject(const SDLState& state, GameObject& obj, float deltaTime);
void update(const SDLState& state, Resources& res, GameObject& obj, float deltaTime);

int main(int argc, char* argv[])
{
	SDLState state;
	state.width = 1600;
	state.height = 900;
	state.logW = 640;
	state.logH = 320;

	if (!initialiseSDL(state)) return 1;
	 
	// load assets
	Resources res;
	res.load(state);

	// setup game data
	GameState gs;
	GameObject player;
	player.type = ObjectType::player;
	player.data.player = PlayerData();
	player.texture = res.idleTex;
	player.animations = res.playerAnims;
	player.currentAnimation = res.ANIM_PLAYER_IDLE;
	player.acceleration = glm::vec2(300, 0);
	player.maxSpeed.x = 100;
	gs.layers[LAYER_IDX_CHARACTERS].push_back(player);

	// game loop
	bool running = true;
	uint64_t prevTime = SDL_GetTicks();

	while (running)
	{
		uint64_t nowTime = SDL_GetTicks();
		float deltaTime = (nowTime - prevTime) / 1000.f;

		SDL_Event event { 0 };
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_EVENT_QUIT:
				{
					running = false;
					break;
				}
				case SDL_EVENT_WINDOW_RESIZED:
				{
					state.width = event.window.data1;
					state.height = event.window.data2;
				}
			}

		}

		// update all objects
		for (auto& layer : gs.layers)
		{
			for (auto& obj : layer)
			{
				update(state, res, obj, deltaTime);
				if (obj.currentAnimation != -1)
				{
					obj.animations[obj.currentAnimation].step(deltaTime);
				}
			}
		}

		// perform drawing commands
		SDL_SetRenderDrawColor(state.renderer, 20, 10, 30, 255);
		SDL_RenderClear(state.renderer);

		// draw all objects
		for (auto& layer : gs.layers)
		{
			for (auto& obj : layer)
			{
				drawObject(state, obj, deltaTime);
			}
		}

		// swap buffers and present
		SDL_RenderPresent(state.renderer);
		prevTime = nowTime;
	}

	// cleanup
	res.unload(state);
	cleanupSDL(state);
	return 0;
}

bool initialiseSDL(SDLState& state)
{
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error initialising SDL3", nullptr);
		return false;
	}

	// create window
	state.window = SDL_CreateWindow("2D Shooter", state.width, state.height, SDL_WINDOW_RESIZABLE);
	if (!state.window)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating window", nullptr);
		cleanupSDL(state);
		return false;
	}

	// create renderer
	state.renderer = SDL_CreateRenderer(state.window, nullptr);
	if (!state.renderer)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating renderer", nullptr);
		cleanupSDL(state);
		return false;
	}

	// configure presentation
	SDL_SetRenderLogicalPresentation(state.renderer, state.logW, state.logH, SDL_LOGICAL_PRESENTATION_LETTERBOX);

	return true;
}

void cleanupSDL(SDLState& state)
{
	SDL_DestroyRenderer(state.renderer);
	SDL_DestroyWindow(state.window);
	SDL_Quit();
}

void drawObject(const SDLState& state, GameObject& obj, float deltaTime)
{
	float frameX = obj.animations[obj.currentAnimation].currentFrame() * TILE_SIZE;
	SDL_FRect srcRect
	{
		.x = frameX,
		.y = 0,
		.w = TILE_SIZE,
		.h = TILE_SIZE
	};

	SDL_FRect dstRect
	{
		.x = obj.position.x,
		.y = obj.position.y,
		.w = TILE_SIZE,
		.h = TILE_SIZE
	};

	SDL_FlipMode flipMode = obj.direction == -1 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
	SDL_RenderTextureRotated(state.renderer, obj.texture, &srcRect, &dstRect, 0, nullptr, flipMode);

}

void update(const SDLState& state, Resources& res, GameObject& obj, float deltaTime)
{
	switch (obj.type)
	{
		case ObjectType::player:
		{
			float currentDirection = 0;
			glm::vec2 moveAmount = glm::vec2(0);

			if (state.keys[SDL_SCANCODE_A])
			{
				currentDirection += -1;
			}
			if (state.keys[SDL_SCANCODE_D])
			{
				currentDirection += 1;
			}
			if (state.keys[SDL_SCANCODE_ESCAPE])
			{
				SDL_Event quitEvent{ .type = SDL_EVENT_QUIT };
				SDL_PushEvent(&quitEvent);
			}

			if (currentDirection != 0) obj.direction = currentDirection;

			switch (obj.data.player.state)
			{
				case PlayerState::idle:
				{
					if (currentDirection != 0)
					{
						obj.data.player.state = PlayerState::running;
						obj.currentAnimation = res.ANIM_PLAYER_RUN;
						obj.texture = res.runTex;
					}
					else
					{
						// add deceleration
						if (obj.velocity.x)
						{
							const float factor = obj.velocity.x > 0 ? -1.5 : 1.5;
							float amount = obj.acceleration.x * factor * deltaTime;
							if (std::abs(obj.velocity.x) < std::abs(amount)) obj.velocity.x = 0;
							else obj.velocity.x += amount;
						}
					}
					break;
				}
				case PlayerState::running:
				{
					if (currentDirection == 0)
					{
						obj.data.player.state = PlayerState::idle;
						obj.currentAnimation = res.ANIM_PLAYER_IDLE;
						obj.texture = res.idleTex;
					}
					break;
				}
			}

			// add acceleration to current velocity
			obj.velocity.x += obj.acceleration.x * currentDirection * deltaTime;
			if (std::abs(obj.velocity.x) > obj.maxSpeed.x) obj.velocity.x = obj.maxSpeed.x * currentDirection;

			obj.position.x += obj.velocity.x * deltaTime;
		}
	}
}