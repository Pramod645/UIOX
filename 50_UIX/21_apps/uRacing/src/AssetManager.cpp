#include "AssetManager.h"
#include <stdexcept>

namespace Mario {

AssetManager& AssetManager::instance()
{
    static AssetManager inst;
    return inst;
}

void AssetManager::init(SDL_Renderer* r)
{
    renderer_ = r;
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
    TTF_Init();
    createPlaceholderTextures();
}

void AssetManager::shutdown()
{
    for (auto& [k,t] : textures_) SDL_DestroyTexture(t);
    for (auto& [k,f] : fonts_)    TTF_CloseFont(f);
    textures_.clear();
    fonts_.clear();
    TTF_Quit();
    IMG_Quit();
}

SDL_Texture* AssetManager::texture(const std::string& key)
{
    auto it = textures_.find(key);
    return (it != textures_.end()) ? it->second : nullptr;
}

TTF_Font* AssetManager::font(const std::string& key)
{
    auto it = fonts_.find(key);
    return (it != fonts_.end()) ? it->second : nullptr;
}

void AssetManager::loadTexture(const std::string& key,
                                 const std::string& path)
{
    SDL_Texture* t = IMG_LoadTexture(renderer_, path.c_str());
    if (t) textures_[key] = t;
}

void AssetManager::loadFont(const std::string& key,
                              const std::string& path, int pt)
{
    TTF_Font* f = TTF_OpenFont(path.c_str(), pt);
    if (f) fonts_[key] = f;
}
/*
Step by step:

SDL_CreateTexture — allocates GPU memory for a w×h texture with STATIC access (uploaded once, read many times)
SDL_PIXELFORMAT_RGBA8888 — 4 bytes per pixel: Red, Green, Blue, Alpha in that order
std::vector<Uint32> — fills CPU memory with w*h identical pixels
SDL_MapRGBA — packs R,G,B,A bytes into a single 32-bit integer in the correct format
SDL_UpdateTexture — uploads the CPU pixel buffer to GPU memory; w * 4 = row pitch (bytes per row = width × 4 bytes per pixel)
*/
SDL_Texture* AssetManager::createColorTexture(int w, int h, SDL_Color c)
{
    SDL_Texture* t = SDL_CreateTexture(renderer_,
        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, w, h);
    std::vector<Uint32> pixels(w * h,
        SDL_MapRGBA(SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888),
                    c.r, c.g, c.b, c.a));
    SDL_UpdateTexture(t, nullptr, pixels.data(), w * 4);
    return t;
}

void AssetManager::createPlaceholderTextures()
{
    /* These are solid-colour fallbacks used when real sprites
       are not present. Replace with actual sprite sheets. */
    textures_["player"] = createColorTexture(32,32,{220,60,40,255});
    textures_["goomba"] = createColorTexture(32,32,{140,80,20,255});
    textures_["koopa"]  = createColorTexture(32,48,{40,160,40,255});
    textures_["coin"]   = createColorTexture(20,20,{255,210,0,255});
    textures_["ground"] = createColorTexture(32,32,{140,80,20,255});
    textures_["brick"]  = createColorTexture(32,32,{180,80,20,255});
    textures_["qblock"] = createColorTexture(32,32,{255,180,0,255});
}

} /* namespace Mario */
