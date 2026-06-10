#pragma once

namespace Mario {
/*
enum class (scoped enum) prevents name collisions — GameState::Playing won't accidentally 
match an integer 1 the way a plain enum could. Each value is a distinct named state that 
the game's main loop switches between.
*/
enum class GameState {
    MainMenu,
    Playing,
    Paused,
    GameOver,
    LevelComplete,
    Credits
};

} /* namespace Mario */
