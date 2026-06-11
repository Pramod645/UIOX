#pragma once
/*
 * Physics.h — Standalone physics resolution utilities for MarioGame.
 *
 * Separates collision math from Entity/Level so each class
 * stays focused on its own responsibility.  Level calls these
 * helpers instead of duplicating AABB logic in multiple places.
 */
#include "Utils.h"
#include "Tile.h"
#include "Entity.h"
#include "ParticleSystem.h"
#include "AudioManager.h"
#include <vector>
#include <memory>

namespace Mario {

/* Forward declarations */
class Player;

/* ── Contact result returned by checkEntityPlayer ─────────── */
struct PhysContact {
    bool hit     = false;  /* any overlap at all               */
    bool stomped = false;  /* player jumped on top of enemy    */
    bool side    = false;  /* lateral / front collision        */
};

/* ── Physics utility class (all static methods) ──────────── */
class Physics {
public:
    Physics() = delete;   /* pure utility — no instances */

    /* Player-specific resolution (handles block breaking, flag) */
    static void resolvePlayer(
        Player                               &player,
        std::vector<std::vector<Tile>>       &tiles,
        int cols, int rows,
        float dt,
        ParticleSystem                       &particles,
        std::vector<std::unique_ptr<Entity>> &entities,
        bool                                 &levelComplete);

    /* Generic entity vs tiles (enemies, power-ups) */
    static bool resolveEntity(
        Entity                                       &e,
        Rect                                         &bounds,
        Vec2                                         &vel,
        const std::vector<std::vector<Tile>>         &tiles,
        int cols, int rows,
        float dt,
        bool reverseOnWall = true);

    /* Projectile (fireball) — bounces on ground, dies on wall */
    static bool resolveProjectile(
        Entity                                       &proj,
        Rect                                         &bounds,
        Vec2                                         &vel,
        const std::vector<std::vector<Tile>>         &tiles,
        int cols, int rows,
        float dt);

    /* Stomp / side-hit detection */
    static PhysContact checkEntityPlayer(
        const Rect &entityBounds, const Vec2 &entityVel,
        const Rect &playerBounds, const Vec2 &playerVel);

    /* Generic overlap test */
    static bool checkEntityEntity(const Rect &a, const Rect &b);

    /* Gravity only (for death animations) */
    static void applyGravityOnly(Vec2 &vel, float dt,
                                  float scale = 1.0f);

    /* Euler position advance */
    static void integratePosition(Rect &bounds,
                                   const Vec2 &vel, float dt);

    /* One-pixel ground probe */
    static bool isOnGround(
        const Rect                                   &bounds,
        const std::vector<std::vector<Tile>>         &tiles,
        int cols, int rows);

    /* Clamp entity x to [0, levelWidth] */
    static void clampToLevelBounds(Rect &bounds, Vec2 &vel, int cols);
};

} /* namespace Mario */
