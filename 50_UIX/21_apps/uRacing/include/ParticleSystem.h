#pragma once
#include "Utils.h"
#include <SDL2/SDL.h>
#include <vector>

namespace Mario {
   /*
    Each particle is a self-contained data record. life / maxLife gives a 0→1 ratio used to fade the alpha: as life decreases toward 0, the particle becomes transparent.
    */
struct Particle {
    Vec2      pos, vel;
    SDL_Color color;
    float     life     = 1.0f;
    float     maxLife  = 1.0f;
    float     size     = 4.0f;
    bool      alive    = true;
};

class ParticleSystem {
public:
/*
Named spawn functions — each sets a different combination of velocities, colours, sizes, and lifetimes. Centralising them here means the Level just calls particles_.spawnBrickBreak(pos) without knowing particle internals.
*/
    void spawnBrickBreak (Vec2 pos);
    void spawnCoinCollect(Vec2 pos);
    void spawnStarBurst  (Vec2 pos);
    void spawnDust       (Vec2 pos);

    void update(float dt);
    void render(SDL_Renderer* r, const Vec2& camOffset);

private:
    std::vector<Particle> particles_;
    void spawn(Vec2 pos, Vec2 vel, SDL_Color col,
               float life, float size);
};

} /* namespace Mario */
