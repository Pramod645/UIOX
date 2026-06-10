#pragma once
#include "Tile.h"
#include "Player.h"
#include "Goomba.h"
#include "Koopa.h"
#include "Coin.h"
#include "PowerUp.h"
#include "Projectile.h"
#include "ParticleSystem.h"
#include "Camera.h"
#include <vector>
#include <memory>
#include <string>

namespace Mario {
//A 2D grid of Tiles stored as a vector-of-vectors. tiles[row][col] — row-major order matching screen space where Y increases downward.
struct LevelData {
    int         cols       = 0;
    int         rows       = 0;
    std::vector<std::vector<Tile>> tiles;
    Vec2        playerStart;
    Vec2        flagPos;
    float       timeLimit   = 400.0f;
    std::string bgColor;
    int         musicTrack  = 0;
};
/*
Physics is split into four separate resolvers:

Player vs. tiles (most complex — handles block breaking and flag)
Any entity vs. tiles (shared physics for enemies and projectiles)
Entities vs. player (stomp detection, damage, coin pickup)
Entities vs. entities (fireball kills enemies)
*/
class Level {
public:
    explicit Level(int levelNum);

    void update(float dt);
    void render(SDL_Renderer* r);

    bool  isComplete()  const { return complete_; }
    bool  playerDied()  const { return playerDead_; }
    float timeLeft()    const { return timeLeft_; }
    int   levelNum()    const { return levelNum_; }

    Player&        player()       { return *player_; }
    const Player&  player() const { return *player_; }
    Camera&        camera()       { return camera_; }
    ParticleSystem& particles()   { return particles_; }

private:
    int                             levelNum_;
    LevelData                       data_;
    std::unique_ptr<Player>         player_;
    std::vector<std::unique_ptr<Entity>> entities_;
    Camera                          camera_;
    ParticleSystem                  particles_;
    float                           timeLeft_;
    bool                            complete_  = false;
    bool                            playerDead_= false;
    float                           completeTimer_ = 0;

    /* physics */
    void resolvePlayerTiles    ();
    void resolveEntityTiles    (Entity& e, Rect& bounds, Vec2& vel,
                                bool& onGround);
    void resolveEntityPlayer   ();
    void resolveEntityEntity   ();
    void spawnPowerUp          (float x, float y, PowerUpType t);
    void breakBrick            (int col, int row);//These are called from the physics resolver when the player hits a tile from below. They modify the tile type and spawn particles or power-ups as side effects.
    void hitQuestionBlock      (int col, int row);//These are called from the physics resolver when the player hits a tile from below. They modify the tile type and spawn particles or power-ups as side effects.
    bool tileAt                (int col, int row) const;
    const Tile* getTile        (int col, int row) const;
    Tile*       getTileMut     (int col, int row);

    /* level loading */
    void loadLevel             (int n);
    void buildLevel1           ();
    void buildLevel2           ();
    void buildLevel3           ();
    void placeGoomba           (float x, float y);
    void placeKoopa            (float x, float y);
    void placeCoin             (float x, float y);
};

} /* namespace Mario */
