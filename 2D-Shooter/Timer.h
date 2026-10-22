#pragma once

class Timer
{
private:
	float length, time;
	bool timeout = false;
public:
	Timer(float length) : time(0), length(length) {}

	void step(float deltaTime)
	{
		time += deltaTime;

		if (time >= length)
		{
			time -= length;
			timeout = true;
		}
	}

	const bool isTimeout() { return timeout; }
	const float getTime() { return time; }
	const float getLength() { return length; }
	
	void reset()
	{
		time = 0;
		timeout = false;
	}
};