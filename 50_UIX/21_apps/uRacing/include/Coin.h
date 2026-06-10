#pragma once
#include "Entity.h"

namespace Mario {

class Coin : public Entity {
public:
    Coin(float x, float y, bool animated = true);
    void update(float dt) override;
    void render(SDL_Renderer* r, const Vec2& cam) override;
    //The coin marks itself collected and dead in a single operation. The Level checks collected() before alive() to trigger the score/sound effects at the moment of collection.
    bool collected() const { return collected_; }
    void collect()         { collected_ = true; kill(); }

private:
    bool  collected_ = false;
    bool  animated_  = true;
    //Used to produce the vertical bobbing sine-wave animation: bobHeight_ = sin(bobTime_) * 4.0f. The coin hovers up and down 4 pixels.
    float bobTime_   = 0;
    float bobHeight_ = 0;
};

} /* namespace Mario */
