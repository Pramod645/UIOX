#include "PowerUp.h"
#include <SDL2/SDL.h>

namespace Mario {

PowerUp::PowerUp(float x, float y, PowerUpType t)
    : Entity(EntityType::PowerUp), puType_(t)
{
    bounds_ = {x, y, 28, 28};
    vel_.x  = 60.0f;
}

void PowerUp::update(float dt)
{
    if (!popped_) {
        popTimer_ += dt;
        bounds_.y -= 40.0f * dt;
        if (popTimer_ >= 0.5f) popped_ = true;
        return;
    }
    vel_.y += GRAVITY * dt;
}

void PowerUp::render(SDL_Renderer* r, const Vec2& cam)
{
    SDL_Rect dst = {
        (int)(bounds_.x - cam.x),
        (int)(bounds_.y - cam.y),
        (int)bounds_.w, (int)bounds_.h
    };

    switch (puType_) {
        case PowerUpType::Mushroom:
            SDL_SetRenderDrawColor(r, 220, 50, 50, 255);
            SDL_RenderFillRect(r, &dst);
            SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
            for (int i = 0; i < 3; i++) {
                SDL_Rect dot = {dst.x+4+i*8, dst.y+4, 5, 5};
                SDL_RenderFillRect(r, &dot);
            }
            break;
        case PowerUpType::FireFlower:
            SDL_SetRenderDrawColor(r, 255, 120, 0, 255);
            SDL_RenderFillRect(r, &dst);
            SDL_SetRenderDrawColor(r, 255, 255, 0, 255);
            { SDL_Rect petal = {dst.x+8, dst.y+2, 12, 10};
              SDL_RenderFillRect(r, &petal); }
            break;
        case PowerUpType::Star:
            SDL_SetRenderDrawColor(r, 255, 220, 0, 255);
            SDL_RenderFillRect(r, &dst);
            break;
        case PowerUpType::OneUp:
            SDL_SetRenderDrawColor(r, 40, 200, 40, 255);
            SDL_RenderFillRect(r, &dst);
            SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
            { SDL_Rect txt = {dst.x+4, dst.y+8, dst.w-8, 12};
              SDL_RenderFillRect(r, &txt); }
            break;
    }
}

} /* namespace Mario */
