#pragma once
//#include <SDL2/SDL_mixer.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <unordered_map>
#include <string>

namespace Mario {
/*
Mix_Music* — streaming music (.ogg, .mp3) decoded on-the-fly — used for background tracks because they are too large to load fully into RAM
Mix_Chunk* — short sounds (.wav) loaded completely into RAM — used for instant sound effects like jumps and coins
*/
class AudioManager {
public:
    static AudioManager& instance();

    void init();
    void shutdown();

    void loadMusic (const std::string& key, const std::string& path);
    void loadSound (const std::string& key, const std::string& path);

    void playMusic (const std::string& key, int loops = -1);
    void stopMusic ();
    void pauseMusic();
    void resumeMusic();

    void playSound (const std::string& key, float vol = 1.0f);

    void setMusicVolume(float v);
    void setSoundVolume(float v);

    bool musicEnabled = true;
    bool soundEnabled = true;

private:
    AudioManager() = default;
    std::unordered_map<std::string, Mix_Music*>  music_;
    std::unordered_map<std::string, Mix_Chunk*>  sounds_;
};

} /* namespace Mario */
