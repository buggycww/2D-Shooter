#pragma once

#include <algorithm>
#include "SDL3/SDL.h"

#include "Timer.h"

struct SpriteSheet
{
	SDL_Texture* texture;
	int frameWidth = 0;
	int frameHeight = 0;
	int frameCount = 0;

	SDL_FRect frameRect(int frame) const
	{
		return { static_cast<float>(frame * frameWidth), 0.f, static_cast<float>(frameWidth), static_cast<float>(frameHeight) };
	}
};

struct Animation
{
	SpriteSheet* spriteSheet = nullptr;
	Timer timer;
	int frameCount;

	Animation() : timer(0), frameCount(0) {}
	Animation(int frameCount, float length) : timer(length), frameCount(frameCount) {}

	const float getLength() { return timer.getLength(); }
	const int currentFrame()
	{
		if (frameCount <= 0 || timer.getLength() <= 0) return 0;

		const int frame = static_cast<int>(timer.getTime() / timer.getLength() * frameCount);
		return std::min(frame, frameCount - 1); // timer can overshoot by more than one frame
	}

	void step(float deltaTime)
	{
		timer.step(deltaTime);
	}

	bool isDone() const { return timer.isTimeout(); }

	void reset()
	{
		timer.reset();
	}
};