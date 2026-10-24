#pragma once

#include "Timer.h"

struct Animation
{
	Timer timer;
	int frameCount;

	Animation() : timer(0), frameCount(0) {}
	Animation(int frameCount, float length) : timer(length), frameCount(frameCount) {}

	const float getLength() { return timer.getLength(); }
	const int currentFrame()
	{
		return static_cast<int>(timer.getTime() / timer.getLength() * frameCount);
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