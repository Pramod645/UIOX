#pragma once
#include "Enemy.h"

namespace Mario {
/*
override is a C++11 keyword that tells the compiler "this is intentionally overriding a virtual function." If the base class signature changes, the compiler immediately reports an error instead of silently creating a new unrelated function.
*/
class Goomba : public Enemy {
public:
    Goomba(float x, float y);
    void update(float dt) override;
    void render(SDL_Renderer* r, const Vec2& cam) override;
    void stomp() override;
    void onCollideWith(Entity* other) override;

    /* physics callbacks */
    //Accessors needed by the Level's physics resolver. Same pattern as Player — a deliberate narrow hole in encapsulation for physics.
    void setOnGround(bool v) { onGround_ = v; }
    Rect& boundsRef()        { return bounds_; }
    Vec2& velRef()           { return vel_; }
};

} /* namespace Mario */
