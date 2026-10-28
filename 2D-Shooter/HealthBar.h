// HealthBar.h
#pragma once

#include <algorithm>

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "GameObject.h"

struct HealthBar
{
    float width = 32.f;
    float height = 4.f;
    float offsetY = -8.f;
    SDL_Color background{ 30, 30, 30, 255 };

    void Draw(SDL_Renderer* renderer, float health, float maxHealth, glm::vec2 spriteTopLeft, float spriteWidth) const
    {
        if (maxHealth <= 0.f) return;

        const float ratio = std::clamp(health / maxHealth, 0.f, 1.f);

        const float x = spriteTopLeft.x + (spriteWidth - width) * 0.5f;
        const float y = spriteTopLeft.y + offsetY;

        const SDL_FRect bg{ x, y, width, height };
        const SDL_FRect fill{ x, y, width * ratio, height };

        SDL_SetRenderDrawColor(renderer, background.r, background.g, background.b, background.a);
        SDL_RenderFillRect(renderer, &bg);

        if (ratio > 0.f)
        {
            SDL_SetRenderDrawColor(renderer, 220, 60, 60, 255);
            SDL_RenderFillRect(renderer, &fill);
        }
    }
};