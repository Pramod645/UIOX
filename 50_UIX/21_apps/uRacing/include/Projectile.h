#pragma once
#include "Entity.h"

namespace Mario {

class Projectile : public Entity {
public:
    Projectile(float x, float y, bool goRight);
    void update(float dt) override;
    void render(SDL_Renderer* r, const Vec2& cam) override;

    Rect& boundsRef() { return bounds_; }
    Vec2& velRef()    { return vel_; }
    void  setOnGround(bool v) { onGround_ = v; }

private:
/*
Fireballs disappear after 3 seconds regardless — prevents the level from filling up with infinite projectiles if the player spams fire.

The fireball bounces on the ground (vel_.y = -250.0f when onGround_), giving it the characteristic bouncing motion.
*/
    float lifeTime_ = 3.0f;
};

} /* namespace Mario */
