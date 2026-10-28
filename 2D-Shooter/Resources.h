#pragma once

#include <filesystem>
#include <memory>
#include <vector>
#include "tmx.h"

struct SDL_Texture;
struct SDLState;
struct SDL_Renderer;
struct MIX_Audio;
struct MIX_Mixer;
struct Animation;
struct SpriteSheet;

struct TileSetTextures
{
	int firstgid;
	std::vector<SDL_Texture*> textures;
};

struct Resources
{
	const int ANIM_PLAYER_IDLE = 0;
	const int ANIM_PLAYER_RUN = 1;
	const int ANIM_PLAYER_SLIDE = 2;
	const int ANIM_PLAYER_SHOOT = 3;
	const int ANIM_PLAYER_SHOOTRUN = 4;
	const int ANIM_PLAYER_SHOOTSLIDE = 5;

	const int ANIM_ENEMY_IDLE = 0;
	const int ANIM_ENEMY_HIT = 1;
	const int ANIM_ENEMY_DIE = 2;

	const int ANIM_BULLET = 0;
	const int ANIM_BULLET_HIT = 1;

	std::vector<Animation> playerAnims;
	std::vector<Animation> enemyAnims;
	std::vector<Animation> bulletAnims;

	std::vector<SDL_Texture*> textures;
	SDL_Texture* bg1Tex, * bg2Tex, * bg3Tex, * bg4Tex;

	std::vector<MIX_Audio*> audios;
	MIX_Audio* audioShoot, * audioShootHit, * audioJump,
		* audioEnemyHit, * audioEnemyDie, * audioBgMusic;

	std::vector<TileSetTextures> tilesetTextures;
	std::unique_ptr<tmx::Map> map;

	~Resources();
	SDL_Texture* loadTexture(SDL_Renderer* renderer, const char* filePath);
	SpriteSheet* loadSheet(SDL_Renderer* renderer, const char* path, int frameCount);
	MIX_Audio* loadAudio(MIX_Mixer* mixer, const char* filePath);

	void load(SDLState& state);
	void unload(SDLState& state);
};