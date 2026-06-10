#pragma once
#include "Entity.h"

namespace Mario {
//Adds a stomp() pure virtual method — all enemies must define what happens when the player jumps on them, but the implementation differs (Goomba dies, Koopa enters shell mode).
class Enemy : public Entity {
public:
    explicit Enemy(EntityType t);
    virtual void stomp() = 0;  /* called when player jumps on it */
    bool isStomped()   const { return stomped_; }
/*
stomped_ — prevents re-triggering stomp effects
stompTimer — enemies play a flattened animation before disappearing
moveSpeed_ — walking speed in pixels per second
patrolling_ — flag for future AI expansion (patrol vs chase)
*/
protected:
    bool  stomped_  = false;
    float stompTimer= 0;
    float moveSpeed_= 80.0f;
    bool  patrolling_= true;
};

} /* namespace Mario */
