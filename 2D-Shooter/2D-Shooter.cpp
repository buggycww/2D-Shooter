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
#include "ProfilingStats.h"

void cleanupSDL(SDLState& state);
bool initialiseSDL(SDLState& state);
void drawObject(const SDLState& state, const GameState& gs, GameObject& obj, float deltaTime);
void drawHealthBars(const SDLState& state, const GameState& gs, const HealthBar& bar);
void update(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime);
void createTiles(const SDLState& state, GameState& gs, const Resources& res);
void collisionResponse(const SDLState& state, const Resources& res, GameObject& objA, GameObject& objB, glm::vec2& tentativePos,
	glm::vec2 surfaceNormal, glm::vec2& overlap, float deltaTime);
bool intersectAABB(const SDL_FRect& a, const SDL_FRect& b, glm::vec2& outOverlap);
void handleKeyInput(const SDLState& state, const Resources& res, GameObject& obj, SDL_Scancode key, bool keyDown);
void drawParallaxBackground(const SDLState& state, SDL_Texture* tex, float cameraX, float scrollFactor);
void handleMouseInput(GameState& gs, const Resources& res, GameObject& obj, uint8_t button, bool isPressed);
void handleShooting(const SDLState& state, GameState& gs, const Resources& res, GameObject& player);
void drawDebugOverlay(const SDLState& state, const GameState& gs, const Resources& res, const FrameStats& stats);


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
	const double perfFreq = static_cast<double>(SDL_GetPerformanceFrequency());
	uint64_t prevCounter = SDL_GetPerformanceCounter();
	FrameStats stats;

	TrackPtr bgMusicTrack(MIX_CreateTrack(state.mixer));
	MIX_SetTrackGain(bgMusicTrack.get(), 0.5f);
	MIX_SetTrackAudio(bgMusicTrack.get(), res.audioBgMusic);
	MIX_PlayTrack(bgMusicTrack.get(), 0);

	HealthBar healthBar;

	while (running)
	{
		const uint64_t frameStart = SDL_GetPerformanceCounter();
		const float deltaTime = static_cast<float>((frameStart - prevCounter) / perfFreq);
		prevCounter = frameStart;

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

		const uint64_t afterEvents = SDL_GetPerformanceCounter();

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

		const uint64_t afterUpdate = SDL_GetPerformanceCounter();

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

		// draw healthbars
		drawHealthBars(state, gs, healthBar);

		const uint64_t afterDraw = SDL_GetPerformanceCounter();
		stats.Add(deltaTime,
			static_cast<float>((afterUpdate - afterEvents) / perfFreq),
			static_cast<float>((afterDraw - afterUpdate) / perfFreq));

		// display debug info
		if (gs.debugMode) drawDebugOverlay(state, gs, res, stats);

		// swap buffers and present
		SDL_RenderPresent(state.renderer);
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
	const SpriteSheet* sheet = obj.currentAnimation != -1
		? obj.animations[obj.currentAnimation].spriteSheet
		: obj.getSpriteSheet();

	float frameX = obj.currentAnimation != -1 
		? obj.animations[obj.currentAnimation].currentFrame()
		: obj.spriteFrame - 1;

	SDL_FRect srcRect
	{
		.x = frameX * sheet->frameWidth,
		.y = 0,
		.w = static_cast<float>(sheet->frameWidth),
		.h = static_cast<float>(sheet->frameHeight)
	};

	SDL_FRect dstRect
	{                                
		.x = obj.position.x - gs.mapViewport.x,
		.y = obj.position.y - gs.mapViewport.y,
		.w = static_cast<float>(sheet->frameWidth),
		.h = static_cast<float>(sheet->frameHeight)
	};

	SDL_FlipMode flipMode = obj.direction == -1 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
	if (!obj.shouldFlash) 
	{
		SDL_RenderTextureRotated(state.renderer, sheet->texture, &srcRect, &dstRect, 0, nullptr, flipMode);
	}
	else
	{
		// flash
		SDL_SetTextureColorModFloat(sheet->texture, 2.5f, 1.f, 1.f);
		SDL_RenderTextureRotated(state.renderer, sheet->texture, &srcRect, &dstRect, 0, nullptr, flipMode);
		SDL_SetTextureColorModFloat(sheet->texture, 1.f, 1.f, 1.f);

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

void drawHealthBars(const SDLState& state, const GameState& gs, const HealthBar& bar)
{
	for (const auto& layer : gs.layers)
	{
		for (const auto& obj : layer)
		{
			if (obj.type != ObjectType::enemy) continue;

			const CharacterData* character = &obj.data.enemy;

			if (!character || !obj.getSpriteSheet() || character->isDead) continue;

			const glm::vec2 topLeft{
				obj.position.x - gs.mapViewport.x,
				obj.position.y - gs.mapViewport.y
			};

			bar.Draw(state.renderer, character->health, character->maxHealth, topLeft, static_cast<float>(obj.getSpriteSheet()->frameWidth));
		}
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

					obj.playAnimation(obj.data.player.isShooting ? res.ANIM_PLAYER_SHOOT : res.ANIM_PLAYER_IDLE);
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
						obj.playAnimation(obj.data.player.isShooting ? res.ANIM_PLAYER_SHOOTSLIDE : res.ANIM_PLAYER_SLIDE);
					}
					else
					{
						// preserve run cycle phase
						obj.playAnimation( obj.data.player.isShooting ? res.ANIM_PLAYER_SHOOTRUN : res.ANIM_PLAYER_RUN, true);
					}

					break;
				}
				case PlayerState::jumping:
				{
					obj.playAnimation(res.ANIM_PLAYER_RUN);
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
						obj.playAnimation(res.ANIM_ENEMY_IDLE);
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

	// physics and collision detection
	const float maxSpeed = obj.maxSpeed.x;
	const float targetSpeed = currentDirection * maxSpeed;

	if (std::abs(obj.velocity.x) > maxSpeed) // velocity above maxspeed is knockback, since input is clamped
	{
		// decay knockback toward max speed at DRAG
		const float s = std::copysign(1.f, obj.velocity.x);
		obj.velocity.x = s * std::max(std::abs(obj.velocity.x) - DRAG * deltaTime, maxSpeed);

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

	// add gravity
	if (obj.gravityEnabled && !obj.grounded)
	{
		obj.velocity.y += GRAVITY * deltaTime;
	}

	glm::vec2 tentativePos = obj.position;
	bool contacts[4] = { false };
	const int RIGHT_CONTACT = 0;
	const int LEFT_CONTACT = 1;
	const int BOTTOM_CONTACT = 2;
	const int TOP_CONTACT = 3;

	for (int axis = 0; axis < 2; ++axis)
	{
		tentativePos[axis] += obj.velocity[axis] * deltaTime;
		for (auto& layer : gs.layers)
		{
			for (auto& objB : layer)
			{
				if (&obj == &objB || !obj.dynamic) continue;
				if (objB.collider.w == 0.f || objB.collider.h == 0.f) continue;

				const SDL_FRect rectA{
					.x = tentativePos.x + obj.collider.x,
					.y = tentativePos.y + obj.collider.y,
					.w = obj.collider.w,
					.h = obj.collider.h
				};
				const SDL_FRect rectB{
					.x = objB.position.x + objB.collider.x,
					.y = objB.position.y + objB.collider.y,
					.w = objB.collider.w,
					.h = objB.collider.h
				};

				glm::vec2 overlap;
				if (!intersectAABB(rectA, rectB, overlap)) continue;
				if (overlap[axis] <= 0.001f) continue; // ignore sub-pixel skin contact; otherwise resting on the floor
														// can be interpreted as a horizontal wall collision.
				
				glm::vec2 surfaceNormal{ 0.f };
				if (axis == 0)
				{
					const float aCenter = rectA.x + rectA.w * 0.5f;
					const float bCenter = rectB.x + rectB.w * 0.5f;
					const bool aIsBeforeB = (obj.velocity.x != 0.f) ? (obj.velocity.x > 0.f) : (aCenter < bCenter);

					if (aIsBeforeB)
					{
						surfaceNormal = { -1.f, 0.f };
						contacts[RIGHT_CONTACT] = true;
					}
					else
					{
						surfaceNormal = { 1.f, 0.f };
						contacts[LEFT_CONTACT] = true;
					}
				}
				else
				{
					const float aCenter = rectA.y + rectA.h * 0.5f;
					const float bCenter = rectB.y + rectB.h * 0.5f;
					const bool aIsBeforeB = (obj.velocity.y != 0.f) ? (obj.velocity.y > 0.f) : (aCenter < bCenter);

					if (aIsBeforeB)
					{
						surfaceNormal = { 0.f, -1.f };
						contacts[BOTTOM_CONTACT] = true;
					}
					else
					{
						surfaceNormal = { 0.f, 1.f };
						contacts[TOP_CONTACT] = true;
					}
				}

				collisionResponse(state, res, obj, objB, tentativePos, surfaceNormal, overlap, deltaTime);
			}
		}
	}

	obj.position = tentativePos;

	// check grounded using sensor
	obj.grounded = false;
	if (obj.gravityEnabled && obj.velocity.y >= 0.f)
	{
		const SDL_FRect sensor{
			.x = obj.position.x + obj.collider.x,
			.y = obj.position.y + obj.collider.y + obj.collider.h,
			.w = obj.collider.w,
			.h = 1.f
		};

		for (const auto& layer : gs.layers)
		{
			for (const auto& objB : layer)
			{
				if (&obj == &objB) continue;
				if (objB.type != ObjectType::level) continue;
				if (objB.collider.w == 0.f || objB.collider.h == 0.f) continue;

				const SDL_FRect rectB{
					.x = objB.position.x + objB.collider.x,
					.y = objB.position.y + objB.collider.y,
					.w = objB.collider.w,
					.h = objB.collider.h
				};

				glm::vec2 overlap;
				if (intersectAABB(sensor, rectB, overlap))
				{
					obj.grounded = true;
					obj.velocity.y = 0.f;
					contacts[BOTTOM_CONTACT] = true;
					break;
				}
			}

			if (obj.grounded) break;
		}
	}

	if (contacts[BOTTOM_CONTACT] && !obj.contacts[BOTTOM_CONTACT])
	{
		// landed event
		if (obj.type == ObjectType::player)
		{
			obj.data.player.state = obj.direction != 0 ? PlayerState::running : PlayerState::idle;
		}
	}
	else if (obj.contacts[BOTTOM_CONTACT] && !contacts[BOTTOM_CONTACT])
	{
		// in air event
	}

	// update object's contacts
	for (int i = 0; i < 4; i++) obj.contacts[i] = contacts[i];
}

bool intersectAABB(const SDL_FRect& a, const SDL_FRect& b, glm::vec2& outOverlap)
{
	const float minXA = a.x;
	const float maxXA = a.x + a.w;
	const float minYA = a.y;
	const float maxYA = a.y + a.h;

	const float minXB = b.x;
	const float maxXB = b.x + b.w;
	const float minYB = b.y;
	const float maxYB = b.y + b.h;

	if (minXA < maxXB && minXB < maxXA && minYA < maxYB && minYB < maxYA)
	{
		outOverlap.x = std::min(maxXA, maxXB) - std::max(minXA, minXB);
		outOverlap.y = std::min(maxYA, maxYB) - std::max(minYA, minYB);
		return true;
	}

	return false;
}

void collisionResponse(const SDLState& state, const Resources& res, GameObject& objA, GameObject& objB, glm::vec2& tentativePos, 
	glm::vec2 surfaceNormal, glm::vec2& overlap, float deltaTime)
{
	auto resolveCollision = [&]() {
		tentativePos += surfaceNormal * overlap;
	};

	auto clearVelocity = [&]() {
		if (surfaceNormal.x != 0)
		{
			objA.velocity.x = 0;
		}
		else if (surfaceNormal.y != 0)
		{
			objA.velocity.y = 0;
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
					clearVelocity();
					resolveCollision();
					break;
				}
				case ObjectType::enemy:
				{
					if (objB.data.enemy.isDead) break;

					clearVelocity();
					resolveCollision();

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
								objB.playAnimation(res.ANIM_ENEMY_DIE);
								MIX_PlayAudio(state.mixer, res.audioEnemyDie);
							}
							else
							{
								objB.data.enemy.state = EnemyState::hit;
								objB.playAnimation(res.ANIM_ENEMY_HIT);
							}
							break;
						}
					}

					if (!passThrough)
					{
						objA.dynamic = false;
						objA.data.bullet.state = BulletState::colliding;
						objA.playAnimation(res.ANIM_BULLET_HIT);
						clearVelocity();
						resolveCollision();
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
					clearVelocity();
					resolveCollision(); 
					break;
				}
			}
			break;
		}

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
								return tGid >= tst.firstgid && tGid < tst.firstgid + tst.spriteSheets.size();
							});
						if (itr == res.tilesetTextures.end()) continue; // no tileset has this gid

						const SpriteSheet* spriteSheet = itr->spriteSheets[tGid - itr->firstgid];

						newLayer.push_back(factory.makeTile(glm::vec2(c * res.map->tileWidth, r * res.map->tileHeight), spriteSheet, layer.name == "Level"));
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

