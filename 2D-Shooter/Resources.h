#pragma once

#include <filesystem>

#include "tmx.h"

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
	SDL_Texture* idleTex, * runTex, * slideTex, * shootTex, * shootRunTex, *shootSlideTex,
		* enemyTex, * enemyHitTex, * enemyDieTex,
		* bulletTex, * bulletHitTex,
		* bg1Tex, * bg2Tex, * bg3Tex, * bg4Tex;

	std::vector<MIX_Audio*> audios;
	MIX_Audio* audioShoot, * audioShootHit, * audioJump,
		* audioEnemyHit, * audioEnemyDie, * audioBgMusic;

	std::vector<TileSetTextures> tilesetTextures;
	std::unique_ptr<tmx::Map> map;

	SDL_Texture* loadTexture(SDL_Renderer* renderer, const char* filePath)
	{
		SDL_Texture* tex = IMG_LoadTexture(renderer, filePath);
		SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
		textures.push_back(tex);
		return tex;
	}

	MIX_Audio* loadAudio(MIX_Mixer* mixer, const char* filePath)
	{
		MIX_Audio* audio = MIX_LoadAudio(mixer, filePath, false);
		audios.push_back(audio);
		return audio;
	} 

	void load(SDLState& state)
	{
		playerAnims.resize(6);
		playerAnims[ANIM_PLAYER_IDLE] = Animation(8, 1.6f);
		playerAnims[ANIM_PLAYER_SHOOT] = Animation(4, 0.8f);
		playerAnims[ANIM_PLAYER_RUN] = Animation(4, 0.5f);
		playerAnims[ANIM_PLAYER_SHOOTRUN] = Animation(4, 0.5f);
		playerAnims[ANIM_PLAYER_SLIDE] = Animation(1, 1.0f);
		playerAnims[ANIM_PLAYER_SHOOTSLIDE] = Animation(1, 1.0f);
		idleTex = loadTexture(state.renderer, idleTexFilePath);
		shootTex = loadTexture(state.renderer, shootTexFilePath);
		runTex = loadTexture(state.renderer, runTexFilePath);
		shootRunTex = loadTexture(state.renderer, shootRunTexFilePath);
		slideTex = loadTexture(state.renderer, slideTexFilePath);
		shootSlideTex = loadTexture(state.renderer, shootSlideTexFilePath);

		enemyAnims.resize(3);
		enemyAnims[ANIM_ENEMY_IDLE] = Animation(8, 1.6f);
		enemyAnims[ANIM_ENEMY_HIT] = Animation(8, 1.0f);
		enemyAnims[ANIM_ENEMY_DIE] = Animation(18, 3.5f);
		enemyTex = loadTexture(state.renderer, enemyTexFilePath);
		enemyHitTex = loadTexture(state.renderer, enemyHitTexFilePath);
		enemyDieTex = loadTexture(state.renderer, enemyDieTexFilePath);

		bulletAnims.resize(2);
		bulletAnims[ANIM_BULLET] = Animation(4, 0.5f);
		bulletAnims[ANIM_BULLET_HIT] = Animation(4, 0.2f);
		bulletTex = loadTexture(state.renderer, bulletTexFilePath);
		bulletHitTex = loadTexture(state.renderer, bulletHitTexFilePath);

		bg1Tex = loadTexture(state.renderer, bg1FilePath);
		bg2Tex = loadTexture(state.renderer, bg2FilePath);
		bg3Tex = loadTexture(state.renderer, bg3FilePath);
		bg4Tex = loadTexture(state.renderer, bg4FilePath);

		audioBgMusic = loadAudio(state.mixer, audioBgMusicFilePath);
		audioShoot = loadAudio(state.mixer, audioShootFilePath);
		audioShootHit = loadAudio(state.mixer, audioShootHitFilePath);
		audioEnemyHit = loadAudio(state.mixer, audioEnemyHitFilePath);
		audioEnemyDie = loadAudio(state.mixer, audioEnemyDieFilePath);
		audioJump = loadAudio(state.mixer, audioJumpFilePath);

		map = tmx::loadMap(smallMapFilePath);
		if (!map) return;

		for (tmx::TileSet& tileSet : map->tileSets)
		{
			TileSetTextures tst;
			tst.firstgid = tileSet.firstgid;
			tst.textures.reserve(tileSet.tiles.size());

			for (tmx::Tile& tile : tileSet.tiles)
			{
				const std::string imagePath = tilesPath + std::filesystem::path(tile.image.source).filename().string();
				tst.textures.push_back(loadTexture(state.renderer, imagePath.c_str()));
			}

			tilesetTextures.push_back(std::move(tst));
		}

		TILE_SIZE = map->tileWidth;
	}

	void unload(SDLState& state)
	{
		for (SDL_Texture* tex : textures)
		{
			SDL_DestroyTexture(tex);
		}

		for (MIX_Audio* audio : audios)
		{
			MIX_DestroyAudio(audio);
		}
	}
};