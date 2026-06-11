#pragma once
//#include <SDL2/SDL.h>
//#include <SDL2/SDL_image.h>
//#include <SDL2/SDL_ttf.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <unordered_map>
#include <string>

namespace Mario {
/*
Singleton pattern — instance() returns a reference to the single global AssetManager. The static local variable inside instance() is created once and lives for the program's lifetime. Thread-safe in C++11 and later.
*/
class AssetManager {
public:
    static AssetManager& instance();

    void init(SDL_Renderer* r);
    void shutdown();

    SDL_Texture* texture(const std::string& key);
    TTF_Font*    font   (const std::string& key);
    void         loadTexture(const std::string& key,
                              const std::string& path);
    void         loadFont   (const std::string& key,
                              const std::string& path, int pt);

    /* generated placeholder textures (no file needed) */
    void createPlaceholderTextures();//Generates solid-colour textures programmatically when real image files are absent. This allows the game to run and be tested without any assets — the placeholder colours convey enough shape to understand what each object is.

private:
    AssetManager() = default;
    SDL_Renderer* renderer_ = nullptr;
    std::unordered_map<std::string, SDL_Texture*> textures_;
    std::unordered_map<std::string, TTF_Font*>    fonts_;

    SDL_Texture* createColorTexture(int w, int h, SDL_Color c);
    SDL_Texture* createCheckerboard(int w, int h,
                                     SDL_Color a, SDL_Color b,
                                     int cellSize);
};

} /* namespace Mario */