void drawDebugOverlay(const SDLState& state, const GameState& gs, const Resources& res, const FrameStats& stats)
{
	if (!gs.hasPlayer()) return;

	const GameObject& player =
		gs.layers[gs.playerLayer][gs.playerIndex];

	int activeBullets = 0;
	for (const GameObject& bullet : gs.bullets)
		if (bullet.data.bullet.state != BulletState::inactive)
			++activeBullets;

	int enemies = 0;
	int objects = 0;
	for (const auto& layer : gs.layers)
	{
		objects += static_cast<int>(layer.size());
		for (const GameObject& obj : layer)
			if (obj.type == ObjectType::enemy && !obj.data.enemy.isDead)
				++enemies;
	}

	static constexpr const char* stateNames[] =
	{
		"idle", "running", "jumping"
	};
	const int stateIndex = static_cast<int>(player.data.player.state);
	const char* stateName =
		(stateIndex >= 0 && stateIndex < 3) ? stateNames[stateIndex] : "?";

	const std::array<std::string, 6> lines{
		std::format("FPS {:6.1f}   frame {:6.2f} ms   worst {:6.2f} ms",
			stats.fps, stats.frameMs, stats.worstFrameMs),
		std::format("update {:5.2f} ms   draw {:5.2f} ms",
			stats.updateMs, stats.drawMs),
		std::format("player ({:7.1f},{:7.1f})   vel ({:6.1f},{:6.1f})",
			player.position.x, player.position.y,
			player.velocity.x, player.velocity.y),
		std::format("state {:<7} grounded {:<3} hp {:.0f}/{:.0f}",
			stateName, player.grounded ? "yes" : "no",
			player.data.player.health, player.data.player.maxHealth),
		std::format("bullets {}/{}   enemies {}   objects {}",
			activeBullets, gs.bullets.size(), enemies, objects),
		std::format("camera ({:.1f},{:.1f})   map {}x{}",
			gs.mapViewport.x, gs.mapViewport.y,
			res.map->mapWidth, res.map->mapHeight)
	};

	const SDL_FRect panel{
		2.f, 2.f, 460.f, static_cast<float>(lines.size()) * 11.f + 4.f
	};

	SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, 170);
	SDL_RenderFillRect(state.renderer, &panel);
	SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_NONE);

	SDL_SetRenderDrawColor(state.renderer, 235, 235, 235, 255);

	float y = 6.f;
	for (const std::string& line : lines)
	{
		SDL_RenderDebugText(state.renderer, 6.f, y, line.c_str());
		y += 11.f;
	}
}
