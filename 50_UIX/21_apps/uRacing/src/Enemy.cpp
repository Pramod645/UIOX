/*
 * Enemy.cpp — Base Enemy class implementation.
 *
 * Provides the shared physics loop, wall-reversal patrol logic,
 * ledge detection, hurt-flash animation, and death sequence
 * that all enemies (Goomba, Koopa…) inherit.
 */
#include "Enemy.h"
#include "Physics.h"
#include "AudioManager.h"
//#include <SDL2/SDL.h>
#include <SDL.h>
#include <cmath>
#include <algorithm>

namespace Mario {

/* ── Constructor ────────────────────────────────────────────── */
Enemy::Enemy(EntityType t) : Entity(t)
{
    facingRight_ = false;   /* enemies start walking left by default */
}

/* ============================================================
   Enemy::updatePhysics
   Shared physics tick called by every subclass.
   Handles: gravity, tile collision, patrol reversal,
            ledge detection, and death-fall animation.
   ============================================================ */
void Enemy::updatePhysics(
        float dt,
        const std::vector<std::vector<Tile>> &tiles,
        int cols, int rows)
{
    if (stomped_) {
        /* ── stomp death animation ──────────────────────────── */
        stompTimer -= dt;
        if (stompTimer <= 0.0f) kill();
        return;  /* no movement during stomp */
    }

    if (!alive_) return;

    /* hurt flash */
    if (hurtFlash_ > 0.0f) {
        hurtFlash_ -= dt;
        if (hurtFlash_ < 0.0f) hurtFlash_ = 0.0f;
    }

    /* ── gravity + tile resolution ──────────────────────────── */
    bool wasOnGround = onGround_;

    onGround_ = Physics::resolveEntity(
                    *this, bounds_, vel_,
                    tiles, cols, rows,
                    dt, true /* reverseOnWall */);

    /* update facing direction from velocity */
    if (vel_.x >  0.01f) facingRight_ = true;
    if (vel_.x < -0.01f) facingRight_ = false;

    /* ── ledge detection (optional per enemy) ───────────────── */
    if (onGround_ && detectLedges_) {
        /* probe one tile ahead and one tile below */
        float probeX = facingRight_
                       ? bounds_.right() + 2.0f
                       : bounds_.x       - 2.0f;
        float probeY  = bounds_.bottom() + 2.0f;

        int pc = (int)(probeX / TILE_SIZE);
        int pr = (int)(probeY / TILE_SIZE);

        bool floorAhead = false;
        if (pc >= 0 && pc < cols && pr >= 0 && pr < rows)
            floorAhead = tiles[pr][pc].solid;

        if (!floorAhead) {
            /* turn around at ledge edge */
            vel_.x = -vel_.x;
            facingRight_ = !facingRight_;
        }
    }

    /* ── maintain patrol speed ──────────────────────────────── */
    if (onGround_ && patrolling_ && !stomped_) {
        float target = facingRight_ ?  moveSpeed_ : -moveSpeed_;
        /* snap to patrol speed (simple, no acceleration) */
        vel_.x = target;
    }

    /* ── clamp fall speed ───────────────────────────────────── */
    vel_.y = std::min(vel_.y, 800.0f);

    /* landing event */
    if (!wasOnGround && onGround_)
        onLand();
}

/* ============================================================
   Enemy::onLand
   Called the frame an enemy lands after being airborne.
   Subclasses override for specific landing behaviour.
   ============================================================ */
void Enemy::onLand() { /* default: do nothing */ }

/* ============================================================
   Enemy::takeDamage
   Generic damage handler: triggers hurt flash.
   Subclasses override to add specific death logic.
   ============================================================ */
void Enemy::takeDamage()
{
    if (stomped_) return;
    hurtFlash_ = 0.3f;
    kill();
}

/* ============================================================
   Enemy::drawDebugRect
   Draws the AABB as a coloured outline for debugging.
   ============================================================ */
void Enemy::drawDebugRect(SDL_Renderer *r, const Vec2 &cam) const
{
#ifdef DEBUG
    SDL_SetRenderDrawColor(r, 255, 0, 0, 200);
    SDL_Rect dbg = {
        (int)(bounds_.x - cam.x),
        (int)(bounds_.y - cam.y),
        (int)bounds_.w,
        (int)bounds_.h
    };
    SDL_RenderDrawRect(r, &dbg);
#else
    (void)r; (void)cam;
#endif
}

/* ============================================================
   Enemy::drawShadow
   Draws a soft oval shadow under the enemy (cosmetic).
   ============================================================ */
void Enemy::drawShadow(SDL_Renderer *r, const Vec2 &cam) const
{
    if (!onGround_) return;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 50);

