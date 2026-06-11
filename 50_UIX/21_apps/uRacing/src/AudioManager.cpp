#include "AudioManager.h"
//#include <SDL2/SDL.h>
#include <SDL.h>

namespace Mario {

AudioManager& AudioManager::instance()
{
    static AudioManager inst;
    return inst;
}

void AudioManager::init()
{
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
    //Mix_
    Mix_AllocateChannels(16);
}

void AudioManager::shutdown()
{
    for (auto& [k,m] : music_)  Mix_FreeMusic(m);
    for (auto& [k,c] : sounds_) Mix_FreeChunk(c);
    music_.clear();
    sounds_.clear();
    Mix_CloseAudio();
}
/*
if (c) silently skips missing sound files — the game continues without audio rather than crashing. 
This is deliberate: the project ships without audio assets, and the game must still run.
*/
void AudioManager::loadMusic(const std::string& key,
                               const std::string& path)
{
    Mix_Music* m = Mix_LoadMUS(path.c_str());
    if (m) music_[key] = m;
}

void AudioManager::loadSound(const std::string& key,
                               const std::string& path)
{
    Mix_Chunk* c = Mix_LoadWAV(path.c_str());
    if (c) sounds_[key] = c;
}
/*
Mix_PlayChannel(-1, ...) — -1 asks SDL_mixer to find a free channel automatically
0 loops argument — play once only
Mix_Volume sets the volume on the specific channel that was just allocated, not globally
Returns early silently if the sound key doesn't exist in the map
*/
void AudioManager::playMusic(const std::string& key, int loops)
{
    if (!musicEnabled) return;
    auto it = music_.find(key);
    if (it != music_.end()) Mix_PlayMusic(it->second, loops);
}

void AudioManager::stopMusic()   { Mix_HaltMusic();  }
void AudioManager::pauseMusic()  { Mix_PauseMusic(); }
void AudioManager::resumeMusic() { Mix_ResumeMusic();}

void AudioManager::playSound(const std::string& key, float vol)
{
    if (!soundEnabled) return;
    auto it = sounds_.find(key);
    if (it == sounds_.end()) return;
    int ch = Mix_PlayChannel(-1, it->second, 0);
    if (ch >= 0)
        Mix_Volume(ch, (int)(vol * MIX_MAX_VOLUME));
}

void AudioManager::setMusicVolume(float v)
{
    Mix_VolumeMusic((int)(v * MIX_MAX_VOLUME));
}

void AudioManager::setSoundVolume(float v)
{
    Mix_Volume(-1, (int)(v * MIX_MAX_VOLUME));
}

} /* namespace Mario */
