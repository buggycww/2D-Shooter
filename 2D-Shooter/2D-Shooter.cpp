// 2D-Shooter.cpp : Defines the entry point for the application.
//

#include "2D-Shooter.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <vector>
#include <array>
#include <format>
#include <algorithm>
#include <filesystem>

#include "AssetPaths.h"
#include "GameObject.h"
#include "GameObjectFactory.h"
#include "GlobalConstants.h"
#include "GameStates.h"
#include "Resources.h"

void cleanupSDL(SDLState& state);
bool initialiseSDL(SDLState& state);
void drawObject(const SDLState& state, const GameState& gs, GameObject& obj, float deltaTime);
void update(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime);
void createTiles(const SDLState& state, GameState& gs, const Resources& res);
void checkCollision(const SDLState& state, const Resources& res, GameObject& objA, GameObject& objB);
void handleKeyInput(const SDLState& state, const Resources& res, GameObject& obj, SDL_Scancode key, bool keyDown);
void drawParallaxBackground(const SDLState& state, SDL_Texture* tex, float cameraX, float scrollFactor);
void handleMouseInput(GameState& gs, const Resources& res, GameObject& obj, uint8_t button, bool isPressed);
void handleShooting(const SDLState& state, GameState& gs, const Resources& res, GameObject& player);

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

	if (!res.map)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error",
			"Could not load data/maps/smallmap.tmx (run the game from the project root)", state.window);
		res.unload(state);
		cleanupSDL(state);
		return 1;
	}

	// setup game data
	GameState gs(state);
	createTiles(state, gs, res);

	if (!gs.hasPlayer())
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error",
			"The map does not contain a Player object", state.window);
		res.unload(state);
		cleanupSDL(state);
		return 1;
	}

	// setup viewport
	gs.maxViewportPos = glm::vec2(
		res.map->mapWidth * TILE_SIZE - state.logW,
		res.map->mapHeight * res.map->tileHeight - gs.mapViewport.h
	);
	gs.mapViewport.y = gs.maxViewportPos.y;

	// game loop
	bool running = true;
	uint64_t prevTime = SDL_GetTicks();

	MIX_Track* bgMusicTrack = MIX_CreateTrack(state.mixer);
	MIX_SetTrackGain(bgMusicTrack, 0.5f);
	MIX_SetTrackAudio(bgMusicTrack, res.audioBgMusic);
	MIX_PlayTrack(bgMusicTrack, 0);

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
					handleKeyInput(state, res, gs.getPlayer(), event.key.scancode, true);
					break;
				}
				case SDL_EVENT_KEY_UP:
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

					if (event.key.scancode == SDL_SCANCODE_F11)
					{
						state.fullscreen = !state.fullscreen;
						SDL_SetWindowFullscreen(state.window, state.fullscreen);
						break;
					}

					handleKeyInput(state, res, gs.getPlayer(), event.key.scancode, false);
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
				if (obj.dynamic) update(state, gs, res, obj, deltaTime);
			}
		}

		for (auto& bullet : gs.bullets)
		{
			update(state, gs, res, bullet, deltaTime);
		}

		// calculate viewport position
		gs.mapViewport.x = (gs.getPlayer().position.x + TILE_SIZE / 2) - (gs.mapViewport.w / 2);
		gs.mapViewport.x = std::clamp(gs.mapViewport.x, 0.f, gs.maxViewportPos.x);

		// viewport.y upper and lower band that player is allowed to move within without affecting camera
		const float topEdge = gs.mapViewport.y + gs.mapViewport.h * 0.35f;
		const float botEdge = gs.mapViewport.y + gs.mapViewport.h * 0.65f;
		const float playerCenterY = gs.getPlayer().position.y + TILE_SIZE / 2.f;

		float targetY = gs.mapViewport.y;   // default: don't move

		if (playerCenterY < topEdge) {
			// player below band
			targetY = playerCenterY - gs.mapViewport.h * 0.35f;
		}
		else if (playerCenterY > botEdge) {
			// player above band
			targetY = playerCenterY - gs.mapViewport.h * 0.65f;
		}

		targetY = std::clamp(targetY, 0.f, gs.maxViewportPos.y);

		// lerp
		const float t = 1.f - std::exp(-20.f * deltaTime);
		gs.mapViewport.y += (targetY - gs.mapViewport.y) * t;

		// perform drawing commands
		SDL_SetRenderDrawColor(state.renderer, 20, 10, 30, 255);
		SDL_RenderClear(state.renderer);

		// draw background
		SDL_RenderTexture(state.renderer, res.bg1Tex, nullptr, nullptr);
		drawParallaxBackground(state, res.bg4Tex, gs.mapViewport.x, 0.075f);
		drawParallaxBackground(state, res.bg3Tex, gs.mapViewport.x, 0.150f);
		drawParallaxBackground(state, res.bg2Tex, gs.mapViewport.x, 0.3f);

		// draw all objects
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

		// display debug info
		if (gs.debugMode)
		{
			SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
			SDL_RenderDebugText(state.renderer, 5, 5,
				std::format("State: {}, Grounded: {}, Bullets: {}, xVelocity: {}",
					static_cast<int>(gs.getPlayer().data.player.state),
					gs.getPlayer().grounded, gs.bullets.size(), res.map->mapWidth
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

	if (!MIX_Init())
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error initialising MIX", nullptr);
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

	// create mixer
	state.mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
	if (!state.mixer)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", ("Error creating mixer: %s", SDL_GetError()), nullptr);
		cleanupSDL(state);
		return false;
	}

	MIX_SetMixerGain(state.mixer, 0.5f);

	return true;
}

void cleanupSDL(SDLState& state)
{
	SDL_DestroyRenderer(state.renderer);
	SDL_DestroyWindow(state.window);
	MIX_DestroyMixer(state.mixer);
	MIX_Quit();
	SDL_Quit();
}

void drawObject(const SDLState& state, const GameState& gs, GameObject& obj, float deltaTime)
{
	float frameX = obj.currentAnimation != -1 
		? obj.animations[obj.currentAnimation].currentFrame()
		: obj.spriteFrame - 1;

	SDL_FRect srcRect
	{
		.x = frameX * obj.sprite->frameWidth,
		.y = 0,
		.w = static_cast<float>(obj.sprite->frameWidth),
		.h = static_cast<float>(obj.sprite->frameHeight)
	};

	SDL_FRect dstRect
	{                                
		.x = obj.position.x - gs.mapViewport.x,
		.y = obj.position.y - gs.mapViewport.y,
		.w = static_cast<float>(obj.sprite->frameWidth),
		.h = static_cast<float>(obj.sprite->frameHeight)
	};

	SDL_FlipMode flipMode = obj.direction == -1 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
	if (!obj.shouldFlash) 
	{
		SDL_RenderTextureRotated(state.renderer, obj.sprite->texture, &srcRect, &dstRect, 0, nullptr, flipMode);
	}
	else
	{
		// flash
		SDL_SetTextureColorModFloat(obj.sprite->texture, 2.5f, 1.f, 1.f);
		SDL_RenderTextureRotated(state.renderer, obj.sprite->texture, &srcRect, &dstRect, 0, nullptr, flipMode);
		SDL_SetTextureColorModFloat(obj.sprite->texture, 1.f, 1.f, 1.f);

		obj.flashTimer.step(deltaTime);
		if (obj.flashTimer.isTimeout()) obj.shouldFlash = false;
	}

	if (gs.debugMode)
	{
		SDL_FRect rect{
		.x = obj.position.x + obj.collider.x - gs.mapViewport.x,
		.y = obj.position.y + obj.collider.y - gs.mapViewport.y,
		.w = obj.collider.w,
		.h = obj.collider.h
		};

		SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(state.renderer, 255, 0, 0, 150);
		SDL_RenderFillRect(state.renderer, &rect);
		SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_NONE);
	}
}

