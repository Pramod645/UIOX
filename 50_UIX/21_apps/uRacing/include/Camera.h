#pragma once
#include "Utils.h"

namespace Mario {
/*
Each frame the camera is told where the player is and the level boundaries so it 
can follow the player while staying within bounds.
*/
class Camera {
public:
    void update(float targetX, float targetY,
                float levelW, float levelH);

    Vec2  offset()  const { return offset_; }//Returns the current scroll offset. Subtracting this from a world position gives the screen position.
    float x()       const { return offset_.x; }
    float y()       const { return offset_.y; }

    /* world → screen */
    SDL_Rect worldToScreen(const Rect& r) const;//Converts a world-space Rect to an integer SDL_Rect in screen coordinates by subtracting the camera offset — used for all drawing calls.

private:
    Vec2 offset_;
    static constexpr float LERP = 0.12f;//Linear interpolation factor (12% per frame). The camera moves 12% of the remaining distance to the target each frame — creates a smooth lag effect rather than snapping instantly. Higher = snappier, lower = floatier.
    float ox_ = 0, oy_ = 0; //The actual interpolated position of the camera, updated each frame with the lerp formula: ox_ += (targetOx - ox_) * LERP.
};

} /* namespace Mario */
