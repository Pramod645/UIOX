#pragma once
#include "Entity.h"
#include "InputManager.h"

namespace Mario {
//Tracks the player's current behaviour. The state drives which animation plays and which physics rules apply — e.g. the player can only jump when onGround_ is true.
//The state machine in Player::applyInput drives both animation and physics:
/*
                    ┌──────────────────────┐
             SPACE  │                      │ land
    Idle ──────────►│        Jump          │────► Idle
     │              │                      │
     │              └──────────────────────┘
     │  ←/→ key                │ no jump held,
     ▼                         │ vel.y > 0
    Walk ──►Run (Shift)        ▼
     │                        Fall
     │  take damage             │
     ▼                         │ land
    Die ◄──────────────────────┘

*/
enum class PlayerState {
    Idle, Walk, Run, Jump, Fall, Crouch, Die, Invincible
};
//Three power levels matching classic Mario — each changes the player's hitbox size and available abilities.
enum class PowerLevel { Small, Big, Fire };

class Player : public Entity {
public:
    Player();

    void update(float dt) override;
    void render(SDL_Renderer* r, const Vec2& camOffset) override;
    void onCollideWith(Entity* other) override;

    void setInput(const InputManager* inp) { input_ = inp; }//Dependency injection — the Player doesn't know where input comes from, it just reads from the interface pointer. Lets us swap keyboard for AI or replay without changing Player.
    //Public game-event methods called by the Level's collision resolver. Keeping them as named methods (rather than direct field writes) means all side effects (sounds, score, state changes) happen in one place.
    void onLanding();
    void takeDamage();
    void collectCoin();
    void grow();
    void gainFirePower();
    bool isDead()  const { return state_ == PlayerState::Die; }
    bool canShoot()const;
    void shoot(std::vector<Entity*>& entities);

    int         lives()      const { return lives_; }
    int         score()      const { return score_; }
    int         coins()      const { return coins_; }
    PowerLevel  powerLevel() const { return power_; }
    PlayerState state()      const { return state_; }
    void        addScore(int s)    { score_ += s; }

    /* physics callbacks from Level */
    void setOnGround(bool v) { onGround_ = v; }
    void setVelY(float vy)   { vel_.y    = vy; }
    void setVelX(float vx)   { vel_.x    = vx; }
    //Return references to the private physics fields so the Level collision resolver can directly modify position and velocity without making them fully public. A deliberate narrow exception to encapsulation.
    Vec2& velRef()            { return vel_; }
    Rect& boundsRef()         { return bounds_; }

private:
    const InputManager* input_  = nullptr;
    PlayerState         state_  = PlayerState::Idle;
    PowerLevel          power_  = PowerLevel::Small;
    int                 lives_  = 3;
    int                 score_  = 0;
    int                 coins_  = 0;
    float               invTime_= 0;    /* invincibility timer  *///Frame-accurate timers — decremented each frame by dt. When they reach zero the corresponding effect ends. This is the standard "timer as float" pattern in game development.
    float               dieTime_= 0;//Frame-accurate timers — decremented each frame by dt. When they reach zero the corresponding effect ends. This is the standard "timer as float" pattern in game development.
    float               shootCd_= 0;//Frame-accurate timers — decremented each frame by dt. When they reach zero the corresponding effect ends. This is the standard "timer as float" pattern in game development.
    bool                running_= false;

    /* animation frames per state */
    static constexpr int ANIM_IDLE  = 0;
    static constexpr int ANIM_WALK  = 1;
    static constexpr int ANIM_JUMP  = 4;
    static constexpr int ANIM_DIE   = 5;

    void updateAnimation(float dt);
    void drawSprite(SDL_Renderer* r, const Vec2& cam,
                    int row, int col, int w, int h);
    void applyInput(float dt);
    void clampVelocity();
};

} /* namespace Mario */
