#pragma once
#include "Enemy.h"

namespace Mario {
/*
Koopa has three distinct states:

Walk — normal patrol
Shell — stomped, stationary shell that can be kicked
SlideShell — kicked shell flying at high speed, kills enemies
*/
enum class KoopaState { Walk, Shell, SlideShell };

class Koopa : public Enemy {
public:
    Koopa(float x, float y);
    void update(float dt) override;
    void render(SDL_Renderer* r, const Vec2& cam) override;
    void stomp() override;
    void kick();//Called when the player touches a stationary shell — launches it sliding in the direction the player is facing.
    void onCollideWith(Entity* other) override;

    KoopaState koopaState() const { return kState_; }
    Rect& boundsRef()             { return bounds_; }
    Vec2& velRef()                { return vel_; }
    void  setOnGround(bool v)     { onGround_ = v; }

private:
    KoopaState kState_ = KoopaState::Walk;
    float      shellTimer_ = 0;
};

} /* namespace Mario */