void drawParallaxBackground(const SDLState& state, SDL_Texture* tex, float cameraX, float scrollFactor)
{
	const float w = static_cast<float>(tex->w);
	float x = -cameraX * scrollFactor; 
	if (x <= -tex->w)
	{
		x = 0;
	}

	SDL_FRect dstRect
	{
		.x = x,
		.y = 32,
		.w = w * 2.f,
		.h = static_cast<float>(tex->h)
	};

	SDL_RenderTextureTiled(state.renderer, tex, nullptr, 1, &dstRect);
}

void update(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime)
{
	// update animations
	if (obj.currentAnimation != -1)
	{
		obj.animations[obj.currentAnimation].step(deltaTime);
	}

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
			obj.data.player.hitTimer.step(deltaTime);
			handleShooting(state, gs, res, obj);

			switch (obj.data.player.state)
			{
				case PlayerState::idle:
				{
					// switching to running state
					if (currentDirection != 0)
					{
						obj.data.player.state = PlayerState::running;
					}

					obj.currentAnimation = obj.data.player.isShooting ? res.ANIM_PLAYER_SHOOT : res.ANIM_PLAYER_IDLE;
					obj.sprite = obj.animations[obj.currentAnimation].spriteSheet;
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
						obj.sprite = obj.animations[obj.currentAnimation].spriteSheet;
					}
					else
					{
						obj.currentAnimation = res.ANIM_PLAYER_RUN; // use same index to keep run cycle consistent
						obj.sprite = obj.animations[obj.currentAnimation].spriteSheet;
					}

					break;
				}
				case PlayerState::jumping:
				{
					obj.currentAnimation = res.ANIM_PLAYER_RUN;
					obj.sprite = obj.animations[obj.currentAnimation].spriteSheet;
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
		case ObjectType::enemy:
		{
			switch (obj.data.enemy.state)
			{
				case EnemyState::idle:
				{
					glm::vec2 playerDir = gs.getPlayer().position - obj.position;
					if (glm::length(playerDir) < 100) obj.data.enemy.state = EnemyState::shambling;

					break;
				}
				case EnemyState::shambling:
				{
					glm::vec2 playerDir = gs.getPlayer().position - obj.position;
					if (glm::length(playerDir) < 100)
					{
						currentDirection = playerDir.x < 0 ? -1 : 1;
						obj.acceleration = glm::vec2(100.f, 0);
					}
					else obj.data.enemy.state = EnemyState::idle;

					break;
				}
				case EnemyState::hit:
				{
					obj.data.enemy.hitTimer.step(deltaTime);

					if (obj.data.enemy.hitTimer.isTimeout())
					{
						obj.data.enemy.state = EnemyState::idle;
						obj.currentAnimation = res.ANIM_ENEMY_IDLE;
						obj.sprite = obj.animations[obj.currentAnimation].spriteSheet;
					}
					break;
				}
				case EnemyState::dead:
				{
					obj.velocity.x = 0;
					if (obj.currentAnimation != -1 && obj.animations[obj.currentAnimation].isDone())
					{
						obj.spriteFrame = 18;
						obj.currentAnimation = -1;
					}

					break;
				}
			}
			break;
		}
	}

	if (currentDirection != 0) obj.direction = currentDirection;

	const float maxSpeed = obj.maxSpeed.x;
	const float targetSpeed = currentDirection * maxSpeed;

	if (std::abs(obj.velocity.x) > maxSpeed) // velocity above maxspeed is knockback, since input is clamped
	{
		// decay knockback toward max speed at DRAG
		const float s = std::copysign(1.f, obj.velocity.x);
		obj.velocity.x = s * std::max(std::abs(obj.velocity.x) /*- DRAG */* deltaTime, maxSpeed);

		// let input offset knockback
		if (currentDirection != 0 && obj.velocity.x * currentDirection < 0)
			obj.velocity.x += obj.acceleration.x * currentDirection * deltaTime;
	}
	else
	{
		// add acceleration to current velocity (capped at max speed)
		const float step = obj.acceleration.x * deltaTime;
		if (obj.velocity.x < targetSpeed) obj.velocity.x = std::min(obj.velocity.x + step, targetSpeed);
		else if (obj.velocity.x > targetSpeed) obj.velocity.x = std::max(obj.velocity.x - step, targetSpeed);
	}

	// add resultant velocity
	obj.position += obj.velocity * deltaTime;

	// handle collision detection for dynamic objects
	if (obj.dynamic)
	{
		bool foundGround = false;
		for (auto& layer : gs.layers)
		{
			for (auto& other : layer)
			{
				if (&other != &obj && other.collider.w != 0 && other.collider.h != 0)
				{
					checkCollision(state, res, obj, other);

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

void collisionResponse(const SDLState& state, const Resources& res, GameObject& objA, GameObject& objB, const SDL_FRect& rectC)
{
	const auto genericResponse = [&]() {
			if (rectC.h < rectC.w) // vertical collision
			{
				const float aCenterY = objA.position.y + objA.collider.y + objA.collider.h * 0.5f;
				const float bCenterY = objB.position.y + objB.collider.y + objB.collider.h * 0.5f;
				objA.position.y += aCenterY < bCenterY ? -rectC.h : rectC.h; // going down : going up

				objA.velocity.y = 0;
			}
			else // horizontal collision
			{
				const float aCenterX = objA.position.x + objA.collider.x + objA.collider.w * 0.5f;
				const float bCenterX = objB.position.x + objB.collider.x + objB.collider.w * 0.5f;
				objA.position.x += aCenterX < bCenterX ? -rectC.w : rectC.w; // going right : going left

				objA.velocity.x = 0;
			}
		};

	switch (objA.type)
	{
		case ObjectType::player:
		{
			switch (objB.type)
			{
				case ObjectType::level:
				{
					genericResponse();
					break;
				}
				case ObjectType::enemy:
				{
					if (objB.data.enemy.isDead) break;

					genericResponse();
					if (objA.data.player.hitTimer.isTimeout())
					{
						objA.data.player.hitTimer.reset();
						float direction = objA.position.x < objB.position.x ? -1 : 1;
						objA.velocity.x += direction * KNOCKBACK;
						MIX_PlayAudio(state.mixer, res.audioEnemyHit);
					}

					objA.shouldFlash = true;
					objA.flashTimer.reset();
					break;
				}
			}
			break;
		}
		case ObjectType::bullet:
		{
			switch (objA.data.bullet.state)
			{
				case BulletState::moving:
				{
					bool passThrough = false;
					switch (objB.type)
					{
						case ObjectType::level:
						{
							MIX_PlayAudio(state.mixer, res.audioShootHit);
							break;
						}
						case ObjectType::enemy:
						{
							if (objB.data.enemy.isDead)
							{
								passThrough = true;
								break;
							}

							MIX_PlayAudio(state.mixer, res.audioEnemyHit);
							objB.direction = -objA.direction;
							objB.shouldFlash = true;
							objB.flashTimer.reset();
							objB.data.enemy.hitTimer.reset();
							objB.data.enemy.Damage(BULLET_DMG);
							if (objB.data.enemy.isDead)
							{
								objB.data.enemy.state = EnemyState::dead;
								objB.currentAnimation = res.ANIM_ENEMY_DIE;
								objB.sprite = objB.animations[objB.currentAnimation].spriteSheet;
								MIX_PlayAudio(state.mixer, res.audioEnemyDie);
							}
							else
							{
								objB.data.enemy.state = EnemyState::hit;
								objB.currentAnimation = res.ANIM_ENEMY_HIT;
								objB.sprite = objB.animations[objB.currentAnimation].spriteSheet;
							}
							break;
						}
					}

					if (!passThrough)
					{
						genericResponse();
						objA.velocity *= 0;
						objA.data.bullet.state = BulletState::colliding;
						objA.currentAnimation = res.ANIM_BULLET_HIT;
						objA.sprite = objA.animations[objA.currentAnimation].spriteSheet;
					}
					break;
				}
			}
			break;
		}
		case ObjectType::enemy:
		{
			switch (objB.type)
			{
				case ObjectType::level:
				{
					genericResponse();
					break;
				}
			}
			break;
		}

	}
}

void checkCollision(const SDLState& state, const Resources& res, GameObject& objA, GameObject& objB)
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
		collisionResponse(state, res, objA, objB, rectC);
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

	struct LayerVisitor
	{
		GameState& gs;
		const Resources& res;
		GameObjectFactory factory;

		LayerVisitor(GameState& gs, const Resources& res) : gs(gs), res(res), factory(res) {}

		void operator()(tmx::Layer& layer)
		{
			std::vector<GameObject> newLayer;
			for (int r = 0; r < res.map->mapHeight; r++)
			{
				for (int c = 0; c < res.map->mapWidth; c++)
				{
					const size_t index = static_cast<size_t>(r) * res.map->mapWidth + c;
					if (index >= layer.data.size()) continue;

					const int tGid = layer.data[index];
					if (tGid) // if not empty slot
					{
						const auto itr = std::find_if(res.tilesetTextures.begin(), res.tilesetTextures.end(),
							[tGid](const TileSetTextures& tst) {
								return tGid >= tst.firstgid && tGid < tst.firstgid + tst.textures.size();
							});
						if (itr == res.tilesetTextures.end()) continue; // no tileset has this gid

						SDL_Texture* tex = itr->textures[tGid - itr->firstgid];

						newLayer.push_back(factory.makeTile(glm::vec2(c * res.map->tileWidth, r * res.map->tileHeight), tex, layer.name == "Level"));
					}
				}
			}
			gs.layers.push_back(std::move(newLayer));
		}

		void operator()(tmx::ObjectGroup& objectGroup)
		{
			std::vector<GameObject> newLayer;
			for (tmx::LayerObject& obj : objectGroup.objects)
			{
				glm::vec2 objPos(
					obj.x - res.map->tileWidth / 2,
					obj.y - res.map->tileHeight / 2
				);

				if (obj.type == "Player")
				{
					newLayer.push_back(factory.makePlayer(objPos));
					gs.playerLayer = static_cast<int>(gs.layers.size());
					gs.playerIndex = static_cast<int>(newLayer.size()) - 1;
				}
				else if (obj.type == "Enemy")
				{
					newLayer.push_back(factory.makeEnemy(objPos));
				}
			}
			gs.layers.push_back(std::move(newLayer));
		}
	};

	LayerVisitor visitor(gs, res);
	for (auto& layer : res.map->layers)
	{
		std::visit(visitor, layer);
	}
}

void handleKeyInput(const SDLState& state, const Resources& res, GameObject& obj, SDL_Scancode key, bool keyDown)
{
	if (obj.type == ObjectType::player)
	{
		switch (obj.data.player.state)
		{
			case PlayerState::idle:
			{
				if (key == SDL_SCANCODE_SPACE && keyDown && obj.grounded)
				{
					MIX_PlayAudio(state.mixer, res.audioJump);
					obj.data.player.state = PlayerState::jumping;
					obj.velocity.y = JUMP_FORCE;
				}
				break;
			}
			case PlayerState::running:
			{
				if (key == SDL_SCANCODE_SPACE && keyDown && obj.grounded)
				{
					MIX_PlayAudio(state.mixer, res.audioJump);
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

void handleShooting(const SDLState& state, GameState& gs, const Resources& res, GameObject& player)
{
	if (!player.data.player.isShooting) return;
	if (!player.data.player.shootTimer.isTimeout()) return;

	player.data.player.shootTimer.reset();

	const float xPos = player.direction > 0
		? player.position.x + static_cast<float>(TILE_SIZE) - 8.f
		: player.position.x;

	const glm::vec2 spawnPos(xPos, player.position.y + static_cast<float>(TILE_SIZE) * 0.5f);

	const int yVariation = 40;
	const float yVelocity = static_cast<float>(SDL_rand(yVariation) - yVariation / 2);

	const glm::vec2 velocity(player.velocity.x + 600.f * player.direction, yVelocity);

	GameObjectFactory factory(res);

	auto itr = std::find_if(
		gs.bullets.begin(), gs.bullets.end(),
		[](const GameObject& bullet) {
			return bullet.data.bullet.state == BulletState::inactive;
		});

	if (itr != gs.bullets.end()) factory.resetBullet(*itr, spawnPos, player.direction, velocity);
	else gs.bullets.push_back(factory.makeBullet(spawnPos, player.direction, velocity));

	MIX_PlayAudio(state.mixer, res.audioShoot);
}