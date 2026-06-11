#include "Coin.h"
//#include <SDL2/SDL.h>
#include <SDL.h>
#include <cmath>

namespace Mario {

Coin::Coin(float x, float y, bool anim)
    : Entity(EntityType::Coin), animated_(anim)
{
    bounds_ = {x, y, 20, 20};
}

void Coin::update(float dt)
{
    if (animated_) {
        bobTime_ += dt * 3.0f;
        bobHeight_ = std::sin(bobTime_) * 4.0f;
    }
    frameTime_ += dt;
    if (frameTime_ >= 0.15f) {
        frameTime_ = 0;
        frame_ = (frame_ + 1) % 4;
    }
}

void Coin::render(SDL_Renderer* r, const Vec2& cam)
{
    SDL_Rect dst = {
        (int)(bounds_.x - cam.x),
        (int)(bounds_.y - cam.y + bobHeight_),
        (int)bounds_.w, (int)bounds_.h
    };
    /* Gold coin */
    SDL_SetRenderDrawColor(r, 255, 210, 0, 255);
    SDL_RenderFillRect(r, &dst);
    /* Shine */
    SDL_SetRenderDrawColor(r, 255, 240, 120, 255);
    SDL_Rect shine = {dst.x+3, dst.y+3, 6, 6};
    SDL_RenderFillRect(r, &shine);
    /* Inner circle visual */
    SDL_SetRenderDrawColor(r, 200, 160, 0, 255);
    SDL_Rect inner = {dst.x+6, dst.y+6, 8, 8};
    SDL_RenderFillRect(r, &inner);
}

} /* namespace Mario */
