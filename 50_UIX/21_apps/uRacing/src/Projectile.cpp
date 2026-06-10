#include "Projectile.h"
#include <SDL2/SDL.h>

namespace Mario {

Projectile::Projectile(float x, float y, bool goRight)
    : Entity(EntityType::Fireball)
{
    bounds_   = {x, y, 12, 12};
    vel_.x    = goRight ? 500.0f : -500.0f;
    vel_.y    = 0;
}

void Projectile::update(float dt)
{
    lifeTime_ -= dt;
    if (lifeTime_ <= 0) { kill(); return; }

    vel_.y += GRAVITY * dt * 0.4f;
    /* bounce */
    if (onGround_) {
        vel_.y  = -250.0f;
        onGround_ = false;
    }
}

void Projectile::render(SDL_Renderer* r, const Vec2& cam)
{
    SDL_Rect dst = {
        (int)(bounds_.x - cam.x),
        (int)(bounds_.y - cam.y),
        (int)bounds_.w, (int)bounds_.h
    };
    SDL_SetRenderDrawColor(r, 255, 140, 0, 255);
    SDL_RenderFillRect(r, &dst);
    SDL_SetRenderDrawColor(r, 255, 255, 100, 200);
    SDL_Rect glow = {dst.x-2, dst.y-2, dst.w+4, dst.h+4};
    SDL_RenderFillRect(r, &glow);
}

} /* namespace Mario */
