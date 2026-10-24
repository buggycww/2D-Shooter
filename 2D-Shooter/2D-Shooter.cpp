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
#include <algorithm>

#include "AssetPaths.h"
#include "GameObject.h"
#include "GlobalConstants.h"
#include "GameStates.h"
#include "Resources.h"

void cleanupSDL(SDLState& state);
bool initialiseSDL(SDLState& state);
void drawObject(const SDLState& state, const GameState& gs, GameObject& obj, float deltaTime);
void update(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime);
void createTiles(const SDLState& state, GameState& gs, const Resources& res);
void checkCollision(const Resources& res, GameObject& objA, GameObject& objB);
void handleKeyInput(const SDLState& state, GameObject& obj, SDL_Scancode key, bool keyDown);
void drawParallaxBackground(const SDLState& state, SDL_Texture* tex, float xVelocity, float& scrollPos, float scrollFactor, float deltaTime);
void handleMouseInput(GameState& gs, const Resources& res, GameObject& obj, uint8_t button, bool isPressed);
void handleShooting(GameState& gs, const Resources& res, GameObject& player);

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

					if (event.key.scancode == SDL_SCANCODE_GRAVE)
					{
						gs.debugMode = !gs.debugMode;
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
				case SDL_EVENT_MOUSE_BUTTON_DOWN:
				{
					handleMouseInput(gs, res, gs.getPlayer(), event.button.button, true);
					break;
				}
				case SDL_EVENT_MOUSE_BUTTON_UP:
				{
					handleMouseInput(gs, res, gs.getPlayer(), event.button.button, false);
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

		for (auto& bullet : gs.bullets)
		{
			update(state, gs, res, bullet, deltaTime);
			if (bullet.currentAnimation != -1)
			{
				bullet.animations[bullet.currentAnimation].step(deltaTime);
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
			SDL_FRect dstRect{
				.x = tile.position.x - gs.mapViewport.x, .y = tile.position.y,
				.w = TILE_SIZE, .h = TILE_SIZE
			};

			SDL_RenderTexture(state.renderer, tile.texture, nullptr, &dstRect);
		}

		for (auto& layer : gs.layers)
		{
			for (auto& obj : layer)
			{
				drawObject(state, gs, obj, deltaTime);
			}
		}

		for (auto& bullet : gs.bullets)
		{
			if (bullet.data.bullet.state != BulletState::inactive)
			{
				drawObject(state, gs, bullet, deltaTime);
			}
		}

		for (auto& tile : gs.foregroundTiles)
		{
			SDL_FRect dstRect{
				.x = tile.position.x - gs.mapViewport.x, .y = tile.position.y,
				.w = TILE_SIZE, .h = TILE_SIZE
			};

			SDL_RenderTexture(state.renderer, tile.texture, nullptr, &dstRect);
		}

		// display debug info
		if (gs.debugMode)
		{
			SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
			SDL_RenderDebugText(state.renderer, 5, 5,
				std::format("State: {}, Grounded: {}, Bullets: {}, YPos: {}",
					static_cast<int>(gs.getPlayer().data.player.state),
					gs.getPlayer().grounded, gs.bullets.size(), gs.getPlayer().position.y
				).c_str()
			);
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
	SDL_SetRenderVSync(state.renderer, 1);

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
	float frameX = obj.currentAnimation != -1 ? obj.animations[obj.currentAnimation].currentFrame() * obj.textureSize : 0;
	SDL_FRect srcRect
	{
		.x = frameX,
		.y = 0,
		.w = static_cast<float>(obj.textureSize),
		.h = static_cast<float>(obj.textureSize)
	};

	SDL_FRect dstRect
	{                                
		.x = obj.position.x - gs.mapViewport.x,
		.y = obj.position.y,
		.w = static_cast<float>(obj.textureSize),
		.h = static_cast<float>(obj.textureSize)
	};

	SDL_FlipMode flipMode = obj.direction == -1 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
	SDL_RenderTextureRotated(state.renderer, obj.texture, &srcRect, &dstRect, 0, nullptr, flipMode);

	if (gs.debugMode)
	{
		SDL_FRect rect{
		.x = obj.position.x + obj.collider.x - gs.mapViewport.x,
		.y = obj.position.y + obj.collider.y,
		.w = obj.collider.w,
		.h = obj.collider.h
		};
		SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(state.renderer, 255, 0, 0, 150);
		SDL_RenderFillRect(state.renderer, &rect);
		SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_NONE);
	}
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
	// add gravity
	if (obj.gravityEnabled && !obj.grounded)
	{
		obj.velocity.y += GRAVITY * deltaTime;
	}

	float currentDirection = 0;
	switch (obj.type)
	{
		case ObjectType::player:
		{
			glm::vec2 moveAmount = glm::vec2(0);

			if (state.keys[SDL_SCANCODE_A])
			{
				currentDirection += -1;
			}
			if (state.keys[SDL_SCANCODE_D])
			{
				currentDirection += 1;
			}

			obj.data.player.shootTimer.step(deltaTime);
			handleShooting(gs, res, obj);

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
					obj.currentAnimation = obj.data.player.isShooting ? res.ANIM_PLAYER_SHOOT : res.ANIM_PLAYER_IDLE;
					obj.texture = obj.data.player.isShooting ? res.shootTex : res.idleTex;
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
						obj.currentAnimation = obj.data.player.isShooting ? res.ANIM_PLAYER_SHOOTSLIDE : res.ANIM_PLAYER_SLIDE;
						obj.texture = obj.data.player.isShooting ? res.shootSlideTex : res.slideTex;
					}
					else
					{
						obj.currentAnimation = res.ANIM_PLAYER_RUN; // use same index to keep run cycle consistent
						obj.texture = obj.data.player.isShooting ? res.shootRunTex : res.runTex;
					}

					break;
				}
				case PlayerState::jumping:
				{
					obj.currentAnimation = res.ANIM_PLAYER_RUN;
					obj.texture = obj.data.player.isShooting ? res.shootRunTex : res.runTex;
					break;
				}
			}

			break;
		}
		case ObjectType::bullet:
		{
			switch (obj.data.bullet.state)
			{
				case BulletState::moving:
				{
					if (obj.position.x - gs.mapViewport.x < 0 || // left edge
						obj.position.x - gs.mapViewport.x > state.logW || // right edge
						obj.position.y - gs.mapViewport.y < 0 || // top edge
						obj.position.y - gs.mapViewport.y > state.logH) // bottom edge
					{
						obj.data.bullet.state = BulletState::inactive;
					}
					break;
				}
				case BulletState::colliding:
				{
					if (obj.animations[obj.currentAnimation].isDone())
					{
						obj.data.bullet.state = BulletState::inactive;
					}
					break;
				}
			}
			break;
		}
	}

	if (currentDirection != 0) obj.direction = currentDirection;

	// add acceleration to current velocity
	obj.velocity.x += obj.acceleration.x * currentDirection * deltaTime;
	if (std::abs(obj.velocity.x) > obj.maxSpeed.x) obj.velocity.x = obj.maxSpeed.x * currentDirection;

	// add velocity to position
	obj.position += obj.velocity * deltaTime;

	// handle collision detection for dynamic objects
	if (obj.dynamic)
	{
		bool foundGround = false;
		for (auto& layer : gs.layers)
		{
			for (auto& other : layer)
			{
				if (&other != &obj)
				{
					checkCollision(res, obj, other);

					if (obj.gravityEnabled && other.type == ObjectType::level)
					{
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
						if (SDL_GetRectIntersectionFloat(&sensor, &rectB, &rectC)
							&& rectC.w > 0.0f && rectC.h > 0.0f)
						{
							foundGround = true;
						}
					}
				}
			}
		}
		if (obj.grounded != foundGround) obj.grounded = foundGround;

	}

	// guard against edge case for being suspended 1px away from ground in permanent jumping state
	if (obj.type == ObjectType::player && obj.grounded && obj.data.player.state == PlayerState::jumping && obj.velocity.y >= 0)
	{
		obj.data.player.state = obj.direction != 0 ? PlayerState::running : PlayerState::idle;
	}
}

