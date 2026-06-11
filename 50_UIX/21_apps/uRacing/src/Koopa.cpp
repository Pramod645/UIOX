#include "Koopa.h"
#include "AudioManager.h"
//#include <SDL2/SDL.h>
#include <SDL.h>
#include <cmath>

namespace Mario {

Koopa::Koopa(float x, float y) : Enemy(EntityType::Koopa)
{
    bounds_   = {x, y, 28, 40};
    vel_.x    = -moveSpeed_;
}

void Koopa::update(float dt)
{
    if (stomped_) {
        if (kState_ == KoopaState::Shell) {
            //The Koopa wakes up after 5 seconds only if it is stationary (vel_.x == 0). A sliding shell never wakes — the timer only matters for a kicked-then-stopped shell that has come to rest against a wall.
            shellTimer_ -= dt;
            if (shellTimer_ <= 0 && vel_.x == 0) {
                /* wake up */
                kState_ = KoopaState::Walk;
                stomped_= false;
                vel_.x  = -moveSpeed_;
            }
        }
        return;
    }

    vel_.y += GRAVITY * dt;
    frameTime_ += dt;
    if (frameTime_ >= 0.12f) {
        frameTime_ = 0;
        frame_     = (frame_ + 1) % 4;
    }
}

void Koopa::render(SDL_Renderer* r, const Vec2& cam)
{
    SDL_Rect dst = {
        (int)(bounds_.x - cam.x),
        (int)(bounds_.y - cam.y),
        (int)bounds_.w, (int)bounds_.h
    };

    bool isShell = (kState_ == KoopaState::Shell ||
                    kState_ == KoopaState::SlideShell);

    if (isShell) {
        /* Green shell */
        SDL_SetRenderDrawColor(r, 40, 160, 40, 255);
        SDL_Rect shell = {dst.x, dst.y + dst.h/2,
                           dst.w, dst.h/2};
        SDL_RenderFillRect(r, &shell);
        /* Shell pattern */
        SDL_SetRenderDrawColor(r, 200, 220, 60, 255);
        SDL_Rect cross = {dst.x + dst.w/2-2, shell.y+2, 4, shell.h-4};
        SDL_RenderFillRect(r, &cross);
    } else {
        /* Shell (back) */
        SDL_SetRenderDrawColor(r, 40, 160, 40, 255);
        SDL_RenderFillRect(r, &dst);
        /* Head */
        SDL_SetRenderDrawColor(r, 240, 200, 60, 255);
        SDL_Rect head = {dst.x + 4, dst.y, dst.w-8, dst.h/3};
        SDL_RenderFillRect(r, &head);
        /* Eye */
        SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
        int ex = facingRight_ ? dst.x+dst.w-10 : dst.x+4;
        SDL_Rect eye = {ex, head.y+4, 5, 5};
        SDL_RenderFillRect(r, &eye);
    }
}
//First stomp: enters shell mode, stops moving, starts a 5-second timer. After 5 seconds the Koopa wakes back up.
void Koopa::stomp()
{
    if (kState_ == KoopaState::Walk) {
        kState_     = KoopaState::Shell;
        stomped_    = true;
        shellTimer_ = 5.0f;
        vel_.x      = 0;
        AudioManager::instance().playSound("stomp");
    } else if (kState_ == KoopaState::Shell) {//Stomping an already-shelled Koopa kicks it instead. This is the two-phase interaction: stomp→shell, then touch/stomp shell→kick.
        kick();
    }
}
//Shell launches in the direction the player faces at 400 px/s. In SlideShell state the physics resolver (in Level) treats the shell as a projectile that kills enemies on contact.
/*
The shell launches at 400 px/s — fast enough to cross the screen in about 3 seconds and feel impactful, slow enough to be avoidable. Direction is based on facingRight_ which the collision system updates when the shell hits a wall and reverses.
*/
void Koopa::kick()
{
    kState_ = KoopaState::SlideShell;
    vel_.x  = (facingRight_ ? 1 : -1) * 400.0f;
    AudioManager::instance().playSound("kick");
}

void Koopa::onCollideWith(Entity* other) {}

} /* namespace Mario */
