#include "HUD.h"
#include <string>
#include <sstream>
#include <iomanip>
#include <SDL2/SDL.h>

namespace Mario {

HUD::HUD()
{
    /* Try to load a font; fall back to SDL_RenderDrawRect text if absent */
    fontLarge_ = TTF_OpenFont("assets/fonts/mario.ttf", 24);
    fontSmall_ = TTF_OpenFont("assets/fonts/mario.ttf", 16);
    if (!fontLarge_) fontLarge_ = TTF_OpenFont(
        "assets/fonts/DejaVuSans.ttf", 24);
    if (!fontSmall_) fontSmall_ = TTF_OpenFont(
        "assets/fonts/DejaVuSans.ttf", 16);
}

HUD::~HUD()
{
    if (fontLarge_) TTF_CloseFont(fontLarge_);
    if (fontSmall_) TTF_CloseFont(fontSmall_);
}
/*
SDL_ttf workflow: render text to a SDL_Surface (CPU memory) → upload to GPU as 
SDL_Texture → draw the texture → free both. Creating a new texture every frame for 
text is slow for many labels but simple and correct for a HUD with few labels.
*/
void HUD::drawText(SDL_Renderer* r, TTF_Font* f,
                    const std::string& text,
                    int x, int y, SDL_Color col)
{
    if (!f) {
        /* fallback: draw coloured rectangles as pixel "font" */
        SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
        SDL_Rect box = {x, y, (int)text.size()*10, 16};
        SDL_RenderFillRect(r, &box);
        return;
    }
    SDL_Surface* surf = TTF_RenderText_Blended(f, text.c_str(), col);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
    SDL_Rect dst = {x, y, surf->w, surf->h};
    SDL_RenderCopy(r, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

void HUD::drawCentered(SDL_Renderer* r, TTF_Font* f,
                         const std::string& text,
                         int y, SDL_Color col)
{
    if (!f) { drawText(r, f, text, SCREEN_W/2-80, y, col); return; }
    int w = 0, h = 0;
    TTF_SizeText(f, text.c_str(), &w, &h);
    drawText(r, f, text, (SCREEN_W - w) / 2, y, col);
}

void HUD::drawLife(SDL_Renderer* r, int lives, int x, int y)
{
    /* small red rectangle per life */
    for (int i = 0; i < lives; ++i) {
        SDL_SetRenderDrawColor(r, 220, 60, 40, 255);
        SDL_Rect rect = {x + i * 22, y, 18, 18};
        SDL_RenderFillRect(r, &rect);
        SDL_SetRenderDrawColor(r, 255, 200, 0, 255);
        SDL_Rect hat = {x + i*22 + 2, y-4, 14, 5};
        SDL_RenderFillRect(r, &hat);
    }
}

void HUD::drawPowerIcon(SDL_Renderer* r, PowerLevel p, int x, int y)
{
    SDL_Rect icon = {x, y, 20, 20};
    switch (p) {
        case PowerLevel::Small:
            SDL_SetRenderDrawColor(r, 150, 150, 150, 255);
            break;
        case PowerLevel::Big:
            SDL_SetRenderDrawColor(r, 220, 60, 40, 255);
            break;
        case PowerLevel::Fire:
            SDL_SetRenderDrawColor(r, 255, 140, 0, 255);
            break;
    }
    SDL_RenderFillRect(r, &icon);
}

void HUD::render(SDL_Renderer* r, const Player& player,
                  float timeLeft, int levelNum)
{
    /* semi-transparent HUD bar */
    /*
    The HUD background is drawn with alpha 140 (≈55% opacity) using blend mode. 
    BLENDMODE_BLEND makes the drawn colour blend with whatever is behind it. 
    Resetting to BLENDMODE_NONE afterward ensures subsequent opaque drawing calls aren't accidentally blended.
    */
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 140);
    SDL_Rect bar = {0, 0, SCREEN_W, 44};
    SDL_RenderFillRect(r, &bar);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color yellow= {255, 220, 0,   255};
    SDL_Color red   = {255, 80,  80,  255};

    /* Score */
    drawText(r, fontSmall_, "SCORE", 12, 6, white);
    std::string scoreStr = std::to_string(player.score());
    while (scoreStr.size() < 6) scoreStr = "0" + scoreStr;
    drawText(r, fontLarge_, scoreStr, 12, 22, yellow);

    /* Coins */
    SDL_SetRenderDrawColor(r, 255, 210, 0, 255);
    SDL_Rect coinIcon = {180, 10, 14, 14};
    SDL_RenderFillRect(r, &coinIcon);
    drawText(r, fontSmall_, "x" + std::to_string(player.coins()),
             198, 10, white);

    /* Level */
    drawText(r, fontSmall_,
             "WORLD " + std::to_string(levelNum) + "-1",
             SCREEN_W/2 - 60, 6, white);

    /* Timer */
    drawText(r, fontSmall_, "TIME", SCREEN_W-120, 6, white);
    int t = (int)timeLeft;
    SDL_Color timeCol = (t < 60) ? red : white;
    drawText(r, fontLarge_, std::to_string(t),
             SCREEN_W-110, 22, timeCol);

    /* Lives */
    drawText(r, fontSmall_, "LIVES", 300, 6, white);
    drawLife(r, player.lives(), 300, 22);

    /* Power level icon */
    drawText(r, fontSmall_, "PWR", 440, 6, white);
    drawPowerIcon(r, player.powerLevel(), 440, 22);
}

void HUD::renderGameOver(SDL_Renderer* r)
{
    /* dark overlay */
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 180);
    SDL_Rect full = {0, 0, SCREEN_W, SCREEN_H};
    SDL_RenderFillRect(r, &full);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    SDL_Color red   = {220, 50, 50, 255};
    SDL_Color white = {255,255,255,255};
    SDL_Color grey  = {180,180,180,255};

    drawCentered(r, fontLarge_, "GAME OVER",
                 SCREEN_H/2 - 60, red);
    drawCentered(r, fontSmall_, "Press SPACE to retry",
                 SCREEN_H/2, white);
    drawCentered(r, fontSmall_, "Press ESC for menu",
                 SCREEN_H/2 + 30, grey);
}

void HUD::renderComplete(SDL_Renderer* r, int score)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 160);
    SDL_Rect full = {0, 0, SCREEN_W, SCREEN_H};
    SDL_RenderFillRect(r, &full);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    SDL_Color gold  = {255, 210, 0,   255};
    SDL_Color white = {255, 255, 255, 255};

    drawCentered(r, fontLarge_, "LEVEL COMPLETE!", SCREEN_H/2-60, gold);
    drawCentered(r, fontSmall_,
                 "Score: " + std::to_string(score),
                 SCREEN_H/2, white);
    drawCentered(r, fontSmall_, "Press SPACE for next level",
                 SCREEN_H/2 + 36, white);
}

void HUD::renderPaused(SDL_Renderer* r)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 160);
    SDL_Rect full = {0, 0, SCREEN_W, SCREEN_H};
    SDL_RenderFillRect(r, &full);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    SDL_Color white = {255,255,255,255};
    SDL_Color grey  = {180,180,180,255};

    drawCentered(r, fontLarge_, "PAUSED", SCREEN_H/2 - 50, white);
    drawCentered(r, fontSmall_, "Press P to resume", SCREEN_H/2, grey);
    drawCentered(r, fontSmall_, "Press ESC for menu",
                 SCREEN_H/2 + 30, grey);
}

} /* namespace Mario */
