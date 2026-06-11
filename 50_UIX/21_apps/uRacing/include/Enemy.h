#pragma once
/*
 * Enemy.h — Base class for all MarioGame enemies.
 *
 * Provides:
 *   - Shared patrol physics (wall-reverse, ledge detection)
 *   - Stomp / hurt-flash animation
 *   - AABB debug draw and shadow rendering
 *   - Frustum culling helper
 *   - Pure virtual stomp() that each enemy must implement
 */
#include "Entity.h"
#include "Tile.h"
#include <vector>

namespace Mario {

class Enemy : public Entity {
public:
    explicit Enemy(EntityType t);

    /* ── Pure virtual interface ───────────────────────────── */
    virtual void stomp() = 0;    /* called when player lands on top */

    /* ── Shared physics tick ──────────────────────────────── */
    void updatePhysics(float dt,
                        const std::vector<std::vector<Tile>> &tiles,
                        int cols, int rows);

    /* ── State queries ────────────────────────────────────── */
    bool isStomped()  const { return stomped_; }
    bool isHurt()     const { return hurtFlash_ > 0.0f; }
    bool visible(const Vec2 &cam) const { return isVisible(cam); }

    /* ── Configuration (call at spawn) ───────────────────── */
    void setPatrol(float speed, bool startRight,
                    bool detectLedges = true);
    void setMoveSpeed(float speed);

    /* ── Physics accessors for Level resolver ─────────────── */
    void setOnGround(bool v)    { onGround_  = v; }
    Rect& boundsRef()           { return bounds_; }
    Vec2& velRef()              { return vel_;    }

    /* ── Stomp hit zone (shrunken AABB) ───────────────────── */
    Rect boundsForStomp() const;

protected:
    /* stomp state */
    bool  stomped_    = false;
    float stompTimer  = 0.0f;   /* seconds until corpse removed   */

    /* patrol config */
    float moveSpeed_    = 80.0f;
    bool  patrolling_   = true;
    bool  detectLedges_ = true;  /* turn at ledge edges            */

    /* visual */
    float hurtFlash_  = 0.0f;   /* white-flash timer on hit       */

    /* helpers called by subclasses in render() */
    void drawShadow    (SDL_Renderer *r, const Vec2 &cam) const;
    void drawDebugRect (SDL_Renderer *r, const Vec2 &cam) const;
    void drawHurtFlash (SDL_Renderer *r, const Vec2 &cam) const;
    void advanceFrame  (float dt);
    bool isVisible     (const Vec2 &cam) const;

    /* event hooks */
    virtual void onLand();
    virtual void takeDamage();
};

} /* namespace Mario */
