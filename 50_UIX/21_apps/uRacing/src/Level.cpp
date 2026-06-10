#include "Level.h"
#include "AudioManager.h"
#include "AssetManager.h"
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>

namespace Mario {

Level::Level(int n)
    : levelNum_(n)
    , player_(std::make_unique<Player>())
    , timeLeft_(300.0f)
{
    loadLevel(n);
}

void Level::loadLevel(int n)
{
    switch (n) {
        case 1: buildLevel1(); break;
        case 2: buildLevel2(); break;
        default: buildLevel3(); break;
    }
}

void Level::buildLevel1()
{
    /* 80 cols × 15 rows */
    data_.cols = 80;
    data_.rows = 15;
    data_.tiles.assign(data_.rows,
        std::vector<Tile>(data_.cols, Tile(TileType::Empty)));

    /* Ground row */
    for (int c = 0; c < data_.cols; ++c) {
        data_.tiles[14][c] = Tile(TileType::Ground);
        if (c >= 20 && c <= 22) /* gap */
            data_.tiles[14][c] = Tile(TileType::Empty);
    }

    /* Platforms */
    auto platform = [&](int row, int c0, int c1, TileType t){
        for (int c = c0; c <= c1; ++c)
            data_.tiles[row][c] = Tile(t);
    };

    platform(10,  3,  5, TileType::Brick);
    platform(10,  7,  7, TileType::QuestionBlock);   /* coin */
    platform( 8, 10, 13, TileType::Brick);
    platform( 6, 12, 12, TileType::QuestionBlock);   /* mushroom */
    platform(10, 16, 18, TileType::Brick);
    platform(10, 17, 17, TileType::QuestionBlock);
    platform( 8, 24, 28, TileType::Ground);
    platform( 6, 26, 26, TileType::QuestionBlock);
    platform(11, 30, 30, TileType::QuestionBlock);
    platform( 7, 35, 38, TileType::Brick);
    platform( 5, 40, 45, TileType::Brick);
    platform( 5, 43, 43, TileType::QuestionBlock);

    /* Pipes */
    for (int row = 12; row <= 13; ++row) {
        data_.tiles[row][8]  = Tile(TileType::Pipe);
        data_.tiles[row][9]  = Tile(TileType::Pipe);
        data_.tiles[row][50] = Tile(TileType::Pipe);
        data_.tiles[row][51] = Tile(TileType::Pipe);
        data_.tiles[row][60] = Tile(TileType::Pipe);
        data_.tiles[row][61] = Tile(TileType::Pipe);
    }
    data_.tiles[11][8]  = Tile(TileType::PipeTop);
    data_.tiles[11][9]  = Tile(TileType::PipeTop);
    data_.tiles[11][50] = Tile(TileType::PipeTop);
    data_.tiles[11][51] = Tile(TileType::PipeTop);
    data_.tiles[11][60] = Tile(TileType::PipeTop);
    data_.tiles[11][61] = Tile(TileType::PipeTop);

    /* Flag */
    data_.tiles[4][75] = Tile(TileType::Flag);
    for (int r = 5; r <= 14; ++r)
        data_.tiles[r][75] = Tile(TileType::Ground);

    data_.playerStart = {3.0f * TILE_SIZE, 12.0f * TILE_SIZE};
    player_->boundsRef().x = data_.playerStart.x;
    player_->boundsRef().y = data_.playerStart.y;

    /* Enemies */
    placeGoomba(14 * TILE_SIZE, 13 * TILE_SIZE);
    placeGoomba(25 * TILE_SIZE, 13 * TILE_SIZE);
    placeGoomba(30 * TILE_SIZE, 13 * TILE_SIZE);
    placeKoopa (40 * TILE_SIZE, 13 * TILE_SIZE);
    placeGoomba(55 * TILE_SIZE, 13 * TILE_SIZE);
    placeGoomba(56 * TILE_SIZE, 13 * TILE_SIZE);
    placeKoopa (65 * TILE_SIZE, 13 * TILE_SIZE);

    /* Coins */
    for (int c = 3; c <= 6;  ++c) placeCoin(c*TILE_SIZE, 9*TILE_SIZE);
    for (int c = 16; c<=18; ++c) placeCoin(c*TILE_SIZE, 9*TILE_SIZE);
    for (int c = 36; c<=38; ++c) placeCoin(c*TILE_SIZE, 6*TILE_SIZE);
}

void Level::buildLevel2()
{
    data_.cols = 100;
    data_.rows = 15;
    data_.tiles.assign(data_.rows,
        std::vector<Tile>(data_.cols, Tile(TileType::Empty)));

    /* Underground theme */
    for (int c = 0; c < data_.cols; ++c) {
        data_.tiles[14][c] = Tile(TileType::Ground);
        data_.tiles[0][c]  = Tile(TileType::Ground);
    }
    for (int r = 0; r < data_.rows; ++r) {
        data_.tiles[r][0]             = Tile(TileType::Ground);
        data_.tiles[r][data_.cols-1]  = Tile(TileType::Ground);
    }

    /* platforms */
    auto platform = [&](int row, int c0, int c1, TileType t){
        for (int c = c0; c <= c1; ++c)
            data_.tiles[row][c] = Tile(t);
    };

    platform(10, 5, 15,  TileType::Brick);
    platform( 7, 8, 12,  TileType::Brick);
    platform( 7, 10, 10, TileType::QuestionBlock);
    platform( 4, 5, 20,  TileType::Brick);
    platform(10, 20, 30, TileType::Ground);
    platform( 6, 22, 28, TileType::Brick);
    platform( 6, 25, 25, TileType::QuestionBlock);
    platform( 3, 22, 28, TileType::Brick);
    platform(10, 40, 60, TileType::Brick);
    platform( 7, 45, 55, TileType::Brick);
    platform( 7, 50, 50, TileType::QuestionBlock);

    /* Enemies */
    for (int i = 0; i < 8; ++i)
        placeGoomba((10 + i*10) * TILE_SIZE, 13 * TILE_SIZE);
    for (int i = 0; i < 4; ++i)
        placeKoopa((15 + i*15) * TILE_SIZE, 13 * TILE_SIZE);

    /* Coins */
    for (int c = 6; c <= 14; ++c) placeCoin(c*TILE_SIZE, 3*TILE_SIZE);
    for (int c = 23; c<=27; ++c) placeCoin(c*TILE_SIZE, 5*TILE_SIZE);

    data_.playerStart = {2.0f*TILE_SIZE, 12.0f*TILE_SIZE};
    player_->boundsRef().x = data_.playerStart.x;
    player_->boundsRef().y = data_.playerStart.y;

    /* Flag */
    data_.tiles[4][95] = Tile(TileType::Flag);
    for (int r = 5; r <= 13; ++r)
        data_.tiles[r][95] = Tile(TileType::Ground);
}

void Level::buildLevel3()
{
    buildLevel1(); /* reuse level1 with more enemies */
    placeGoomba(18*TILE_SIZE, 13*TILE_SIZE);
    placeGoomba(20*TILE_SIZE, 13*TILE_SIZE);
    placeKoopa (28*TILE_SIZE, 13*TILE_SIZE);
    placeKoopa (45*TILE_SIZE, 13*TILE_SIZE);
    placeGoomba(70*TILE_SIZE, 13*TILE_SIZE);
}

void Level::placeGoomba(float x, float y)
{
    entities_.push_back(std::make_unique<Goomba>(x, y));
}

void Level::placeKoopa(float x, float y)
{
    entities_.push_back(std::make_unique<Koopa>(x, y));
}

void Level::placeCoin(float x, float y)
{
    entities_.push_back(std::make_unique<Coin>(x, y));
}

/* ── Update ───────────────────────────────────────────────── */
void Level::update(float dt)
{
    if (complete_) {
        completeTimer_ += dt;
        if (completeTimer_ > 3.0f) complete_ = true;
        return;
    }

    timeLeft_ -= dt;
    if (timeLeft_ <= 0) {
        timeLeft_    = 0;
        player_->takeDamage();
    }

    player_->update(dt);
    resolvePlayerTiles();

    for (auto& e : entities_) {
        if (!e->alive()) continue;
        if (auto* g = dynamic_cast<Goomba*>(e.get())) {
            g->update(dt);
            bool og = false;
            resolveEntityTiles(*g, g->boundsRef(), g->velRef(), og);
            g->setOnGround(og);
            /*
            The simplest possible patrol AI: when onGround becomes false (walked off a ledge) reverse horizontal direction. No pathfinding, no lookahead — the Goomba simply bounces back and forth. The Level injects this decision from outside rather than the Goomba doing it internally, keeping the Goomba class focused only on its own state.
            */
            if (!og && !g->isStomped()) g->velRef().x *= -1;
        } else if (auto* k = dynamic_cast<Koopa*>(e.get())) {
            k->update(dt);
            bool og = false;
            resolveEntityTiles(*k, k->boundsRef(), k->velRef(), og);
            k->setOnGround(og);
        } else if (auto* p = dynamic_cast<PowerUp*>(e.get())) {
            p->update(dt);
            bool og = false;
            resolveEntityTiles(*p, p->boundsRef(), p->velRef(), og);
            p->setOnGround(og);
            if (!og) p->velRef().x *= -1;
        } else if (auto* c = dynamic_cast<Coin*>(e.get())) {
            c->update(dt);
        } else if (auto* fb = dynamic_cast<Projectile*>(e.get())) {
            fb->update(dt);
            bool og = false;
            resolveEntityTiles(*fb, fb->boundsRef(), fb->velRef(), og);
            fb->setOnGround(og);
        }
    }

    resolveEntityPlayer();

    /* remove dead entities */
    entities_.erase(
        std::remove_if(entities_.begin(), entities_.end(),
            [](const auto& e){ return !e->alive(); }),
        entities_.end());

    particles_.update(dt);

    /* camera */
    camera_.update(
        player_->boundsRef().centerX(),
        player_->boundsRef().centerY(),
        (float)(data_.cols * TILE_SIZE),
        (float)(data_.rows * TILE_SIZE));

    if (player_->isDead()) playerDead_ = true;
}

/* ── Tile collision ───────────────────────────────────────── */
//Physics uses fixed timestep (FIXED_DT = 1/60). We move in X first, resolve all X collisions, then move in Y and resolve Y collisions. Separating axes prevents the player from clipping through corners when moving diagonally.
void Level::resolvePlayerTiles()
{
    auto& b   = player_->boundsRef();
    auto& v   = player_->velRef();
    bool  og  = false;

    /* move X */
    /*
    Position is updated one axis at a time. Moving X first means we detect and fix horizontal penetration 
    before we ever touch Y. This is called sweep-then-resolve and prevents the diagonal-corner-clipping bug 
    where a fast-moving object teleports through a corner.
    */
    b.x += v.x * FIXED_DT;
    /*
    Converts the AABB's four corners from world-pixel coordinates to tile-grid coordinates by 
    integer-dividing by TILE_SIZE. The -1 on right and bottom prevents a boundary condition where a 
    player whose right edge sits exactly on a tile boundary incorrectly tests the tile to the right.
    */
    int col0 = (int)(b.x            / TILE_SIZE);
    int col1 = (int)((b.right()-1)  / TILE_SIZE);
    int row0 = (int)(b.y            / TILE_SIZE);
    int row1 = (int)((b.bottom()-1) / TILE_SIZE);
/*
For each tile that overlaps the player in X:

Moving right: push player to the tile's left edge (tr.x - b.w)
Moving left: push player to the tile's right edge (tr.right())
Zero velocity to prevent re-penetration next frame
*/
/*
Tests every tile cell the bounding box overlaps. For a 32×32 player in a 32×32 
tile world this is at most a 2×2 grid of tiles — four tests per axis per frame. Very cheap.
*/
    for (int r = row0; r <= row1; ++r) {
        for (int c = col0; c <= col1; ++c) {
            const Tile* t = getTile(c, r);
            if (!t || !t->solid) continue;
            Rect tr = t->worldRect(c, r);
            if (!b.overlaps(tr)) continue;
            /*
            Minimum translation vector for X: push the player to exactly touch the tile edge without overlapping. 
            Moving right → push left edge of player to left edge of tile. Moving left → push right edge of 
            player to right edge of tile. Zeroing velocity prevents re-penetration next frame.
            */
            if (v.x > 0) { b.x = tr.x - b.w; v.x = 0; }
            else          { b.x = tr.right(); v.x = 0; }
        }
    }

    /* move Y */
    //Gravity is applied before Y movement so the player is always pulled down. This is semi-implicit Euler integration — it is slightly more stable than adding gravity after movement.
    v.y += GRAVITY * FIXED_DT;
    b.y += v.y * FIXED_DT;

    col0 = (int)(b.x / TILE_SIZE);
    col1 = (int)((b.right()-1) / TILE_SIZE);
    row0 = (int)(b.y / TILE_SIZE);
    row1 = (int)((b.bottom()-1) / TILE_SIZE);

    for (int r = row0; r <= row1; ++r) {
        for (int c = col0; c <= col1; ++c) {
            Tile* t = getTileMut(c, r);
            if (!t || !t->solid) continue;
            Rect tr = t->worldRect(c, r);
            if (!b.overlaps(tr)) continue;
            /*
            Y collision direction determines the event:

Moving down (falling): landed on top → onGround = true
Moving up (jumping): hit the ceiling → check for block interaction
            */
            if (v.y > 0) {
                b.y = tr.y - b.h;
                v.y = 0;
                og  = true;
            } else {
                /* hit from below */
                b.y = tr.bottom();
                v.y = 0;
                /* hit block interaction */
                if (t->type == TileType::QuestionBlock)
                    hitQuestionBlock(c, r);
                else if (t->type == TileType::Brick &&
                         player_->powerLevel() != PowerLevel::Small)
                    breakBrick(c, r);
            }
        }
    }

    player_->setOnGround(og);

    /* deadly tiles */
    col0 = (int)(b.x / TILE_SIZE);
    col1 = (int)((b.right()-1) / TILE_SIZE);
    row1 = (int)((b.bottom()-1)/ TILE_SIZE);
    for (int c = col0; c <= col1; ++c) {
        const Tile* t = getTile(c, row1);
        if (t && t->deadly) player_->takeDamage();
        if (t && t->type == TileType::Flag) {
            complete_ = true;
            player_->addScore(5000);
            AudioManager::instance().playSound("flagpole");
        }
    }

    /* fall-off bottom */
    if (b.y > data_.rows * TILE_SIZE + 64)
        player_->takeDamage();

    /* screen left clamp */
    if (b.x < 0) { b.x = 0; v.x = 0; }
}

void Level::resolveEntityTiles(Entity& e, Rect& b, Vec2& v,
                                 bool& onGround)
{
    onGround = false;

    b.x += v.x * FIXED_DT;
    int col0 = (int)(b.x / TILE_SIZE);
    int col1 = (int)((b.right()-1) / TILE_SIZE);
    int row0 = (int)(b.y / TILE_SIZE);
    int row1 = (int)((b.bottom()-1)/ TILE_SIZE);

    for (int r = row0; r <= row1; ++r)
        for (int c = col0; c <= col1; ++c) {
            /*
            getTile returns nullptr for out-of-bounds coordinates — safe boundary handling without 
            range checks at call sites. Non-solid tiles (empty, coins, decorations) skip collision entirely.
            */
            const Tile* t = getTile(c, r);
            if (!t || !t->solid) continue;
            /*
            The bounding box test already identified which tiles might overlap by grid cell, 
            but sub-pixel positions mean some don't actually intersect. The overlaps check confirms a 
            real intersection before applying any correction.
            */
            Rect tr = t->worldRect(c, r);
            if (!b.overlaps(tr)) continue;
            v.x = -v.x;
            b.x += v.x * FIXED_DT * 2;
        }

    v.y += GRAVITY * FIXED_DT;
    b.y += v.y * FIXED_DT;

    col0 = (int)(b.x / TILE_SIZE);
    col1 = (int)((b.right()-1) / TILE_SIZE);
    row0 = (int)(b.y / TILE_SIZE);
    row1 = (int)((b.bottom()-1)/ TILE_SIZE);

    for (int r = row0; r <= row1; ++r)
        for (int c = col0; c <= col1; ++c) {
            const Tile* t = getTile(c, r);
            if (!t || !t->solid) continue;
            Rect tr = t->worldRect(c, r);
            if (!b.overlaps(tr)) continue;
            if (v.y > 0) { b.y = tr.y - b.h; v.y=0; onGround=true; }
            else          { b.y = tr.bottom(); v.y=0; }
        }

    /* kill if fell off */
    if (b.y > data_.rows * TILE_SIZE + 128) e.kill();
}

/* ── Entity–Player collision ─────────────────────────────── */
/*
Stomp detection: the player is moving downward (pv.y > 0) AND their bottom edge was ABOVE the enemy's top in the previous frame (playerBottom - pv.y*FIXED_DT). The + 4 gives a small tolerance for near-misses.
*/
void Level::resolveEntityPlayer()
{
    auto& pb = player_->boundsRef();
    auto& pv = player_->velRef();

    for (auto& ep : entities_) {
        if (!ep->alive()) continue;

        /* Coin collection */
        if (ep->type() == EntityType::Coin) {
            auto* coin = static_cast<Coin*>(ep.get());
            if (!coin->collected() && pb.overlaps(coin->bounds())) {
                coin->collect();
                player_->collectCoin();
                particles_.spawnCoinCollect(
                    {coin->bounds().centerX(),
                     coin->bounds().centerY()});
            }
            continue;
        }

        /* Power-up collection */
        if (ep->type() == EntityType::PowerUp) {
            auto* pu = static_cast<PowerUp*>(ep.get());
            if (pb.overlaps(pu->bounds())) {
                switch (pu->puType()) {
                    case PowerUpType::Mushroom:   player_->grow();          break;
                    case PowerUpType::FireFlower:  player_->gainFirePower(); break;
                    case PowerUpType::OneUp:
                        /* gain life */ break;
                    case PowerUpType::Star:
                        player_->addScore(1000); break;
                }
                ep->kill();
                particles_.spawnStarBurst({pu->bounds().centerX(),
                                            pu->bounds().centerY()});
            }
            continue;
        }

        /* Fireball vs enemy */
        if (ep->type() == EntityType::Goomba ||
            ep->type() == EntityType::Koopa) {
            for (auto& ef : entities_) {
                if (!ef->alive() || ef->type()!=EntityType::Fireball) continue;
                if (ef->bounds().overlaps(ep->bounds())) {
                    if (auto* g = dynamic_cast<Goomba*>(ep.get()))
                        g->stomp();
                    if (auto* k = dynamic_cast<Koopa*>(ep.get()))
                        k->stomp();
                    ef->kill();
                    player_->addScore(200);
                }
            }
        }

        /* Player vs enemy */
        if (ep->type() == EntityType::Goomba ||
            ep->type() == EntityType::Koopa) {
            if (!pb.overlaps(ep->bounds())) continue;

            float playerBottom = pb.bottom();
            float enemyTop     = ep->bounds().y;

            /* stomp: player falling onto top of enemy */
            if (pv.y > 0 && playerBottom - pv.y*FIXED_DT <= enemyTop + 4) {
                if (auto* g = dynamic_cast<Goomba*>(ep.get()))
                    g->stomp();
                if (auto* k = dynamic_cast<Koopa*>(ep.get()))
                    k->stomp();
                pv.y = JUMP_FORCE * 0.7f;
                player_->addScore(100);
                particles_.spawnDust(
                    {ep->bounds().centerX(), ep->bounds().y});
            } else {
                /* side collision = damage */
                auto* g = dynamic_cast<Goomba*>(ep.get());
                auto* k = dynamic_cast<Koopa*>(ep.get());
                bool dead = (g && g->isStomped()) ||
                            (k && k->koopaState()==KoopaState::Shell &&
                             k->velocity().x == 0);
                if (!dead)
                    player_->takeDamage();
            }
        }
    }
}

void Level::breakBrick(int col, int row)
{
    Tile* t = getTileMut(col, row);
    if (!t) return;
    t->type    = TileType::Empty;
    t->solid   = false;
    player_->addScore(50);
    particles_.spawnBrickBreak(
        {(float)(col*TILE_SIZE + TILE_SIZE/2),
         (float)(row*TILE_SIZE + TILE_SIZE/2)});
    AudioManager::instance().playSound("break");
}

void Level::hitQuestionBlock(int col, int row)
{
    Tile* t = getTileMut(col, row);
    if (!t || t->type != TileType::QuestionBlock) return;
    t->type  = TileType::HitBlock;
    t->solid = true;
    t->hitCount++;

    /* spawn coin or power-up */
    float px = col * TILE_SIZE;
    float py = (row - 1) * TILE_SIZE;

    if (player_->powerLevel() == PowerLevel::Small) {
        /* coin */
        entities_.push_back(std::make_unique<Coin>(px, py));
        player_->collectCoin();
    } else {
        /* fire flower or mushroom alternating */
        PowerUpType put = (t->hitCount % 2 == 0)
                          ? PowerUpType::FireFlower
                          : PowerUpType::Mushroom;
        entities_.push_back(
            std::make_unique<PowerUp>(px, py, put));
    }
    AudioManager::instance().playSound("block");
}

bool Level::tileAt(int c, int r) const
{
    const Tile* t = getTile(c, r);
    return t && t->solid;
}

const Tile* Level::getTile(int c, int r) const
{
    if (r < 0 || r >= data_.rows ||
        c < 0 || c >= data_.cols) return nullptr;
    return &data_.tiles[r][c];
}

Tile* Level::getTileMut(int c, int r)
{
    if (r < 0 || r >= data_.rows ||
        c < 0 || c >= data_.cols) return nullptr;
    return &data_.tiles[r][c];
}

/* ── Render ───────────────────────────────────────────────── */
/*
Parallax scrolling: clouds move at 30% of the camera speed (cam.x * 0.3f). fmod wraps the position at 1600 pixels so clouds loop seamlessly. This creates the illusion of depth — clouds appear to be in the background, further away than the tiles.
*/
/*
1. Clear screen with sky colour
2. Draw parallax clouds (30% of camera speed)
3. Draw visible tiles (frustum-culled)
4. Draw entities (coins, enemies, power-ups, projectiles)
5. Draw player
6. Draw particles (on top of everything)
7. Draw virtual touch buttons (Android only)
8. HUD draws over everything (called from Game::render)

Each layer is drawn in back-to-front order (painter's algorithm) so later items appear in front of earlier ones.
*/
void Level::render(SDL_Renderer* r)
{
    Vec2 cam = camera_.offset();

    /* sky background */
    SDL_SetRenderDrawColor(r, 107, 140, 255, 255);
    SDL_RenderClear(r);

    /* clouds (decorative) */
    SDL_SetRenderDrawColor(r, 255, 255, 255, 200);
    for (int i = 0; i < 8; ++i) {
        /*
        Breaking this down:

i * 200 — places cloud i at intervals of 200 pixels
cam.x * 0.3f — clouds scroll at 30% of camera speed (parallax depth)
std::fmod(..., 1600.0f) — wraps at 1600 pixels (8 clouds × 200px spacing) so clouds loop seamlessly
The subtraction makes clouds appear to move rightward as the camera moves right
        */
        int cx = (int)((i * 200 - std::fmod(cam.x * 0.3f, 1600.0f)));
        int cy = 60 + (i % 3) * 40;
        SDL_Rect cloud = {cx, cy, 80, 30};
        SDL_RenderFillRect(r, &cloud);
        SDL_Rect cl = {cx-20, cy+10, 40, 20};
        SDL_Rect cr = {cx+60, cy+10, 40, 20};
        SDL_RenderFillRect(r, &cl);
        SDL_RenderFillRect(r, &cr);
    }

    /* tiles */
    int startCol = (int)(cam.x / TILE_SIZE);//Frustum culling: only renders tiles that are actually visible on screen. Without this, rendering 80×15 = 1200 tiles every frame would waste time. With culling, only ~42×24 = ~1000 tiles are checked and at most ~600 are drawn.
    int endCol   = startCol + SCREEN_W / TILE_SIZE + 2;
    int startRow = (int)(cam.y / TILE_SIZE);
    int endRow   = startRow + SCREEN_H / TILE_SIZE + 2;//Frustum culling: only renders tiles that are actually visible on screen. Without this, rendering 80×15 = 1200 tiles every frame would waste time. With culling, only ~42×24 = ~1000 tiles are checked and at most ~600 are drawn.

    startCol = std::max(0, startCol);
    endCol   = std::min(data_.cols-1, endCol);
    startRow = std::max(0, startRow);
    endRow   = std::min(data_.rows-1, endRow);

    for (int row = startRow; row <= endRow; ++row)
        for (int col = startCol; col <= endCol; ++col) {
            const Tile& t = data_.tiles[row][col];
            if (t.type == TileType::Empty) continue;

            SDL_Rect dst = {
                col * TILE_SIZE - (int)cam.x,
                row * TILE_SIZE - (int)cam.y,
                TILE_SIZE, TILE_SIZE
            };

            switch (t.type) {
                /*
                Each tile is drawn with 2-3 SDL_RenderFillRect calls — one for the main body and one for 
                details (grass, shine, pattern). This approach requires no image files and renders instantly. 
                With real sprites you would replace each case with a single 
                SDL_RenderCopy using the correct source rectangle from a sprite sheet.
                */
                case TileType::Ground:
                    SDL_SetRenderDrawColor(r, 140, 80, 20, 255);
                    SDL_RenderFillRect(r, &dst);
                    SDL_SetRenderDrawColor(r, 60, 160, 60, 255);
                    { SDL_Rect top = {dst.x, dst.y, dst.w, 6};
                      SDL_RenderFillRect(r, &top); }
                    break;
                case TileType::Brick:
                    SDL_SetRenderDrawColor(r, 180, 80, 20, 255);
                    SDL_RenderFillRect(r, &dst);
                    SDL_SetRenderDrawColor(r, 200, 100, 40, 255);
                    for (int i = 0; i < 2; ++i) {
                        SDL_Rect line = {dst.x, dst.y+i*16, dst.w, 2};
                        SDL_RenderFillRect(r, &line);
                    }
                    for (int i = 0; i < 2; ++i) {
                        SDL_Rect vl = {dst.x+i*16, dst.y, 2, dst.h};
                        SDL_RenderFillRect(r, &vl);
                    }
                    break;
                case TileType::QuestionBlock:
                    SDL_SetRenderDrawColor(r, 255, 180, 0, 255);
                    SDL_RenderFillRect(r, &dst);
                    SDL_SetRenderDrawColor(r, 255, 255, 80, 255);
                    { SDL_Rect q = {dst.x+10, dst.y+6, 12, 16};
                      SDL_RenderFillRect(r, &q); }
                    break;
                case TileType::HitBlock:
                    SDL_SetRenderDrawColor(r, 100, 60, 10, 255);
                    SDL_RenderFillRect(r, &dst);
                    break;
                case TileType::Pipe:
                    SDL_SetRenderDrawColor(r, 20, 140, 20, 255);
                    SDL_RenderFillRect(r, &dst);
                    SDL_SetRenderDrawColor(r, 40, 180, 40, 255);
                    { SDL_Rect hl = {dst.x+2, dst.y, 6, dst.h};
                      SDL_RenderFillRect(r, &hl); }
                    break;
                case TileType::PipeTop:
                    SDL_SetRenderDrawColor(r, 20, 140, 20, 255);
                    SDL_RenderFillRect(r, &dst);
                    { SDL_Rect brd = {dst.x-2, dst.y, dst.w+4, dst.h};
                      SDL_SetRenderDrawColor(r, 40, 180, 40, 255);
                      SDL_RenderDrawRect(r, &brd); }
                    break;
                case TileType::Flag: {
                    SDL_SetRenderDrawColor(r, 60, 60, 60, 255);
                    SDL_Rect pole = {dst.x+14, dst.y, 4, dst.h};
                    SDL_RenderFillRect(r, &pole);
                    SDL_SetRenderDrawColor(r, 60, 200, 60, 255);
                    SDL_Rect flag = {dst.x+18, dst.y+4, 14, 10};
                    SDL_RenderFillRect(r, &flag);
                    break;
                }
                default: break;
            }

            /* tile border */
            SDL_SetRenderDrawColor(r, 0, 0, 0, 40);
            SDL_RenderDrawRect(r, &dst);
        }

    /* entities */
    for (const auto& e : entities_)
        if (e->alive()) e->render(r, cam);

    /* player */
    player_->render(r, cam);

    /* particles */
    particles_.render(r, cam);

    /* virtual touch buttons on mobile */
    /*
    The buttons are drawn only on Android (#ifdef __ANDROID__). Alpha 60 (≈24% opacity) makes 
    them semi-transparent overlays that hint at button positions without obscuring the gameplay. 
    The coordinates match exactly the zones defined in InputManager (zLeft_, zRight_ etc.) — they 
    must be kept in sync manually.
    */
#ifdef __ANDROID__
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 255, 255, 255, 60);
    SDL_Rect btnL  = {0,   550, 100, 150};
    SDL_Rect btnR  = {110, 550, 100, 150};
    SDL_Rect btnJmp= {1150,550, 130, 150};
    SDL_Rect btnRun= {1000,550, 130, 150};
    SDL_RenderFillRect(r, &btnL);
    SDL_RenderFillRect(r, &btnR);
    SDL_RenderFillRect(r, &btnJmp);
    SDL_RenderFillRect(r, &btnRun);
    SDL_SetRenderDrawColor(r, 255,255,255, 150);
    SDL_RenderDrawRect(r, &btnL);
    SDL_RenderDrawRect(r, &btnR);
    SDL_RenderDrawRect(r, &btnJmp);
    SDL_RenderDrawRect(r, &btnRun);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
#endif
}

} /* namespace Mario */
