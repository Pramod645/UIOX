#pragma once
#include "Utils.h"
#include <SDL2/SDL.h>
#include <string>

namespace Mario {

enum class EntityType {
    Player,
    Goomba,
    Koopa,
    Coin,
    PowerUp,
    Fireball,
    Particle,
};
/*
The base class for every game object. virtual ... = 0 makes these pure virtual — every subclass 
(Player, Goomba, Coin…) must implement its own update and render. This is the polymorphism that 
lets the level store std::unique_ptr<Entity> and call the right behaviour for each object.
*/
class Entity {
public:
    explicit Entity(EntityType type);
    virtual ~Entity() = default;

    virtual void update(float dt) = 0;
    virtual void render(SDL_Renderer* r, const Vec2& camOffset) = 0;
    virtual void onCollideWith(Entity* other) {}//Default empty implementation — subclasses override only if they care about collisions with other entities.

    bool      alive()    const { return alive_; } //Rather than immediately removing entities from the list (which would corrupt iteration), we mark them dead and remove them in a batch at the end of the frame.
    EntityType type()    const { return type_;  }
    Rect       bounds()  const { return bounds_;}
    Vec2       velocity()const { return vel_;   }

    void kill() { alive_ = false; } //Rather than immediately removing entities from the list (which would corrupt iteration), we mark them dead and remove them in a batch at the end of the frame.
/*
bounds_ — the AABB used for both physics and rendering
vel_ — velocity in pixels per second
acc_ — acceleration (force divided by mass)
onGround_ — set by the physics resolver each frame, used to allow jumping
facingRight_ — controls sprite flip direction
*/
protected:
    EntityType type_;
    Rect       bounds_;
    Vec2       vel_;
    Vec2       acc_;
    bool       alive_      = true;
    bool       onGround_   = false;
    bool       facingRight_= true;

    /* animation */
    /*
    Simple frame-counter animation system. frameTime_ accumulates delta time; when it exceeds 
    frameDur_ we advance to the next frame and reset. numFrames_ wraps the counter.
    */
    int   frame_    = 0;
    float frameTime_= 0;
    float frameDur_ = 0.1f;
    int   numFrames_= 1;
};

} /* namespace Mario */
