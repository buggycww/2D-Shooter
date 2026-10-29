#include "Resources.h"
#include "SDL3_image/SDL_image.h"
#include "GameStates.h"
#include "Animation.h"
#include "AssetPaths.h"
#include <cassert>
#include "GlobalConstants.h"

Resources::~Resources() = default;

SDL_Texture* Resources::loadTexture(SDL_Renderer* renderer, const char* filePath)
{
	SDL_Texture* tex = IMG_LoadTexture(renderer, filePath);
	assert(tex); // crash instead of proceeding in unpredictable state
	SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
	textures.emplace_back(tex);
	return tex;
}

const SpriteSheet* Resources::loadSheet(SDL_Renderer* renderer, const char* path, int frameCount)
{
	SDL_Texture* tex = loadTexture(renderer, path);
	if (!tex) return nullptr;

	float w = 0, h = 0;
	SDL_GetTextureSize(tex, &w, &h);
	const int frameWidth = static_cast<int>(w) / frameCount;

	if (frameWidth * frameCount != static_cast<int>(w))
		SDL_Log("Sheet %s: width %.0f not divisible by %d frames", path, w, frameCount);

	auto sheet = std::make_unique<SpriteSheet>(
		SpriteSheet{ tex, frameWidth, static_cast<int>(h), frameCount });

	const SpriteSheet* result = sheet.get();
	spriteSheets.push_back(std::move(sheet));
	return result;
}

MIX_Audio* Resources::loadAudio(MIX_Mixer* mixer, const char* filePath)
{
	MIX_Audio* audio = MIX_LoadAudio(mixer, filePath, false);
	audios.emplace_back(audio);
	return audio;
}

void Resources::load(SDLState& state)
{
	// player animations
	playerAnims.resize(6);
	playerAnims[ANIM_PLAYER_IDLE] = Animation(8, 1.6f);
	playerAnims[ANIM_PLAYER_IDLE].spriteSheet = loadSheet(state.renderer, idleTexFilePath, 8);

	playerAnims[ANIM_PLAYER_SHOOT] = Animation(4, 0.8f);
	playerAnims[ANIM_PLAYER_SHOOT].spriteSheet = loadSheet(state.renderer, shootTexFilePath, 4);

	playerAnims[ANIM_PLAYER_RUN] = Animation(4, 0.5f);
	playerAnims[ANIM_PLAYER_RUN].spriteSheet = loadSheet(state.renderer, runTexFilePath, 4);

	playerAnims[ANIM_PLAYER_SHOOTRUN] = Animation(4, 0.5f);
	playerAnims[ANIM_PLAYER_SHOOTRUN].spriteSheet = loadSheet(state.renderer, shootRunTexFilePath, 4);

	playerAnims[ANIM_PLAYER_SLIDE] = Animation(1, 1.0f);
	playerAnims[ANIM_PLAYER_SLIDE].spriteSheet = loadSheet(state.renderer, slideTexFilePath, 1);

	playerAnims[ANIM_PLAYER_SHOOTSLIDE] = Animation(4, 1.0f);
	playerAnims[ANIM_PLAYER_SHOOTSLIDE].spriteSheet = loadSheet(state.renderer, shootSlideTexFilePath, 4);

	// enemy animations
	enemyAnims.resize(3);
	enemyAnims[ANIM_ENEMY_IDLE] = Animation(8, 1.6f);
	enemyAnims[ANIM_ENEMY_IDLE].spriteSheet = loadSheet(state.renderer, enemyTexFilePath, 8);

	enemyAnims[ANIM_ENEMY_HIT] = Animation(8, 1.0f);
	enemyAnims[ANIM_ENEMY_HIT].spriteSheet = loadSheet(state.renderer, enemyHitTexFilePath, 8);

	enemyAnims[ANIM_ENEMY_DIE] = Animation(18, 3.5f);
	enemyAnims[ANIM_ENEMY_DIE].spriteSheet = loadSheet(state.renderer, enemyDieTexFilePath, 18);

	// bullet animations
	bulletAnims.resize(2);
	bulletAnims[ANIM_BULLET] = Animation(4, 0.5f);
	bulletAnims[ANIM_BULLET].spriteSheet = loadSheet(state.renderer, bulletTexFilePath, 4);

	bulletAnims[ANIM_BULLET_HIT] = Animation(4, 0.2f);
	bulletAnims[ANIM_BULLET_HIT].spriteSheet = loadSheet(state.renderer, bulletHitTexFilePath, 4);

	// background textures
	bg1Tex = loadTexture(state.renderer, bg1FilePath);
	bg2Tex = loadTexture(state.renderer, bg2FilePath);
	bg3Tex = loadTexture(state.renderer, bg3FilePath);
	bg4Tex = loadTexture(state.renderer, bg4FilePath);

	// audios
	audioBgMusic = loadAudio(state.mixer, audioBgMusicFilePath);
	audioShoot = loadAudio(state.mixer, audioShootFilePath);
	audioShootHit = loadAudio(state.mixer, audioShootHitFilePath);
	audioEnemyHit = loadAudio(state.mixer, audioEnemyHitFilePath);
	audioEnemyDie = loadAudio(state.mixer, audioEnemyDieFilePath);
	audioJump = loadAudio(state.mixer, audioJumpFilePath);

	// map
	map = tmx::loadMap(smallMapFilePath);
	if (!map) return;

	for (tmx::TileSet& tileSet : map->tileSets)
	{
		TileSetTextures tst;
		tst.firstgid = tileSet.firstgid;
		tst.spriteSheets.reserve(tileSet.tiles.size());

		for (tmx::Tile& tile : tileSet.tiles)
		{
			const std::string imagePath = tilesPath + std::filesystem::path(tile.image.source).filename().string();
			const SpriteSheet* sheet = loadSheet(state.renderer, imagePath.c_str(), 1);
			tst.spriteSheets.push_back(sheet);
		}

		tilesetTextures.push_back(std::move(tst));
	}

	assert(TILE_SIZE == map->tileWidth);
}

void Resources::unload(SDLState& state)
{
	textures.clear();
	audios.clear();
	spriteSheets.clear();
}
