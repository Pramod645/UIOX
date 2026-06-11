#include "Game.h"
#include "AssetManager.h"
#include "AudioManager.h"
//#include "StyleSheet.h"
//#include <SDL2/SDL.h>
#include <SDL.h>

namespace Mario {
/*
MainMenu
  │ startLevel(n)
  ▼
Playing ──P key──► Paused
  │                  │ P key
  │ ◄───────────────┘
  │
  ├── level complete ──► LevelComplete ──SPACE──► startLevel(n+1)
  │                                              or MainMenu if n==3
  │
  └── player died
        │
        ├── lives > 0 ──► startLevel(same) (restart)
        └── lives == 0 ──► GameOver ──SPACE──► startLevel(same)
                                      ESC ──► MainMenu

*/
Game::Game() {}
/*
Initialisation order matters: renderer first (creates the SDL window and renderer), 
then AssetManager (needs the renderer to create textures), then AudioManager (initialises SDL_mixer independently).
*/
bool Game::init()
{
    if (!renderer_.init("Super Mario Adventure",
                         SCREEN_W, SCREEN_H))
        return false;

    AssetManager::instance().init(renderer_.get());
    AudioManager::instance().init();

    /* Load assets (if files exist) */
    auto& am = AssetManager::instance();
    am.loadTexture("tiles",  "assets/sprites/tiles.png");
    am.loadTexture("player", "assets/sprites/player.png");
    am.loadTexture("enemies","assets/sprites/enemies.png");

    auto& au = AudioManager::instance();
    au.loadMusic("theme",   "assets/sounds/theme.ogg");
    au.loadMusic("underground","assets/sounds/underground.ogg");
    au.loadSound("jump",    "assets/sounds/jump.wav");
    au.loadSound("coin",    "assets/sounds/coin.wav");
    au.loadSound("stomp",   "assets/sounds/stomp.wav");
    au.loadSound("powerup", "assets/sounds/powerup.wav");
    au.loadSound("powerdown","assets/sounds/powerdown.wav");
    au.loadSound("break",   "assets/sounds/break.wav");
    au.loadSound("block",   "assets/sounds/block.wav");
    au.loadSound("die",     "assets/sounds/die.wav");
    au.loadSound("1up",     "assets/sounds/1up.wav");
    au.loadSound("fireball","assets/sounds/fireball.wav");
    au.loadSound("kick",    "assets/sounds/kick.wav");
    au.loadSound("flagpole","assets/sounds/flagpole.wav");

    hud_  = std::make_unique<HUD>();
    showMainMenu();

    running_  = true;
    lastTime_ = SDL_GetPerformanceCounter();
    return true;
}

void Game::showMainMenu()
{
    state_ = GameState::MainMenu;
    menu_  = std::make_unique<Menu>();
    menu_->setItems({
        {"START GAME", [this]{ startLevel(1); }},
        {"LEVEL 2",    [this]{ startLevel(2); }},
        {"LEVEL 3",    [this]{ startLevel(3); }},
        {"QUIT",       [this]{ running_ = false; }},
    });
    AudioManager::instance().stopMusic();
}
/*
std::make_unique<Level>(n) constructs a brand new Level and assigns it. The old Level is automatically 
destroyed via std::unique_ptr's destructor — all enemies, coins, particles, and player state are reset 
to initial values. No manual cleanup code needed.
*/
void Game::startLevel(int n)
{
    levelNum_ = n;
    state_    = GameState::Playing;
    level_    = std::make_unique<Level>(n);
    level_->player().setInput(&input_);
    menu_.reset();
    AudioManager::instance().playMusic(
        n == 2 ? "underground" : "theme");
}

void Game::showGameOver()
{
    state_ = GameState::GameOver;
    AudioManager::instance().stopMusic();
}

void Game::showLevelComplete()
{
    state_ = GameState::LevelComplete;
}
/*
SDL_GetPerformanceCounter() returns high-resolution timer ticks. Dividing by SDL_GetPerformanceFrequency() 
converts to seconds. Capping dt at 0.05f (20 FPS minimum) prevents physics explosions when the window is 
dragged, the game is tabbed out, or the frame rate drops severely.
*/
void Game::run()
{
    while (running_) {
        Uint64 now  = SDL_GetPerformanceCounter();
        /*
        The game uses a hybrid approach:
        This means:

The game loop runs at whatever frame rate the hardware achieves
Physics always uses 1/60 second as the timestep
At 60 FPS: physics runs once per frame
At 120 FPS: physics runs with half the dt, particles and animations are smoother
At 30 FPS: physics is slightly slower but still deterministic
        */
       /*
       A fully correct fixed-timestep implementation would use an accumulator:
       accumDt_ += dt;
while (accumDt_ >= FIXED_DT) {
    level_->update(FIXED_DT);
    accumDt_ -= FIXED_DT;
}


The current simplified version runs physics once per render frame using FIXED_DT — good enough at 60 FPS target 
but physics would slow down at lower frame rates.

       */
        float  dt   = (float)(now - lastTime_) /
                       SDL_GetPerformanceFrequency();
        lastTime_   = now;
        dt = std::min(dt, 0.05f);   /* cap at 50ms */

        handleEvents();
        update(dt);
        render();
    }
}

void Game::handleEvents()
{
    input_.update();
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) running_ = false;
        input_.handleEvent(e);
    }
}

