// 2D-Shooter.cpp : Defines the entry point for the application.
//

#include "2D-Shooter.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>
#include <vector>
#include <array>
#include <format>

#include "AssetPaths.h"
#include "GameObject.h"
#include "GlobalConstants.h"

struct SDLState
{
	SDL_Window* window;
	SDL_Renderer* renderer;
	int width, height, logW, logH;
	const bool* keys;

	SDLState()
		: window(nullptr), renderer(nullptr),
		width(0), height(0), logW(0), logH(0),
		keys(SDL_GetKeyboardState(nullptr)) {
	}
};

struct GameState
{
	std::array<std::vector<GameObject>, 2> layers;
	std::vector<GameObject> backgroundTiles;
	std::vector<GameObject> foregroundTiles;
	int playerIndex;
	SDL_FRect mapViewport;
	float bg2Scroll, bg3Scroll, bg4Scroll;

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

struct Resources
{
	const int ANIM_PLAYER_IDLE = 0;
	const int ANIM_PLAYER_RUN = 1;
	const int ANIM_PLAYER_SLIDE = 2;

	std::vector<Animation> playerAnims;

	std::vector<SDL_Texture*> textures;
	SDL_Texture* idleTex, * runTex, * slideTex,
		* groundTex, * grassTex, * panelTex, * brickTex,
		* bg1Tex, * bg2Tex, * bg3Tex, * bg4Tex;

	SDL_Texture* loadTexture(SDL_Renderer* renderer, const char* filePath)
	{
		SDL_Texture* tex = IMG_LoadTexture(renderer, filePath);
		SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
		textures.push_back(tex);
		return tex;
	}

	void load(SDLState& state)
	{
		playerAnims.resize(3);
		playerAnims[ANIM_PLAYER_IDLE] = Animation(8, 1.6f);
		playerAnims[ANIM_PLAYER_RUN] = Animation(4, 0.5f);
		playerAnims[ANIM_PLAYER_SLIDE] = Animation(1, 1.0f);
		idleTex = loadTexture(state.renderer, idleTexFilePath);
		runTex = loadTexture(state.renderer, runTexFilePath);
		slideTex = loadTexture(state.renderer, slideTexFilePath);
		groundTex = loadTexture(state.renderer, groundFilePath);
		grassTex = loadTexture(state.renderer, grassFilePath);
		panelTex = loadTexture(state.renderer, panelFilePath);
		brickTex = loadTexture(state.renderer, brickFilePath);
		bg1Tex = loadTexture(state.renderer, bg1FilePath);
		bg2Tex = loadTexture(state.renderer, bg2FilePath);
		bg3Tex = loadTexture(state.renderer, bg3FilePath);
		bg4Tex = loadTexture(state.renderer, bg4FilePath);
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
void drawObject(const SDLState& state, const GameState& gs, GameObject& obj, float deltaTime);
void update(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime);
void createTiles(const SDLState& state, GameState& gs, const Resources& res);
void checkCollision(GameObject& objA, GameObject& objB);
void handleKeyInput(const SDLState& state, GameObject& obj, SDL_Scancode key, bool keyDown);
void drawParallaxBackground(const SDLState& state, SDL_Texture* tex, float xVelocity, float& scrollPos, float scrollFactor, float deltaTime);

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
	GameState gs(state);
	createTiles(state, gs, res);

	// game loop
	bool running = true;
	uint64_t prevTime = SDL_GetTicks();

	while (running)
	{
		uint64_t nowTime = SDL_GetTicks();
		float deltaTime = (nowTime - prevTime) / 1000.f;

		SDL_Event event{ 0 };
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
					break;
				}
				case SDL_EVENT_KEY_DOWN:
				{
					if (event.key.scancode == SDL_SCANCODE_ESCAPE)
					{
						SDL_Event quitEvent{ .type = SDL_EVENT_QUIT };
						SDL_PushEvent(&quitEvent);
						break;
					}

					handleKeyInput(state, gs.getPlayer(), event.key.scancode, true);
					break;
				}
				case SDL_EVENT_KEY_UP:
				{
					handleKeyInput(state, gs.getPlayer(), event.key.scancode, false);
					break;
				}
			}

		}

