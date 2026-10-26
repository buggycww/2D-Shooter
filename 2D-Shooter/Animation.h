#pragma once

#include <algorithm>

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