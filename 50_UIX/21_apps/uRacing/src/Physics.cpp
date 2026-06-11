#include "Physics.h"
#include "Level.h"
#include "Player.h"
#include "Enemy.h"
#include "Projectile.h"
#include "Coin.h"
#include "PowerUp.h"
#include <algorithm>
#include <cmath>

namespace Mario {

/* ── Constants ──────────────────────────────────────────────── */
static constexpr float EPSILON        = 0.001f;
static constexpr float MAX_FALL_SPEED = 800.0f;
static constexpr float BOUNCE_DAMP    = 0.3f;   /* projectile bounce  */

/* ── Internal helper: resolve one AABB against the tile grid ── */
/*
 * Moves 'bounds' by (vx*dt, vy*dt), resolves collisions against
 * all solid tiles in the level, writes back corrected bounds +
 * velocity, and sets 'onGround' if the entity landed on top of
 * a solid tile.
 *
 * Returns a bitmask of which sides had a collision:
 *   PHYS_HIT_LEFT   (1 << 0)
 *   PHYS_HIT_RIGHT  (1 << 1)
 *   PHYS_HIT_TOP    (1 << 2)   (hit ceiling)
 *   PHYS_HIT_BOTTOM (1 << 3)  (landed)
 */
static constexpr int PHYS_HIT_LEFT   = (1 << 0);
static constexpr int PHYS_HIT_RIGHT  = (1 << 1);
static constexpr int PHYS_HIT_TOP    = (1 << 2);
static constexpr int PHYS_HIT_BOTTOM = (1 << 3);

static int resolveTiles(Rect &bounds, Vec2 &vel,
                         bool  &onGround,
                         const std::vector<std::vector<Tile>> &tiles,
                         int cols, int rows,
                         float dt)
{
    int hits = 0;
    onGround = false;

    /* ── X axis ──────────────────────────────────────────────── */
    bounds.x += vel.x * dt;

    int col0 = (int)( bounds.x            / TILE_SIZE);
    int col1 = (int)((bounds.right() - 1) / TILE_SIZE);
    int row0 = (int)( bounds.y            / TILE_SIZE);
    int row1 = (int)((bounds.bottom()- 1) / TILE_SIZE);

    col0 = std::max(0, col0);  col1 = std::min(cols - 1, col1);
    row0 = std::max(0, row0);  row1 = std::min(rows - 1, row1);

    for (int r = row0; r <= row1 && !(hits & (PHYS_HIT_LEFT | PHYS_HIT_RIGHT)); ++r) {
        for (int c = col0; c <= col1; ++c) {
            if (!tiles[r][c].solid) continue;
            Rect tr = tiles[r][c].worldRect(c, r);
            if (!bounds.overlaps(tr)) continue;

            if (vel.x > EPSILON) {
                bounds.x = tr.x - bounds.w;
                vel.x    = 0.0f;
                hits    |= PHYS_HIT_RIGHT;
            } else if (vel.x < -EPSILON) {
                bounds.x = tr.right();
                vel.x    = 0.0f;
                hits    |= PHYS_HIT_LEFT;
            }
            break;
        }
    }

    /* ── Y axis ──────────────────────────────────────────────── */
    vel.y    += GRAVITY * dt;
    vel.y     = std::min(vel.y, MAX_FALL_SPEED);
    bounds.y += vel.y * dt;

    col0 = (int)( bounds.x            / TILE_SIZE);
    col1 = (int)((bounds.right() - 1) / TILE_SIZE);
    row0 = (int)( bounds.y            / TILE_SIZE);
    row1 = (int)((bounds.bottom()- 1) / TILE_SIZE);

    col0 = std::max(0, col0);  col1 = std::min(cols - 1, col1);
    row0 = std::max(0, row0);  row1 = std::min(rows - 1, row1);

    for (int r = row0; r <= row1; ++r) {
        for (int c = col0; c <= col1; ++c) {
            if (!tiles[r][c].solid) continue;
            Rect tr = tiles[r][c].worldRect(c, r);
            if (!bounds.overlaps(tr)) continue;

            if (vel.y > EPSILON) {
                /* falling: landed on top */
                bounds.y  = tr.y - bounds.h;
                vel.y     = 0.0f;
                onGround  = true;
                hits     |= PHYS_HIT_BOTTOM;
            } else if (vel.y < -EPSILON) {
                /* rising: hit ceiling */
                bounds.y = tr.bottom();
                vel.y    = 0.0f;
                hits    |= PHYS_HIT_TOP;
            }
            break;
        }
    }

    return hits;
}

/* ============================================================
   Physics::resolvePlayer
   Full player-vs-tile resolution with block interaction.
   ============================================================ */
void Physics::resolvePlayer(Player               &player,
                             std::vector<std::vector<Tile>> &tiles,
                             int cols, int rows,
                             float dt,
                             ParticleSystem       &particles,
                             std::vector<std::unique_ptr<Entity>> &entities,
                             bool &levelComplete)
{
    Rect &b  = player.boundsRef();
    Vec2 &v  = player.velRef();
    bool  og = false;

    /* ── X axis ──────────────────────────────────────────────── */
    b.x += v.x * dt;

    int col0 = std::max(0, (int)( b.x            / TILE_SIZE));
    int col1 = std::min(cols-1,(int)((b.right()-1) / TILE_SIZE));
    int row0 = std::max(0, (int)( b.y            / TILE_SIZE));
    int row1 = std::min(rows-1,(int)((b.bottom()-1)/ TILE_SIZE));

    for (int r = row0; r <= row1; ++r)
        for (int c = col0; c <= col1; ++c) {
            if (!tiles[r][c].solid) continue;
            Rect tr = tiles[r][c].worldRect(c, r);
            if (!b.overlaps(tr)) continue;
            if (v.x > 0) { b.x = tr.x - b.w; v.x = 0; }
            else          { b.x = tr.right(); v.x = 0; }
        }

    /* ── Y axis ──────────────────────────────────────────────── */
    v.y  += GRAVITY * dt;
    v.y   = std::min(v.y, MAX_FALL_SPEED);
    b.y  += v.y * dt;

    col0 = std::max(0, (int)( b.x            / TILE_SIZE));
    col1 = std::min(cols-1,(int)((b.right()-1) / TILE_SIZE));
    row0 = std::max(0, (int)( b.y            / TILE_SIZE));
    row1 = std::min(rows-1,(int)((b.bottom()-1)/ TILE_SIZE));

    for (int r = row0; r <= row1; ++r)
        for (int c = col0; c <= col1; ++c) {
            Tile *t = &tiles[r][c];
            if (!t->solid) continue;
            Rect tr = t->worldRect(c, r);
            if (!b.overlaps(tr)) continue;

            if (v.y > EPSILON) {
                /* ── landed on top ─────────────────────────── */
                b.y  = tr.y - b.h;
                v.y  = 0.0f;
                og   = true;

                /* flag pole detection */
                if (t->type == TileType::Flag) {
                    player.addScore(5000);
                    levelComplete = true;
                    AudioManager::instance().playSound("flagpole");
                }
                /* lava / deadly */
                if (t->deadly) player.takeDamage();

            } else if (v.y < -EPSILON) {
                /* ── hit ceiling ───────────────────────────── */
                b.y = tr.bottom();
                v.y = 0.0f;

                Vec2 blockCentre {
                    (float)(c * TILE_SIZE + TILE_SIZE / 2),
                    (float)(r * TILE_SIZE + TILE_SIZE / 2)
                };

                if (t->type == TileType::QuestionBlock) {
                    /* question block hit */
                    t->type  = TileType::HitBlock;
                    t->solid = true;
                    t->hitCount++;

                    float px = c * (float)TILE_SIZE;
                    float py = (r - 1) * (float)TILE_SIZE;

                    if (player.powerLevel() == PowerLevel::Small) {
                        /* coin pops out */
                        entities.push_back(
                            std::make_unique<Coin>(px, py));
                        player.collectCoin();
                    } else {
                        PowerUpType put = (t->hitCount % 2 == 0)
                                          ? PowerUpType::FireFlower
                                          : PowerUpType::Mushroom;
                        entities.push_back(
                            std::make_unique<PowerUp>(px, py, put));
                    }
                    AudioManager::instance().playSound("block");

                } else if (t->type == TileType::Brick &&
                           player.powerLevel() != PowerLevel::Small) {
                    /* brick break */
                    t->type  = TileType::Empty;
                    t->solid = false;
                    player.addScore(50);
                    particles.spawnBrickBreak(blockCentre);
                    AudioManager::instance().playSound("break");
                }
            }
        }

    /* ── left screen boundary ───────────────────────────────── */
    if (b.x < 0.0f) { b.x = 0.0f; v.x = 0.0f; }

    /* ── fell below level ───────────────────────────────────── */
    if (b.y > rows * (float)TILE_SIZE + 64.0f)
        player.takeDamage();

    player.setOnGround(og);
}

/* ============================================================
   Physics::resolveEntity
   Generic entity (enemy / power-up / projectile) vs tiles.
   Reverses X velocity on horizontal wall hit (patrol bounce).
   Returns true if entity landed on ground this frame.
   ============================================================ */
bool Physics::resolveEntity(Entity                              &e,
                              Rect                               &bounds,
                              Vec2                               &vel,
                              const std::vector<std::vector<Tile>> &tiles,
                              int cols, int rows,
                              float dt,
                              bool reverseOnWall)
{
    bool onGround = false;

    /* ── X axis ──────────────────────────────────────────────── */
    bounds.x += vel.x * dt;

    int col0 = std::max(0,      (int)( bounds.x            / TILE_SIZE));
    int col1 = std::min(cols-1, (int)((bounds.right() - 1) / TILE_SIZE));
    int row0 = std::max(0,      (int)( bounds.y            / TILE_SIZE));
    int row1 = std::min(rows-1, (int)((bounds.bottom()- 1) / TILE_SIZE));

    for (int r = row0; r <= row1; ++r)
        for (int c = col0; c <= col1; ++c) {
            if (!tiles[r][c].solid) continue;
            Rect tr = tiles[r][c].worldRect(c, r);
            if (!bounds.overlaps(tr)) continue;
            if (vel.x > EPSILON) {
                bounds.x = tr.x - bounds.w;
                if (reverseOnWall) vel.x = -std::abs(vel.x);
                else               vel.x =  0.0f;
            } else if (vel.x < -EPSILON) {
                bounds.x = tr.right();
                if (reverseOnWall) vel.x =  std::abs(vel.x);
                else               vel.x =  0.0f;
            }
        }

    /* ── Y axis ──────────────────────────────────────────────── */
    vel.y   += GRAVITY * dt;
    vel.y    = std::min(vel.y, MAX_FALL_SPEED);
    bounds.y += vel.y * dt;

    col0 = std::max(0,      (int)( bounds.x            / TILE_SIZE));
    col1 = std::min(cols-1, (int)((bounds.right() - 1) / TILE_SIZE));
    row0 = std::max(0,      (int)( bounds.y            / TILE_SIZE));
    row1 = std::min(rows-1, (int)((bounds.bottom()- 1) / TILE_SIZE));

    for (int r = row0; r <= row1; ++r)
        for (int c = col0; c <= col1; ++c) {
            if (!tiles[r][c].solid) continue;
            Rect tr = tiles[r][c].worldRect(c, r);
            if (!bounds.overlaps(tr)) continue;
            if (vel.y > EPSILON) {
                bounds.y  = tr.y - bounds.h;
                vel.y     = 0.0f;
                onGround  = true;
            } else if (vel.y < -EPSILON) {
                bounds.y = tr.bottom();
                vel.y    = 0.0f;
            }
            break;
        }

    /* kill entity if it falls off the bottom */
    if (bounds.y > rows * (float)TILE_SIZE + 128.0f)
        e.kill();

    return onGround;
}

/* ============================================================
   Physics::resolveProjectile
   Fireballs bounce on the ground.
   ============================================================ */
bool Physics::resolveProjectile(Entity                              &proj,
                                  Rect                               &bounds,
                                  Vec2                               &vel,
                                  const std::vector<std::vector<Tile>> &tiles,
                                  int cols, int rows,
                                  float dt)
{
    /* X axis — kill on wall */
    bounds.x += vel.x * dt;

    int col0 = std::max(0,      (int)( bounds.x            / TILE_SIZE));
    int col1 = std::min(cols-1, (int)((bounds.right() - 1) / TILE_SIZE));
    int row0 = std::max(0,      (int)( bounds.y            / TILE_SIZE));
    int row1 = std::min(rows-1, (int)((bounds.bottom()- 1) / TILE_SIZE));

    for (int r = row0; r <= row1; ++r)
        for (int c = col0; c <= col1; ++c) {
            if (!tiles[r][c].solid) continue;
            Rect tr = tiles[r][c].worldRect(c, r);
            if (!bounds.overlaps(tr)) continue;
            proj.kill();   /* fireball dies on wall */
            return false;
        }

    /* Y axis — bounce upward on ground */
    vel.y   += GRAVITY * dt * 0.4f;
    vel.y    = std::min(vel.y, MAX_FALL_SPEED);
    bounds.y += vel.y * dt;

    col0 = std::max(0,      (int)( bounds.x            / TILE_SIZE));
    col1 = std::min(cols-1, (int)((bounds.right() - 1) / TILE_SIZE));
    row0 = std::max(0,      (int)( bounds.y            / TILE_SIZE));
    row1 = std::min(rows-1, (int)((bounds.bottom()- 1) / TILE_SIZE));

    bool bounced = false;
    for (int r = row0; r <= row1; ++r)
        for (int c = col0; c <= col1; ++c) {
            if (!tiles[r][c].solid) continue;
            Rect tr = tiles[r][c].worldRect(c, r);
            if (!bounds.overlaps(tr)) continue;
            if (vel.y > EPSILON) {
                bounds.y = tr.y - bounds.h;
                vel.y    = -280.0f;  /* bounce upward */
                bounced  = true;
            } else if (vel.y < -EPSILON) {
                bounds.y = tr.bottom();
                vel.y    = 0.0f;
                proj.kill();
            }
            break;
        }

    if (bounds.y > rows * (float)TILE_SIZE + 64.0f)
        proj.kill();

    return bounced;
}

/* ============================================================
   Physics::checkEntityPlayer
   Tests one entity rect vs player rect.
   Returns a PhysContact describing the relationship.
   ============================================================ */
PhysContact Physics::checkEntityPlayer(const Rect &entityBounds,
                                        const Vec2 &entityVel,
                                        const Rect &playerBounds,
                                        const Vec2 &playerVel)
{
    PhysContact contact;
    contact.hit = entityBounds.overlaps(playerBounds);

    if (!contact.hit) return contact;

    /* determine if player is stomping from above:
       player moving downward AND player's bottom was above
       entity's top in the previous frame                    */
    float prevPlayerBottom = playerBounds.bottom() -
                             playerVel.y * FIXED_DT;

    contact.stomped = (playerVel.y  >  EPSILON) &&
                      (prevPlayerBottom <= entityBounds.y + 6.0f);

    /* side contact = player moving horizontally into enemy */
    contact.side = !contact.stomped;

    return contact;
}

/* ============================================================
   Physics::checkEntityEntity
   Simple AABB overlap between two arbitrary entities.
   ============================================================ */
bool Physics::checkEntityEntity(const Rect &a, const Rect &b)
{
    return a.overlaps(b);
}

/* ============================================================
   Physics::applyGravityOnly
   Advance velocity by gravity for one timestep (no tile check).
   Used for entities in their die/pop animation phase.
   ============================================================ */
void Physics::applyGravityOnly(Vec2 &vel, float dt, float scale)
{
    vel.y += GRAVITY * dt * scale;
    vel.y  = std::min(vel.y, MAX_FALL_SPEED);
}

/* ============================================================
   Physics::integratePosition
   Euler-advance a position by its velocity for dt seconds.
   ============================================================ */
void Physics::integratePosition(Rect &bounds, const Vec2 &vel, float dt)
{
    bounds.x += vel.x * dt;
    bounds.y += vel.y * dt;
}

/* ============================================================
   Physics::isOnGround
   Quick check: is the bottom of 'bounds' sitting on a solid tile?
   Checks one pixel below the bottom edge.
   ============================================================ */
bool Physics::isOnGround(const Rect                              &bounds,
                           const std::vector<std::vector<Tile>> &tiles,
                           int cols, int rows)
{
    float probeY = bounds.bottom() + 1.0f;
    int   col0   = std::max(0,      (int)( bounds.x            / TILE_SIZE));
    int   col1   = std::min(cols-1, (int)((bounds.right() - 1) / TILE_SIZE));
    int   row    = std::min(rows-1, (int)( probeY              / TILE_SIZE));

    if (row < 0) return false;
    for (int c = col0; c <= col1; ++c)
        if (tiles[row][c].solid) return true;
    return false;
}

/* ============================================================
   Physics::clampToLevelBounds
   Prevents an entity from scrolling left of x=0.
   ============================================================ */
void Physics::clampToLevelBounds(Rect &bounds, Vec2 &vel, int cols)
{
    if (bounds.x < 0.0f) {
        bounds.x = 0.0f;
        vel.x    = 0.0f;
    }
    float maxX = (float)(cols * TILE_SIZE) - bounds.w;
    if (bounds.x > maxX) {
        bounds.x = maxX;
        vel.x    = 0.0f;
    }
}

} /* namespace Mario */
