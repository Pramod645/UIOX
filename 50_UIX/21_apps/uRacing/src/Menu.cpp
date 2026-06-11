#include "Menu.h"
//#include <SDL2/SDL.h>
#include <SDL.h>
#include <cmath>

namespace Mario {

Menu::Menu()
{
    fontBig_ = TTF_OpenFont("assets/fonts/mario.ttf",    48);
    fontSml_ = TTF_OpenFont("assets/fonts/mario.ttf",    24);
    if (!fontBig_)
        fontBig_ = TTF_OpenFont("assets/fonts/DejaVuSans.ttf", 48);
    if (!fontSml_)
        fontSml_ = TTF_OpenFont("assets/fonts/DejaVuSans.ttf", 24);
}

Menu::~Menu()
{
    if (fontBig_) TTF_CloseFont(fontBig_);
    if (fontSml_) TTF_CloseFont(fontSml_);
}

void Menu::setItems(std::vector<MenuItem> items)
{
    items_    = std::move(items);
    selected_ = 0;
}
//blink_ toggles every 0.5 seconds. During render, if blink_ is false the selected item is skipped (not drawn) — creating a flashing "cursor" effect without any special sprite.
void Menu::update(const InputManager& inp, float dt)
{
    blinkTime_ += dt;
    if (blinkTime_ >= 0.5f) { blinkTime_ = 0; blink_ = !blink_; }
/*
Calls the lambda stored in the selected MenuItem::action. For "START GAME" this is [this]{ startLevel(1); } — a closure that captures the Game* and calls startLevel on it.
*/
    if (inp.isPressed(SDL_SCANCODE_DOWN) ||
        inp.isPressed(SDL_SCANCODE_S)) {
        selected_ = (selected_ + 1) % (int)items_.size();
    }
    if (inp.isPressed(SDL_SCANCODE_UP) ||
        inp.isPressed(SDL_SCANCODE_W)) {
        selected_ = ((selected_ - 1) + (int)items_.size())
                    % (int)items_.size();
    }
    if (inp.isPressed(SDL_SCANCODE_RETURN) ||
        inp.isPressed(SDL_SCANCODE_SPACE)) {
        if (!items_.empty()) items_[selected_].action();
    }
    if (inp.back()) {
        /* treat as select on Android */
        if (!items_.empty()) items_[selected_].action();
    }
}

void Menu::render(SDL_Renderer* r)
{
    /*
    Draws a gradient by rendering one horizontal line per row with increasing blue (100 at the top → 255 at the bottom). Relatively slow (720 draw calls) but visually rich and only done on the menu screen.
    */
    /* Gradient sky background */
    for (int y = 0; y < SCREEN_H; ++y) {
        Uint8 b = (Uint8)(100 + y * 155 / SCREEN_H);
        SDL_SetRenderDrawColor(r, 30, 60, b, 255);
        SDL_RenderDrawLine(r, 0, y, SCREEN_W, y);
    }

    /* Decorative clouds */
    SDL_SetRenderDrawColor(r, 255, 255, 255, 200);
    for (int i = 0; i < 5; ++i) {
        int cx = 80 + i * 250;
        int cy = 60 + (i%3)*40;
        SDL_Rect c1 = {cx, cy, 90, 35};
        SDL_Rect c2 = {cx-20, cy+15, 40, 20};
        SDL_Rect c3 = {cx+70, cy+15, 40, 20};
        SDL_RenderFillRect(r, &c1);
        SDL_RenderFillRect(r, &c2);
        SDL_RenderFillRect(r, &c3);
    }

    /* Ground at bottom */
    SDL_SetRenderDrawColor(r, 140, 80, 20, 255);
    SDL_Rect ground = {0, SCREEN_H-80, SCREEN_W, 80};
    SDL_RenderFillRect(r, &ground);
    SDL_SetRenderDrawColor(r, 60, 160, 60, 255);
    SDL_Rect grass = {0, SCREEN_H-80, SCREEN_W, 10};
    SDL_RenderFillRect(r, &grass);

    /* Title block pattern */
    SDL_Color titleCol = {255, 210, 0, 255};
    auto drawText = [&](TTF_Font* f, const std::string& txt,
                        int x, int y, SDL_Color col)
    {
        if (!f) return;
        SDL_Surface* s = TTF_RenderText_Blended(f, txt.c_str(), col);
        if (!s) return;
        SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
        SDL_Rect dst = {x, y, s->w, s->h};
        SDL_RenderCopy(r, t, nullptr, &dst);
        SDL_DestroyTexture(t);
        SDL_FreeSurface(s);
    };

    auto drawCentered = [&](TTF_Font* f, const std::string& txt,
                             int y, SDL_Color col)
    {
        if (!f) return;
        int w=0,h=0; TTF_SizeText(f, txt.c_str(), &w, &h);
        drawText(f, txt, (SCREEN_W-w)/2, y, col);
    };

    /* Game title */
    drawCentered(fontBig_, "SUPER MARIO",  120, titleCol);
    drawCentered(fontBig_, "ADVENTURE",    170, titleCol);
    drawCentered(fontSml_, "Powered by SDL2 + C++17",
                 230, {200, 200, 200, 255});

    /* Mario decorative sprite */
    SDL_SetRenderDrawColor(r, 220, 60, 40, 255);
    SDL_Rect mario = {SCREEN_W/2-20, 260, 40, 50};
    SDL_RenderFillRect(r, &mario);
    SDL_SetRenderDrawColor(r, 40, 80, 200, 255);
    SDL_Rect over = {SCREEN_W/2-20, 285, 40, 25};
    SDL_RenderFillRect(r, &over);
    SDL_SetRenderDrawColor(r, 220, 60, 40, 255);
    SDL_Rect hat = {SCREEN_W/2-24, 252, 48, 12};
    SDL_RenderFillRect(r, &hat);

    /* Menu items */
    int startY = 340;
    for (int i = 0; i < (int)items_.size(); ++i) {
        bool sel     = (i == selected_);
        SDL_Color col= sel
                       ? SDL_Color{255, 220, 0, 255}
                       : SDL_Color{255, 255, 255, 255};
        if (sel && !blink_) continue;

        std::string label = (sel ? "▶  " : "   ") + items_[i].label;
        drawCentered(fontSml_, label, startY + i * 48, col);
    }

    /* Controls hint */
    drawCentered(fontSml_, "↑↓ Navigate   SPACE Select",
                 SCREEN_H - 60, {160,160,160,255});

    /* Touch hint on Android */
#ifdef __ANDROID__
    drawCentered(fontSml_, "Tap to select",
                 SCREEN_H - 30, {160,160,160,255});
#endif
}

} /* namespace Mario */
