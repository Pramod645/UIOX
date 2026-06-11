#pragma once
#include "Utils.h"
#include "InputManager.h"
#include "GameState.h"
//#include <SDL2/SDL.h>
//#include <SDL2/SDL_ttf.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <vector>
#include <string>
#include <functional>

namespace Mario {
/*
A menu item combines its display text with a callable action stored as std::function<void()>. Lambda captures let menu items refer back to the Game object: {"START GAME", [this]{ startLevel(1); }}.
*/
struct MenuItem {
    std::string            label;
    std::function<void()>  action;
};

class Menu {
public:
    Menu();
    ~Menu();

    void setItems(std::vector<MenuItem> items);
    void update  (const InputManager& inp, float dt);
    void render  (SDL_Renderer* r);

    GameState nextState() const { return nextState_; }
    bool      hasNext()   const { return hasNext_;   }
    void      clearNext()       { hasNext_ = false;  }

private:
    std::vector<MenuItem> items_;
    //selected_ is the currently highlighted item index. blink_ toggles every 0.5 seconds to produce the flashing cursor effect on the selected item.
    int       selected_  = 0;
    float     blinkTime_ = 0;
    bool      blink_     = true;
    TTF_Font* fontBig_   = nullptr;
    TTF_Font* fontSml_   = nullptr;
    GameState nextState_ = GameState::MainMenu;
    bool      hasNext_   = false;

    /* touch selection */
    void handleTouch(float sx, float sy);
};

} /* namespace Mario */
