#pragma once

#include "GameObject.h"

struct Resources;

// no need to be polymorphic since only four types of GOs
class GameObjectFactory
{
public:
    explicit GameObjectFactory(const Resources& res) : res(res) {}

    GameObject makeTile(glm::vec2 position, SDL_Texture* texture, bool solid) const;
    GameObject makePlayer(glm::vec2 position) const;
    GameObject makeEnemy(glm::vec2 position) const;
    GameObject makeBullet(glm::vec2 position, float direction, glm::vec2 velocity) const;

    void resetBullet(GameObject& b, glm::vec2 position, float direction, glm::vec2 velocity) const;

private:
    const Resources& res;
};