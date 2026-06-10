#pragma once
#include "Entity.h"

namespace Mario {
//Type determines both the visual appearance and the effect applied to the player on collection.
enum class PowerUpType {
    Mushroom,    /* grow big                 */
    FireFlower,  /* fire power               */
    Star,        /* invincibility            */
    OneUp,       /* extra life               */
};

class PowerUp : public Entity {
public:
    PowerUp(float x, float y, PowerUpType t);
    void update(float dt) override;
    void render(SDL_Renderer* r, const Vec2& cam) override;

    PowerUpType puType() const { return puType_; }
    Rect& boundsRef()          { return bounds_; }
    Vec2& velRef()             { return vel_; }
    void  setOnGround(bool v)  { onGround_ = v; }

private:
    PowerUpType puType_;
    //When a Question Block is hit, the power-up slides upward out of the block for 0.5 seconds (popTimer_) before becoming active. popped_ prevents the pop phase from repeating.
    float       popTimer_ = 0;   /* rising out of block */
    bool        popped_   = false;
};

} /* namespace Mario */
