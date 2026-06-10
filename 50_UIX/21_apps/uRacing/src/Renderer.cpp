#include "Renderer.h"
#include "Utils.h"
#include <SDL2/SDL_image.h>

namespace Mario {

Renderer::~Renderer()
{
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_)   SDL_DestroyWindow(window_);
    SDL_Quit();
}

bool Renderer::init(const char* title, int w, int h)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO |
                 SDL_INIT_GAMECONTROLLER) < 0)
        return false;
/*
On desktop (macOS/Windows) the window is resizable — SDL_RenderSetLogicalSize handles scaling. 
On Android the window must be fullscreen because there is no window manager; the entire screen 
is always the game surface.
*/
    Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
#ifdef __ANDROID__
    flags = SDL_WINDOW_FULLSCREEN;
#endif

    window_ = SDL_CreateWindow(title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        w, h, flags);
    if (!window_) return false;

    renderer_ = SDL_CreateRenderer(window_, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) return false;

    /* logical size so the game always renders at 1280×720 */
    /*
    This is a critical line — it tells SDL2 to always render as if the window is 1280×720, scaling 
    automatically to whatever the actual window or screen size is. On a 4K monitor or an Android phone with 
    a different aspect ratio, the game scales correctly with letterboxing if needed.
    */
   /*
   This single call enables resolution-independent rendering. The internal render target is always 1280×720. SDL2 automatically scales it to fit:

A 2560×1440 monitor → 2× scale, pixel-perfect
An Android phone at 1080×2400 → letterboxed to 1280×720 with black bars
A 1920×1080 monitor → scaled with black bars on top and bottom
All game coordinates, collision boxes, and touch zones are in 1280×720 space — no platform-specific coordinate transformation is needed anywhere in game code.
   */
    SDL_RenderSetLogicalSize(renderer_, SCREEN_W, SCREEN_H);
    /*
    On Android (and some desktop touchscreens), SDL2 can synthesise mouse events from touch events. Setting this hint ensures single-finger touch behaves like a mouse click — used for menu item selection.
    */
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    /* enable touch events */
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "1");

    return true;
}

void Renderer::beginFrame() {}

void Renderer::endFrame()
{
    SDL_RenderPresent(renderer_);
}

void Renderer::clear(SDL_Color col)
{
    SDL_SetRenderDrawColor(renderer_, col.r, col.g, col.b, col.a);
    SDL_RenderClear(renderer_);
}

} /* namespace Mario */