void collisionResponse(const Resources& res, GameObject& objA, GameObject& objB, const SDL_FRect& rectC)
{
	const auto genericResponse = [&]() {
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
		};

	if (objA.type == ObjectType::player)
	{
		switch (objB.type)
		{
			case ObjectType::level:
			{
				genericResponse();
				break;
			}
		}
	}
	else if (objA.type == ObjectType::bullet)
	{
		switch (objA.data.bullet.state)
		{
			case BulletState::moving:
			{
				genericResponse();
				objA.velocity *= 0;
				objA.data.bullet.state = BulletState::colliding;
				objA.currentAnimation = res.ANIM_BULLET_HIT;
				objA.texture = res.bulletHitTex;
				break;
			}
		}
	}
}

void checkCollision(const Resources& res, GameObject& objA, GameObject& objB)
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
		collisionResponse(res, objA, objB, rectC);
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
							player.gravityEnabled = true;
							gs.layers[LAYER_IDX_CHARACTERS].push_back(player);
							gs.playerIndex = gs.layers[LAYER_IDX_CHARACTERS].size() - 1;
							break;
						}
						case 5: // grass
						{
							GameObject grass = createObject(r, c, ObjectType::level, res.grassTex);
							gs.foregroundTiles.push_back(grass);
							break;
						}
						case 6: // brick
						{
							GameObject brick = createObject(r, c, ObjectType::level, res.brickTex);
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

void handleMouseInput(GameState& gs, const Resources& res, GameObject& obj, uint8_t button, bool isPressed)
{
	if (obj.type == ObjectType::player)
	{
		if (button == SDL_BUTTON_LEFT) gs.getPlayer().data.player.isShooting = isPressed;
	}
}

void handleShooting(GameState& gs, const Resources& res, GameObject& player)
{
	if (!player.data.player.isShooting) return;
	if (player.data.player.isShooting && !player.data.player.shootTimer.isTimeout()) return;

	player.data.player.shootTimer.reset();

	auto itr = std::find_if(gs.bullets.begin(), gs.bullets.end(),
		[&gs](const GameObject& bullet) {
			return bullet.data.bullet.state == BulletState::inactive;
		});

	GameObject* b = nullptr;
	if (itr != gs.bullets.end()) // found inactive bullet from pool
	{
		b = &(*itr);
		b->velocity = glm::vec2(0);
		b->data.bullet.state = BulletState::moving;
	}
	else // no inactive bullet found from pool
	{
		GameObject newBullet;
		newBullet.type = ObjectType::bullet;
		newBullet.data.bullet = BulletData();
		newBullet.textureSize = 4;
		newBullet.collider = {
			.x = 0, .y = 0,
			.w = static_cast<float>(newBullet.textureSize),
			.h = static_cast<float>(newBullet.textureSize)
		};
		newBullet.animations = res.bulletAnims;
		newBullet.dynamic = true;
		newBullet.maxSpeed = glm::vec2(1000, 0);

		gs.bullets.push_back(newBullet);
		b = &gs.bullets.back();
	}

	float xPos = player.direction == 1 ? player.position.x + TILE_SIZE - 8 : player.position.x;
	b->position = glm::vec2(xPos, player.position.y + (TILE_SIZE / 2));
	b->direction = player.direction;
	const int yVariation = 40;
	float yVelocity = SDL_rand(yVariation) - yVariation / 2;
	b->velocity = glm::vec2(player.velocity.x + 600.f * b->direction, yVelocity);
	b->currentAnimation = res.ANIM_BULLET;
	b->animations[res.ANIM_BULLET_HIT].reset();
	b->texture = res.bulletTex;
}