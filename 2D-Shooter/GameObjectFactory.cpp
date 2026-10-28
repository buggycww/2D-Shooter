#include "GameObjectFactory.h"
#include "Resources.h"
#include "tmx.h"
#include "Animation.h"
#include <cassert>

GameObject GameObjectFactory::makeTile(glm::vec2 position, SDL_Texture* texture, bool solid) const
{
    SpriteSheet* sheet = new SpriteSheet{ texture, res.map->tileWidth, res.map->tileHeight, 1 };

    GameObject o;
    o.type = ObjectType::level;
    o.sprite = sheet;
    o.position = position;

    const float w = solid ? static_cast<float>(res.map->tileWidth) : 0.f;
    const float h = solid ? static_cast<float>(res.map->tileHeight) : 0.f;
    o.collider = { .x = 0.f, .y = 0.f, .w = w, .h = h };
    return o;
}

GameObject GameObjectFactory::makePlayer(glm::vec2 position) const
{
    SpriteSheet* sheet = res.playerAnims[res.ANIM_PLAYER_IDLE].spriteSheet;

    GameObject o;
    o.type = ObjectType::player;
    o.data.player = PlayerData();
    o.animations = res.playerAnims;
    o.currentAnimation = res.ANIM_PLAYER_IDLE;
    o.sprite = sheet;
    o.position = position;
    o.acceleration = glm::vec2(300.f, 0.f);
    o.maxSpeed.x = 100.f;
    o.collider = { .x = 12.f, .y = 6.f, .w = 10.f, .h = 26.f };
    o.dynamic = true;
    o.gravityEnabled = true;
    return o;
}

GameObject GameObjectFactory::makeEnemy(glm::vec2 position) const
{
    SpriteSheet* sheet = res.enemyAnims[res.ANIM_ENEMY_IDLE].spriteSheet;

    GameObject o;
    o.type = ObjectType::enemy;
    o.data.enemy = EnemyData();
    o.animations = res.enemyAnims;
    o.currentAnimation = res.ANIM_ENEMY_IDLE;
    o.sprite = sheet;
    o.position = position;
    o.maxSpeed.x = 70.f;
    o.collider = { .x = 10.f, .y = 6.f, .w = 12.f, .h = 26.f };
    o.dynamic = true;
    o.gravityEnabled = true;
    return o;
}

GameObject GameObjectFactory::makeBullet(glm::vec2 position, float direction, glm::vec2 velocity) const
{
    SpriteSheet* sheet = res.bulletAnims[res.ANIM_BULLET].spriteSheet;

    GameObject o;
    o.type = ObjectType::bullet;
    o.data.bullet = BulletData();
    o.animations = res.bulletAnims;
    o.currentAnimation = res.ANIM_BULLET;
    o.sprite = sheet;
    o.position = position;
    o.direction = direction;
    o.velocity = velocity;
    o.maxSpeed = glm::vec2(1000.f, 0.f);
    o.collider = {
        .x = 0.f, .y = 0.f,
        .w = static_cast<float>(o.sprite->frameWidth),
        .h = static_cast<float>(o.sprite->frameHeight)
    };
    o.dynamic = true;
    return o;
}

void GameObjectFactory::resetBullet(GameObject& b, glm::vec2 position, float direction, glm::vec2 velocity) const
{
    assert(b.type == ObjectType::bullet);

    b.data.bullet.state = BulletState::moving;

    b.position = position;
    b.direction = direction;
    b.velocity = velocity;
    b.maxSpeed = glm::vec2(1000.f, 0.f);
    b.dynamic = true;
    b.grounded = false;
    b.currentAnimation = res.ANIM_BULLET;
    b.sprite = res.bulletAnims[b.currentAnimation].spriteSheet;
    b.spriteFrame = 1;
    b.animations[res.ANIM_BULLET].reset();
    b.animations[res.ANIM_BULLET_HIT].reset();
    b.collider = {
        .x = 0.f, .y = 0.f,
        .w = static_cast<float>(b.sprite->frameWidth),
        .h = static_cast<float>(b.sprite->frameHeight)
    };
    b.shouldFlash = false;
    b.flashTimer.reset();
}