		// update all objects
		for (auto& layer : gs.layers)
		{
			for (auto& obj : layer)
			{
				update(state, gs, res, obj, deltaTime);
				if (obj.currentAnimation != -1)
				{
					obj.animations[obj.currentAnimation].step(deltaTime);
				}
			}
		}

		// calculate viewport position
		gs.mapViewport.x = (gs.getPlayer().position.x + TILE_SIZE / 2) - (gs.mapViewport.w / 2);

		// perform drawing commands
		SDL_SetRenderDrawColor(state.renderer, 20, 10, 30, 255);
		SDL_RenderClear(state.renderer);

		// draw background
		
		SDL_RenderTexture(state.renderer, res.bg1Tex, nullptr, nullptr);
		drawParallaxBackground(state, res.bg4Tex, gs.getPlayer().velocity.x, gs.bg4Scroll, 0.075f, deltaTime);
		drawParallaxBackground(state, res.bg3Tex, gs.getPlayer().velocity.x, gs.bg3Scroll, 0.150f, deltaTime);
		drawParallaxBackground(state, res.bg2Tex, gs.getPlayer().velocity.x, gs.bg2Scroll, 0.3f, deltaTime);

		// draw all objects
		for (auto& tile : gs.backgroundTiles)
		{
			drawObject(state, gs, tile, deltaTime);
		}

		for (auto& layer : gs.layers)
		{
			for (auto& obj : layer)
			{
				drawObject(state, gs, obj, deltaTime);
			}
		}

		for (auto& tile : gs.foregroundTiles)
		{
			drawObject(state, gs, tile, deltaTime);
		}

