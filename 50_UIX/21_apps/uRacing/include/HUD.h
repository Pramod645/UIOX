#pragma once
#include "Player.h"
//#include <SDL2/SDL.h>
#include <SDL.h>
//#include <SDL2/SDL_ttf.h>
#include <SDL_ttf.h>

namespace Mario {

class HUD {
public:
    HUD();
    ~HUD();

    void render(SDL_Renderer* r,
                const Player& player,
                float timeLeft,
                int   levelNum);
    void renderGameOver (SDL_Renderer* r);
    void renderComplete (SDL_Renderer* r, int score);
    void renderPaused   (SDL_Renderer* r);

private:
/*
Two font sizes for the HUD. TTF_Font* is a pointer to an SDL_ttf font loaded from a .ttf file. Stored as raw pointers because SDL_ttf manages their lifetime — we call TTF_CloseFont in the destructor.
*/
    TTF_Font* fontLarge_ = nullptr;
    TTF_Font* fontSmall_ = nullptr;

    void drawText(SDL_Renderer* r, TTF_Font* f,
                  const std::string& text,
                  int x, int y, SDL_Color col);
    void drawCentered(SDL_Renderer* r, TTF_Font* f,
                      const std::string& text,
                      int y, SDL_Color col);
    void drawLife(SDL_Renderer* r, int lives, int x, int y);
    void drawPowerIcon(SDL_Renderer* r, PowerLevel p, int x, int y);
};

} /* namespace Mario */
