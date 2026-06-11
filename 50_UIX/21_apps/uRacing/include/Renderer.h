#pragma once
//#include <SDL2/SDL.h>
#include <SDL.h>

namespace Mario {

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    bool init(const char* title, int w, int h);
    void beginFrame();
    void endFrame  ();
    void clear     (SDL_Color col);

    SDL_Renderer* get() const { return renderer_; }
    SDL_Window*   window()    const { return window_; }

private:
    SDL_Window*   window_   = nullptr;
    SDL_Renderer* renderer_ = nullptr;
};

} /* namespace Mario */
