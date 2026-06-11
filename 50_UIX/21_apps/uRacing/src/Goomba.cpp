#include "Goomba.h"
#include "AudioManager.h"
//#include <SDL2/SDL.h>
#include <SDL.h>

namespace Mario {

Goomba::Goomba(float x, float y) : Enemy(EntityType::Goomba)
{
    bounds_   = {x, y, 28, 28};
    vel_.x    = -moveSpeed_;
    numFrames_= 2;
}
//After being stomped the Goomba stays visible for stompTimer seconds (0.4s) showing its flat squished form, then disappears. return skips normal update logic while in the death animation.
/*
The Goomba has two states: alive (patrolling) and stomped (dying). The return early-exit after stomp handling means the gravity and movement code below never runs during the death animation — clean state separation without nested if-blocks.
*/
void Goomba::update(float dt)
{
    if (stomped_) {
        stompTimer -= dt;
        if (stompTimer <= 0) kill();
        return;
    }

    vel_.y += GRAVITY * dt; //Gravity is applied every frame. The Goomba's vertical velocity accumulates downward — if it walks off a platform it falls naturally without any special "fall off edge" logic.
    frameTime_ += dt;
    if (frameTime_ >= frameDur_) {
        frameTime_ = 0;
        frame_ = (frame_ + 1) % 2;
    }
}
//When stomped, draws the Goomba at one-third height and shifts it down to stay grounded — a simple "squish" visual effect without needing a separate sprite.
void Goomba::render(SDL_Renderer* r, const Vec2& cam)
{
    SDL_Rect dst = {
        (int)(bounds_.x - cam.x),
        (int)(bounds_.y - cam.y),
        (int)bounds_.w,
        stomped_ ? (int)(bounds_.h / 3) : (int)bounds_.h
    };
    if (stomped_) dst.y += (int)(bounds_.h * 2 / 3);

    /* Brown body */
    SDL_SetRenderDrawColor(r, 140, 80, 20, 255);
    SDL_RenderFillRect(r, &dst);

    if (!stomped_) {
        /* Eyes */
        SDL_Rect eye1 = {dst.x + 4,        dst.y + 6, 6, 6};
        SDL_Rect eye2 = {dst.x + dst.w-10, dst.y + 6, 6, 6};
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        SDL_RenderFillRect(r, &eye1);
        SDL_RenderFillRect(r, &eye2);
        /* Pupils */
        SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
        SDL_Rect p1 = {eye1.x+2, eye1.y+2, 3, 3};
        SDL_Rect p2 = {eye2.x+1, eye2.y+2, 3, 3};
        SDL_RenderFillRect(r, &p1);
        SDL_RenderFillRect(r, &p2);
        /* Feet */
        int footY = dst.y + dst.h - 6;
        SDL_SetRenderDrawColor(r, 80, 40, 0, 255);
        SDL_Rect fl = {dst.x,           footY, 12, 6};
        SDL_Rect fr = {dst.x+dst.w-12,  footY, 12, 6};
        SDL_RenderFillRect(r, &fl);
        SDL_RenderFillRect(r, &fr);
    }
}

void Goomba::stomp()
{
    stomped_   = true;
    stompTimer = 0.4f;
    vel_.x     = 0;
    vel_.y     = 0;
    AudioManager::instance().playSound("stomp");
}

void Goomba::onCollideWith(Entity* other) {}

} /* namespace Mario */