		// display debug info
		SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
		SDL_RenderDebugText(state.renderer, 5, 5,
			std::format("State: {}, Grounded: {}, Position.X: {}",
				static_cast<int>(gs.getPlayer().data.player.state),
				gs.getPlayer().grounded, gs.getPlayer().position.x
			).c_str()
		);

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

void drawObject(const SDLState& state, const GameState& gs, GameObject& obj, float deltaTime)
{
	float frameX = obj.currentAnimation != -1 ? obj.animations[obj.currentAnimation].currentFrame() * TILE_SIZE : 0;
	SDL_FRect srcRect
	{
		.x = frameX,
		.y = 0,
		.w = TILE_SIZE,
		.h = TILE_SIZE
	};

	SDL_FRect dstRect
	{                                
		.x = obj.position.x - gs.mapViewport.x,
		.y = obj.position.y,
		.w = TILE_SIZE,
		.h = TILE_SIZE
	};

	SDL_FlipMode flipMode = obj.direction == -1 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
	SDL_RenderTextureRotated(state.renderer, obj.texture, &srcRect, &dstRect, 0, nullptr, flipMode);

}

void drawParallaxBackground(const SDLState& state, SDL_Texture* tex, float xVelocity, float& scrollPos, float scrollFactor, float deltaTime)
{
	scrollPos -= xVelocity * scrollFactor * deltaTime;
	if (scrollPos <= -tex->w)
	{
		scrollPos = 0;
	}

	SDL_FRect dstRect
	{
		.x = scrollPos,
		.y = 32,
		.w = tex->w * 2.f,
		.h = static_cast<float>(tex->h)
	};
	
	SDL_RenderTextureTiled(state.renderer, tex, nullptr, 1, &dstRect);

}

void update(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime)
{
	// handle viewport

	// add gravity
	if (obj.dynamic && !obj.grounded)
	{
		obj.velocity.y += GRAVITY * deltaTime;
	}

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

			if (currentDirection != 0) obj.direction = currentDirection;

			switch (obj.data.player.state)
			{
				case PlayerState::idle:
				{
					// switching to running state
					if (currentDirection != 0)
					{
						obj.data.player.state = PlayerState::running;
					}
					else
					{
						// add deceleration
						if (obj.velocity.x)
						{
							const float factor = obj.velocity.x > 0 ? -1.5f : 1.5f;
							float amount = obj.acceleration.x * factor * deltaTime;
							if (std::abs(obj.velocity.x) < std::abs(amount)) obj.velocity.x = 0;
							else obj.velocity.x += amount;
						}
					}
					obj.currentAnimation = res.ANIM_PLAYER_IDLE;
					obj.texture = res.idleTex;
					break;
				}
				case PlayerState::running:
				{
					// switching to idle state
					if (currentDirection == 0)
					{
						obj.data.player.state = PlayerState::idle;
					}
					
					if (obj.velocity.x * obj.direction < 0 && obj.grounded)
					{
						obj.currentAnimation = res.ANIM_PLAYER_SLIDE;
						obj.texture = res.slideTex;
					}
					else
					{
						obj.currentAnimation = res.ANIM_PLAYER_RUN;
						obj.texture = res.runTex;
					}

					break;
				}
				case PlayerState::jumping:
				{
					obj.currentAnimation = res.ANIM_PLAYER_RUN;
					obj.texture = res.runTex;
					break;
				}
			}

			// add acceleration to current velocity
			obj.velocity.x += obj.acceleration.x * currentDirection * deltaTime;
			if (std::abs(obj.velocity.x) > obj.maxSpeed.x) obj.velocity.x = obj.maxSpeed.x * currentDirection;
		}
	}

	// add velocity to position
	obj.position += obj.velocity * deltaTime;

	// handle collision detection for dynamic objects
	bool foundGround = false;
	for (auto& layer : gs.layers)
	{
		for (auto& other : layer)
		{
			if (&other != &obj)
			{
				checkCollision(obj, other);

				SDL_FRect sensor{
					.x = obj.position.x + obj.collider.x,
					.y = obj.position.y + obj.collider.y + obj.collider.h,
					.w = obj.collider.w,
					.h = 1
				};

				SDL_FRect rectB{
					.x = other.position.x + other.collider.x,
					.y = other.position.y + other.collider.y,
					.w = other.collider.w,
					.h = other.collider.h
				};

				SDL_FRect rectC{ 0 };
				if (SDL_GetRectIntersectionFloat(&sensor, &rectB, &rectC))
				{
					foundGround = true;
				}
			}
		}
	}
	if (obj.grounded != foundGround) obj.grounded = foundGround;

	// guard against edge case for being suspended 1px away from ground in permanent jumping state
	if (obj.type == ObjectType::player && obj.grounded && obj.data.player.state == PlayerState::jumping && obj.velocity.y >= 0)
	{
		obj.data.player.state = obj.direction != 0 ? PlayerState::running : PlayerState::idle;
	}
}

void collisionResponse(GameObject& objA, GameObject& objB, const SDL_FRect& rectC)
{
	if (objA.type == ObjectType::player)
	{
		switch (objB.type)
		{
			case ObjectType::level:
			{
				if (rectC.h < rectC.w) // vertical collision
				{
					if (objA.velocity.y > 0) objA.position.y -= rectC.h; // going down
					else if (objA.velocity.y < 0) objA.position.y += rectC.h; // going up

					objA.velocity.y = 0;
				}
				else // horizontal collision
				{
					if (objA.velocity.x > 0) objA.position.x -= rectC.w; // going right
					else if (objA.velocity.x < 0) objA.position.x += rectC.w; // going left

					objA.velocity.x = 0;
				}
				break;
			}
		}
	}
}

