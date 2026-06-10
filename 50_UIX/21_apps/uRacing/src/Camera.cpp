#include "Camera.h"

namespace Mario {
//Computes the offset that would centre the camera on the target. Subtracting half the screen size places the target in the middle of the viewport.
void Camera::update(float tx, float ty,
                     float levelW, float levelH)
{
    float targetOx = tx - SCREEN_W / 2.0f;
    float targetOy = ty - SCREEN_H / 2.0f;
//Exponential smoothing (lerp): each frame the camera moves 12% (LERP = 0.12) of the remaining distance. This converges but never quite reaches the target, creating a natural trailing motion.
    ox_ += (targetOx - ox_) * LERP;
    oy_ += (targetOy - oy_) * LERP;

    /* clamp to level bounds */
    //Clamps the camera so it never shows area outside the level boundaries. Without this the camera would scroll past the level edges revealing empty black space.
    ox_ = std::max(0.0f, std::min(ox_, levelW - SCREEN_W));
    oy_ = std::max(0.0f, std::min(oy_, levelH - SCREEN_H));

    offset_ = {ox_, oy_};
}

SDL_Rect Camera::worldToScreen(const Rect& r) const
{
    return {
        (int)(r.x - offset_.x),
        (int)(r.y - offset_.y),
        (int)r.w, (int)r.h
    };
}

} /* namespace Mario */
