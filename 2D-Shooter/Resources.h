#pragma once

struct Resources
{
	const int ANIM_PLAYER_IDLE = 0;
	const int ANIM_PLAYER_RUN = 1;
	const int ANIM_PLAYER_SLIDE = 2;
	const int ANIM_PLAYER_SHOOT = 3;
	const int ANIM_PLAYER_SHOOTRUN = 4;
	const int ANIM_PLAYER_SHOOTSLIDE = 5;

	const int ANIM_BULLET = 0;
	const int ANIM_BULLET_HIT = 1;

	std::vector<Animation> playerAnims;
	std::vector<Animation> bulletAnims;

	std::vector<SDL_Texture*> textures;
	SDL_Texture* idleTex, * runTex, * slideTex, * shootTex, * shootRunTex, *shootSlideTex,
		* bulletTex, * bulletHitTex,
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

		bulletAnims.resize(2);
		bulletAnims[ANIM_BULLET] = Animation(4, 0.5f);
		bulletAnims[ANIM_BULLET_HIT] = Animation(4, 0.2f);
		bulletTex = loadTexture(state.renderer, bulletTexFilePath);
		bulletHitTex = loadTexture(state.renderer, bulletHitTexFilePath);

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