void Game::update(float dt)
{
    switch (state_) {
        case GameState::MainMenu:     updateMenu    (dt); break;
        case GameState::Playing:      updatePlaying (dt); break;
        case GameState::Paused:       updatePaused  (dt); break;
        case GameState::GameOver:     updateGameOver(dt); break;
        case GameState::LevelComplete:updateComplete(dt); break;
        default: break;
    }
}

void Game::updateMenu(float dt)
{
    if (menu_) menu_->update(input_, dt);
}
/*
The game loop polls level outcome flags after each update. The pattern separates 
concerns — Level sets flags, Game decides what state transition happens next. Level doesn't know about GameState.
*/
/*
The Game class acts as a state machine controller. Level is purely a simulation — it sets flags 
(isComplete_, playerDead_) but has no concept of game states or transitions. Game reads those flags 
and decides what happens next. This separation means Level is fully testable in isolation.
*/
void Game::updatePlaying(float dt)
{
    if (!level_) return;

    /* Pause */
    if (input_.isPressed(SDL_SCANCODE_P) ||
        input_.isPressed(SDL_SCANCODE_ESCAPE) ||
        input_.back()) {
        state_ = GameState::Paused;
        AudioManager::instance().pauseMusic();
        return;
    }

    level_->update(dt);

    if (level_->isComplete())
        showLevelComplete();
    else if (level_->playerDied()) {
        if (level_->player().lives() <= 0)
            showGameOver();
        else {
            /* restart level */
            int n = levelNum_;
            startLevel(n);
        }
    }
}

void Game::updatePaused(float dt)
{
    (void)dt;
    if (input_.isPressed(SDL_SCANCODE_P) ||
        input_.isPressed(SDL_SCANCODE_SPACE)) {
        state_ = GameState::Playing;
        AudioManager::instance().resumeMusic();
    }
    if (input_.isPressed(SDL_SCANCODE_ESCAPE))
        showMainMenu();
}

void Game::updateGameOver(float dt)
{
    (void)dt;
    if (input_.isPressed(SDL_SCANCODE_SPACE) ||
        input_.isPressed(SDL_SCANCODE_RETURN))
        startLevel(levelNum_);
    if (input_.isPressed(SDL_SCANCODE_ESCAPE))
        showMainMenu();
}

void Game::updateComplete(float dt)
{
    (void)dt;
    if (input_.isPressed(SDL_SCANCODE_SPACE) ||
        input_.isPressed(SDL_SCANCODE_RETURN)) {
        int next = levelNum_ + 1;
        if (next > 3) showMainMenu();
        else          startLevel(next);
    }
}

void Game::render()
{
    renderer_.clear({107, 140, 255, 255});

    switch (state_) {
        case GameState::MainMenu:
            if (menu_) menu_->render(renderer_.get());
            break;

        case GameState::Playing:
            if (level_) {
                level_->render(renderer_.get());
                hud_->render(renderer_.get(),
                              level_->player(),
                              level_->timeLeft(),
                              levelNum_);
            }
            break;

        case GameState::Paused:
            if (level_) {
                level_->render(renderer_.get());
                hud_->render(renderer_.get(),
                              level_->player(),
                              level_->timeLeft(),
                              levelNum_);
            }
            hud_->renderPaused(renderer_.get());
            break;

        case GameState::GameOver:
            if (level_) level_->render(renderer_.get());
            hud_->renderGameOver(renderer_.get());
            break;

        case GameState::LevelComplete:
            if (level_) level_->render(renderer_.get());
            hud_->renderComplete(renderer_.get(),
                                  level_->player().score());
            break;

        default: break;
    }

    renderer_.endFrame();
}

void Game::shutdown()
{
    level_.reset();
    hud_.reset();
    menu_.reset();
    AssetManager::instance().shutdown();
    AudioManager::instance().shutdown();
}

} /* namespace Mario */
