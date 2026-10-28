#pragma once

#include <filesystem>
#include <memory>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include "tmx.h"

struct SDLState;
struct Animation;
struct SpriteSheet;

struct TextureDeleter
{
	void operator()(SDL_Texture* tex) const noexcept
	{
		if (tex) SDL_DestroyTexture(tex);
	}
};

struct AudioDeleter
{
	void operator()(MIX_Audio* audio) const noexcept
	{
		if (audio) MIX_DestroyAudio(audio);
	}
};

struct TrackDeleter
{
	void operator()(MIX_Track* track) const noexcept
	{
		if (track) MIX_DestroyTrack(track);
	}
};

using TexturePtr = std::unique_ptr<SDL_Texture, TextureDeleter>;
using AudioPtr = std::unique_ptr<MIX_Audio, AudioDeleter>;
using TrackPtr = std::unique_ptr<MIX_Track, TrackDeleter>;

struct TileSetTextures
{
	int firstgid;
	std::vector<const SpriteSheet*> spriteSheets;
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

	std::vector<TexturePtr> textures;
	SDL_Texture* bg1Tex, * bg2Tex, * bg3Tex, * bg4Tex;

	std::vector<AudioPtr> audios;
	MIX_Audio* audioShoot, * audioShootHit, * audioJump,
		* audioEnemyHit, * audioEnemyDie, * audioBgMusic;

	std::vector<std::unique_ptr<SpriteSheet>> spriteSheets;

	std::vector<TileSetTextures> tilesetTextures;
	std::unique_ptr<tmx::Map> map;

	~Resources();
	SDL_Texture* loadTexture(SDL_Renderer* renderer, const char* filePath);
	const SpriteSheet* loadSheet(SDL_Renderer* renderer, const char* path, int frameCount);
	MIX_Audio* loadAudio(MIX_Mixer* mixer, const char* filePath);

	void load(SDLState& state);
	void unload(SDLState& state);
};