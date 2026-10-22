// 2D-Shooter.cpp : Defines the entry point for the application.
//

#include "2D-Shooter.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>
#include <vector>

#include "AssetPaths.h"
#include "Animation.h"

struct SDLState
{
	SDL_Window* window;
	SDL_Renderer* renderer;
	int width, height, logW, logH;
};

struct Resources
{
	const int ANIM_PLAYER_IDLE = 0;
	std::vector<Animation> playerAnims;

	std::vector<SDL_Texture*> textures;
	SDL_Texture* idleTex;

	SDL_Texture* loadTexture(SDL_Renderer* renderer, const char* filePath)
	{
		SDL_Texture* tex = IMG_LoadTexture(renderer, idleTexFilePath);
		SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
		textures.push_back(tex);
		return tex;
	}

	void load(SDLState& state)
	{
		playerAnims.resize(2);
		playerAnims[ANIM_PLAYER_IDLE] = Animation(8, 1.6f);
		idleTex = loadTexture(state.renderer, idleTexFilePath);
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

int main(int argc, char* argv[])
{
	SDLState state
	{
		.width = 1600,
		.height = 900,
		.logW = 640,
		.logH = 320
	};

	if (!initialiseSDL(state)) return 1;
	 
	// load assets
	Resources res;
	res.load(state);

	const float spriteSize = 32;
	float playerX = 0;
	float currentDir = 1;
	Animation idleAnim(8, 1.2);

	int floor = state.logH;
	const bool* keys = SDL_GetKeyboardState(nullptr);

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

		// handle input
		float moveAmount = 0;
		if (keys[SDL_SCANCODE_A])
		{
			moveAmount += -75.f;
			currentDir = -1;
		}
		if (keys[SDL_SCANCODE_D])
		{
			moveAmount += 75.f;
			currentDir = 1;
		}
		playerX += moveAmount * deltaTime;

		// animation logic
		idleAnim.step(deltaTime);

		// perform drawing commands
		SDL_SetRenderDrawColor(state.renderer, 20, 10, 30, 255);
		SDL_RenderClear(state.renderer);

		float frameX = idleAnim.currentFrame() * spriteSize;
		SDL_FRect srcRect
		{
			.x = frameX,
			.y = 0,
			.w = spriteSize,
			.h = spriteSize
		};

		SDL_FRect dstRect
		{
			.x = playerX,
			.y = floor - spriteSize,
			.w = spriteSize,
			.h = spriteSize
		};

		// SDL_RenderTexture(state.renderer, res.idleTex, &srcRect, &dstRect);
		SDL_RenderTextureRotated(state.renderer, res.idleTex, &srcRect, &dstRect, 0, nullptr, currentDir < 0 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);

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