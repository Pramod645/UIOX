#include "ParticleSystem.h"
#include <algorithm>
#include <random>
namespace Mario {

void ParticleSystem::spawn(Vec2 pos, Vec2 vel, SDL_Color col,
                            float life, float size)
{ 
    Particle p;
    p.pos    = pos;
    p.vel    = vel;
    p.color  = col;
    p.life   = p.maxLife = life;
    p.size   = size;
    p.alive  = true;
    particles_.push_back(p);
}
//Spawns 8 brown particles with random horizontal velocities and upward velocities between -100 and -400 px/s. Using pos.x as the RNG seed means the same brick always breaks the same way — deterministic but varied.
void ParticleSystem::spawnBrickBreak(Vec2 pos)
{
    std::uniform_real_distribution<float>
        vx(-200,200), vy(-400,-100), sz(4,10);
    std::mt19937 rng(pos.x);
    for (int i = 0; i < 8; ++i) {
        spawn(pos,
              {vx(rng), vy(rng)},
              {180, 100, 20, 255},
              0.8f, sz(rng));
    }
}
/*
Distributes 6 particles evenly around a circle using polar coordinates: angle = i * (2π / 6). cos(angle) 
gives X and sin(angle) gives Y for each direction. The - 80 on Y biases all particles slightly upward — the burst 
looks like it rises rather than expanding symmetrically.
*/
void ParticleSystem::spawnCoinCollect(Vec2 pos)
{
    for (int i = 0; i < 6; ++i) {
        float angle = i * 3.14159f * 2 / 6;
        Vec2 v = {std::cos(angle)*150, std::sin(angle)*150 - 80};
        spawn(pos, v, {255, 210, 0, 255}, 0.5f, 5);
    }
}

void ParticleSystem::spawnStarBurst(Vec2 pos)
{
    for (int i = 0; i < 12; ++i) {
        float a = i * 3.14159f * 2 / 12;
        Vec2 v  = {std::cos(a)*200, std::sin(a)*200};
        SDL_Color c = {255, (Uint8)(200 - i*10), 0, 255};
        spawn(pos, v, c, 0.6f, 6);
    }
}

void ParticleSystem::spawnDust(Vec2 pos)
{
    for (int i = 0; i < 4; ++i) {
        Vec2 v = {(float)(i%2==0 ? -60:60), -40.0f};
        spawn(pos, v, {200,200,200,180}, 0.3f, 4);
    }
}
//p.color.a fades from 255 to 0 as p.life decreases — particles become transparent as they die. This is the key visual effect of all particle systems: position = physics, alpha = lifetime.
/*
Three things happen every frame per particle:

Gravity drags them downward at 200 px/s² (lighter than player gravity)
Position advances by velocity × time (Euler integration)
Alpha linearly interpolates from 255 (fully visible) down to 0 (invisible) as life decreases toward 0
*/
void ParticleSystem::update(float dt)
{
    for (auto& p : particles_) {
        if (!p.alive) continue;
        p.life -= dt;
        if (p.life <= 0) { p.alive = false; continue; }
        p.vel.y += 200.0f * dt;
        p.pos   += p.vel * dt;
        p.color.a = (Uint8)(255 * (p.life / p.maxLife));
    }
    particles_.erase(//The erase-remove idiom: std::remove_if moves all dead particles to the end of the vector and returns an iterator to the first dead one; erase then removes from there to the end. This is O(n) and avoids repeated vector shifts.
        std::remove_if(particles_.begin(), particles_.end(),
            [](const Particle& p){ return !p.alive; }),
        particles_.end());
}

void ParticleSystem::render(SDL_Renderer* r, const Vec2& cam)
{
    for (const auto& p : particles_) {
        if (!p.alive) continue;
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, p.color.r, p.color.g,
                                  p.color.b, p.color.a);
        SDL_Rect dst = {
            (int)(p.pos.x - cam.x - p.size/2),
            (int)(p.pos.y - cam.y - p.size/2),
            (int)p.size, (int)p.size
        };
        SDL_RenderFillRect(r, &dst);
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

} /* namespace Mario */
