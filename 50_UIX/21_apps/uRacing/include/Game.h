#pragma once
#include "GameState.h"
#include "Renderer.h"
#include "InputManager.h"
#include "Level.h"
#include "HUD.h"
#include "Menu.h"
#include <memory>

namespace Mario {

class Game {
public:
    Game();
    ~Game() = default;

    bool init();
    void run();
    void shutdown();

private:
    Renderer        renderer_;
    InputManager    input_;
    std::unique_ptr<Level> level_;
    std::unique_ptr<HUD>   hud_;
    std::unique_ptr<Menu>  menu_;
    GameState       state_   = GameState::MainMenu;
    int             levelNum_= 1;
    bool            running_ = false;
    Uint64          lastTime_= 0;
    float           accumDt_ = 0;

    /* state transitions */
    void startLevel(int n);
    void showMainMenu();
    void showGameOver();
    void showLevelComplete();

    /* per-frame logic */
    void update(float dt);
    void render();
    void handleEvents();

    /* input-state routing */
    void updateMenu     (float dt);
    void updatePlaying  (float dt);
    void updatePaused   (float dt);
    void updateGameOver (float dt);
    void updateComplete (float dt);
};

} /* namespace Mario */