void checkCollision(GameObject& objA, GameObject& objB)
{
	SDL_FRect rectA{
		.x = objA.position.x + objA.collider.x,
		.y = objA.position.y + objA.collider.y,
		.w = objA.collider.w,
		.h = objA.collider.h
	};

	SDL_FRect rectB{
		.x = objB.position.x + objB.collider.x,
		.y = objB.position.y + objB.collider.y,
		.w = objB.collider.w,
		.h = objB.collider.h
	};

	SDL_FRect rectC{ 0 };

	if (SDL_GetRectIntersectionFloat(&rectA, &rectB, &rectC))
	{
		collisionResponse(objA, objB, rectC);
	}
}

void createTiles(const SDLState& state, GameState& gs, const Resources& res)
{
	/*
		1 - Ground
		2 - Panel
		3 - Enemy
		4 - Player
		5 - Grass
		6 - Brick
	*/

	short background[MAP_ROWS][MAP_COLUMNS] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	short map[MAP_ROWS][MAP_COLUMNS] = {
		0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 2, 0, 0, 0, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 2, 2, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	short foreground[MAP_ROWS][MAP_COLUMNS] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		5, 0, 0, 5, 5, 5, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	const auto loadMap = [&](short layer[MAP_ROWS][MAP_COLUMNS])
		{
			const auto createObject = [&state](int r, int c, ObjectType type, SDL_Texture* texture) {
				GameObject o;
				o.type = type;
				o.texture = texture;
				o.position = glm::vec2(c * TILE_SIZE, state.logH - ((MAP_ROWS - r) * TILE_SIZE));
				return o;
				};

			for (int r = 0; r < MAP_ROWS; r++)
			{
				for (int c = 0; c < MAP_COLUMNS; c++)
				{
					switch (layer[r][c])
					{
						case 1: // ground
						{
							GameObject ground = createObject(r, c, ObjectType::level, res.groundTex);
							ground.data.level = LevelData();
							gs.layers[LAYER_IDX_LEVEL].push_back(ground);
							break;
						}
						case 2: // panel
						{
							GameObject panel = createObject(r, c, ObjectType::level, res.panelTex);
							panel.data.level = LevelData();
							gs.layers[LAYER_IDX_LEVEL].push_back(panel);
							break;
						}
						case 4: // player
						{
							GameObject player = createObject(r, c, ObjectType::player, res.idleTex);
							player.data.player = PlayerData();
							player.animations = res.playerAnims;
							player.currentAnimation = res.ANIM_PLAYER_IDLE;
							player.acceleration = glm::vec2(300, 0);
							player.maxSpeed.x = 100;
							player.collider = { .x = 12, .y = 6, .w = 10, .h = 26 };
							player.dynamic = true;
							gs.layers[LAYER_IDX_CHARACTERS].push_back(player);
							gs.playerIndex = gs.layers[LAYER_IDX_CHARACTERS].size() - 1;
							break;
						}
						case 5: // grass
						{
							GameObject grass = createObject(r, c, ObjectType::level, res.grassTex);
							grass.data.level = LevelData();
							gs.foregroundTiles.push_back(grass);
							break;
						}
						case 6: // brick
						{
							GameObject brick = createObject(r, c, ObjectType::level, res.brickTex);
							brick.data.level = LevelData();
							gs.backgroundTiles.push_back(brick);
							break;
						}
					}
				}
			}
		};

	loadMap(background);
	loadMap(map);
	loadMap(foreground);

	assert(gs.playerIndex != -1);
}

void handleKeyInput(const SDLState& state, GameObject& obj, SDL_Scancode key, bool keyDown)
{
	if (obj.type == ObjectType::player)
	{
		switch (obj.data.player.state)
		{
			case PlayerState::idle:
			{
				if (key == SDL_SCANCODE_SPACE && keyDown && obj.grounded)
				{
					obj.data.player.state = PlayerState::jumping;
					obj.velocity.y = JUMP_FORCE;
				}
				break;
			}
			case PlayerState::running:
			{
				if (key == SDL_SCANCODE_SPACE && keyDown && obj.grounded)
				{
					obj.data.player.state = PlayerState::jumping;
					obj.velocity.y = JUMP_FORCE;
				}
				break;
			}
		}
	}
}