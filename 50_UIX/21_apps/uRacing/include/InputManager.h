#pragma once
#include <SDL2/SDL.h>
#include <unordered_map>

namespace Mario {
/*
Centralises all input handling so game logic never calls SDL_GetKeyboardState 
directly — makes the system easy to test and replace (e.g. swap keyboard for 
gamepad without changing any game code).
*/
class InputManager {
public:
/* update, Called once per frame before events are processed. 
Copies the current key state into prev_ so we can detect "just pressed" 
(was up, now down) and "just released" (was down, now up). */
    void update();
    /*
    Three different query modes:
isDown — key is held right now
isPressed — key went from up to down this frame only
isReleased — key went from down to up this frame only
    */
    bool isDown    (SDL_Scancode k) const;
    bool isPressed (SDL_Scancode k) const;
    bool isReleased(SDL_Scancode k) const;

    /* touch / gamepad axis (normalised -1..1) */
    //Analogue gamepad stick values from -1.0 (full left/up) to +1.0 (full right/down).
    float axisX() const { return axisX_; }
    float axisY() const { return axisY_; }

    bool quit()  const { return quit_; }
    bool back()  const { return back_; }

    /*
    Virtual button states derived from touch finger positions on 
    screen — used on Android since there is no physical keyboard.
    */
    /* Touch virtual buttons */
    bool touchLeft()  const { return tLeft_;  }
    bool touchRight() const { return tRight_; }
    bool touchJump()  const { return tJump_;  }
    bool touchRun()   const { return tRun_;   }

    void handleEvent(const SDL_Event& e);

private:
/*
    std::unordered_map<SDL_Scancode, bool> cur_, prev_;

Two hash maps — one for the current frame's key states and one for the previous 
frame's. Comparing them lets us detect edges (pressed / released).
*/
    std::unordered_map<SDL_Scancode, bool> cur_, prev_;
    float axisX_ = 0, axisY_ = 0;
    bool  quit_ = false, back_ = false;
    bool  tLeft_ = false, tRight_ = false,
          tJump_ = false, tRun_   = false;
/*
Screen zones for the four virtual buttons at 1280×720 logical 
resolution. These rectangles define where the player must touch to trigger each action.
*/
    /* virtual button zones (for touch) */
    SDL_Rect zLeft_  = {0,   550, 100, 150};
    SDL_Rect zRight_ = {110, 550, 100, 150};
    SDL_Rect zJump_  = {1150,550, 130, 150};
    SDL_Rect zRun_   = {1000,550, 130, 150};

    void processTouchDown(float sx, float sy);
    void processTouchUp  (float sx, float sy);
};

} /* namespace Mario */
