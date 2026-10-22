#pragma once

#include "Animation.h"
#include <vector>

struct GameObject
{
	std::vector<Animation> anims;
	int currentAnim;
	int posX, posY;
	int currentDir;
};