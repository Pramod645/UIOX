#include "InputManager.h"
#include "Utils.h"

namespace Mario {
/*
Called every frame before event processing. Copies cur_ into prev_ so we have a snapshot of last frame's state. Resets touch buttons to false — they must be re-asserted each frame by active touch events.
*/
void InputManager::update()
{
    prev_ = cur_;
    tLeft_ = tRight_ = tJump_ = tRun_ = false;
}
/*
e.key.repeat is non-zero when the OS repeats a held key. We ignore repeats for isPressed detection because we want each physical key-press to register once only.
*/
void InputManager::handleEvent(const SDL_Event& e)
{
    switch (e.type) {
        case SDL_QUIT:
            quit_ = true;
            break;

        case SDL_KEYDOWN:
            if (!e.key.repeat)
                cur_[e.key.keysym.scancode] = true;
            if (e.key.keysym.scancode == SDL_SCANCODE_AC_BACK ||
                e.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                back_ = true;
            break;

        case SDL_KEYUP:
            cur_[e.key.keysym.scancode] = false;
            break;
//SDL gamepad axis values range from -32768 to +32767. Dividing by 32768 normalises to -1.0 to +1.0 (a tiny asymmetry at the negative end is acceptable).
        case SDL_CONTROLLERBUTTONDOWN:
            /* map gamepad buttons */
            if (e.cbutton.button == SDL_CONTROLLER_BUTTON_A)
                cur_[SDL_SCANCODE_SPACE] = true;
            if (e.cbutton.button == SDL_CONTROLLER_BUTTON_B)
                cur_[SDL_SCANCODE_LSHIFT] = true;
            break;
//SDL2 touch coordinates are normalised (0.0–1.0). Multiplying by the logical screen size converts them to pixel coordinates matching the virtual button zones.
        case SDL_CONTROLLERBUTTONUP:
            if (e.cbutton.button == SDL_CONTROLLER_BUTTON_A)
                cur_[SDL_SCANCODE_SPACE] = false;
            if (e.cbutton.button == SDL_CONTROLLER_BUTTON_B)
                cur_[SDL_SCANCODE_LSHIFT] = false;
            break;
//SDL2 touch coordinates are normalised (0.0–1.0). Multiplying by the logical screen size converts them to pixel coordinates matching the virtual button zones.
        case SDL_CONTROLLERAXISMOTION:
            if (e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX)
                axisX_ = e.caxis.value / 32768.0f;
            if (e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY)
                axisY_ = e.caxis.value / 32768.0f;
            break;

        case SDL_FINGERDOWN:
        case SDL_FINGERMOTION: {
            float sx = e.tfinger.x * SCREEN_W;
            float sy = e.tfinger.y * SCREEN_H;
            processTouchDown(sx, sy);
            break;
        }
        case SDL_FINGERUP: {
            float sx = e.tfinger.x * SCREEN_W;
            float sy = e.tfinger.y * SCREEN_H;
            processTouchUp(sx, sy);
            break;
        }
    }
}
//Merges keyboard, gamepad axis, and touch into one unified query. Game logic calls isDown(SDL_SCANCODE_LEFT) and gets true whether the player pressed the keyboard arrow, pushed the gamepad stick, or touched the left zone.
bool InputManager::isDown(SDL_Scancode k) const
{
    auto it = cur_.find(k);
    if (it == cur_.end()) return false;
    /* also check gamepad axis */
    if (k == SDL_SCANCODE_RIGHT && axisX_ > 0.3f) return true;
    if (k == SDL_SCANCODE_LEFT  && axisX_ <-0.3f) return true;
    return it->second || tLeft_  && k==SDL_SCANCODE_LEFT
                      || tRight_ && k==SDL_SCANCODE_RIGHT
                      || tJump_  && k==SDL_SCANCODE_SPACE
                      || tRun_   && k==SDL_SCANCODE_LSHIFT;
}

bool InputManager::isPressed(SDL_Scancode k) const
{
    auto cit = cur_.find(k);
    auto pit = prev_.find(k);
    bool c = cit != cur_.end()  && cit->second;
    bool p = pit != prev_.end() && pit->second;
    return c && !p;
}

bool InputManager::isReleased(SDL_Scancode k) const
{
    auto cit = cur_.find(k);
    auto pit = prev_.find(k);
    bool c = cit != cur_.end()  && cit->second;
    bool p = pit != prev_.end() && pit->second;
    return !c && p;
}

void InputManager::processTouchDown(float sx, float sy)
{
    auto inRect = [&](const SDL_Rect& z) {
        return sx >= z.x && sx <= z.x+z.w &&
               sy >= z.y && sy <= z.y+z.h;
    };
    if (inRect(zLeft_))  tLeft_  = true;
    if (inRect(zRight_)) tRight_ = true;
    if (inRect(zJump_))  tJump_  = true;
    if (inRect(zRun_))   tRun_   = true;
}

void InputManager::processTouchUp(float sx, float sy)
{
    auto inRect = [&](const SDL_Rect& z) {
        return sx >= z.x && sx <= z.x+z.w &&
               sy >= z.y && sy <= z.y+z.h;
    };
    if (inRect(zLeft_))  tLeft_  = false;
    if (inRect(zRight_)) tRight_ = false;
    if (inRect(zJump_))  tJump_  = false;
    if (inRect(zRun_))   tRun_   = false;
}

} /* namespace Mario */