    int sw = (int)(bounds_.w * 0.8f);
    int sh = 6;
    SDL_Rect shadow = {
        (int)(bounds_.x - cam.x + bounds_.w * 0.1f),
        (int)(bounds_.bottom() - cam.y - 2),
        sw, sh
    };
    SDL_RenderFillRect(r, &shadow);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

/* ============================================================
   Enemy::isVisible
   Cheap frustum cull — returns false if the enemy is
   outside the visible screen area (camera view).
   ============================================================ */
bool Enemy::isVisible(const Vec2 &cam) const
{
    float sx = bounds_.x - cam.x;
    float sy = bounds_.y - cam.y;
    return sx > -TILE_SIZE * 2 &&
           sx < SCREEN_W + TILE_SIZE * 2 &&
           sy > -TILE_SIZE * 2 &&
           sy < SCREEN_H + TILE_SIZE * 2;
}

/* ============================================================
   Enemy::drawHurtFlash
   When hurtFlash_ > 0 draws a white overlay flash.
   ============================================================ */
void Enemy::drawHurtFlash(SDL_Renderer *r, const Vec2 &cam) const
{
    if (hurtFlash_ <= 0.0f) return;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    Uint8 alpha = (Uint8)(200 * (hurtFlash_ / 0.3f));
    SDL_SetRenderDrawColor(r, 255, 255, 255, alpha);
    SDL_Rect dst = {
        (int)(bounds_.x - cam.x),
        (int)(bounds_.y - cam.y),
        (int)bounds_.w,
        (int)bounds_.h
    };
    SDL_RenderFillRect(r, &dst);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

/* ============================================================
   Enemy::advanceFrame
   Steps the sprite animation frame counter.
   ============================================================ */
void Enemy::advanceFrame(float dt)
{
    frameTime_ += dt;
    if (frameTime_ >= frameDur_) {
        frameTime_ = 0.0f;
        frame_     = (frame_ + 1) % std::max(1, numFrames_);
    }
}

/* ============================================================
   Enemy::setPatrol
   Configure patrol speed and direction at spawn time.
   ============================================================ */
void Enemy::setPatrol(float speed, bool startRight,
                       bool detectLedges)
{
    moveSpeed_    = speed;
    facingRight_  = startRight;
    patrolling_   = true;
    detectLedges_ = detectLedges;
    vel_.x        = startRight ? speed : -speed;
}

/* ============================================================
   Enemy::setMoveSpeed
   Allows Level or subclass to change patrol speed at runtime
   (e.g. Koopa shell slide).
   ============================================================ */
void Enemy::setMoveSpeed(float speed)
{
    moveSpeed_ = std::abs(speed);
    if (patrolling_)
        vel_.x = facingRight_ ? moveSpeed_ : -moveSpeed_;
}

/* ============================================================
   Enemy::boundsForStomp
   Returns a slightly shrunken rect used only for stomp
   detection, so the player needs to land closer to centre.
   ============================================================ */
Rect Enemy::boundsForStomp() const
{
    Rect r = bounds_;
    r.x += 4.0f;
    r.w -= 8.0f;
    r.y += bounds_.h * 0.25f;  /* top 25% is the stomp zone */
    r.h  = bounds_.h * 0.25f;
    return r;
}

} /* namespace Mario */
