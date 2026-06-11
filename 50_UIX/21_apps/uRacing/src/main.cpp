#include "Game.h"
//#include <SDL2/SDL.h>
#include <SDL.h>
#include <iostream>

/*
On Android, SDL2 requires the entry point to be named SDL_main (declared extern "C" to disable C++ name mangling) 
because the Java SDLActivity calls it via JNI. On every other platform the standard main is used.
*/
/*
On Android the process flow is:
Java SDLActivity.onCreate()
  └── System.loadLibrary("MarioGame")   loads .so
        └── JNI calls SDL_main()        our entry point
              └── Game::run()           blocks here


extern "C" disables C++ name mangling so the JNI linker can find SDL_main by its exact name without 
the compiler-generated decorations like _ZN5Mario4mainEv.              
*/
#ifdef __ANDROID__
extern "C" int SDL_main(int argc, char* argv[])
#else
int main(int argc, char* argv[])
#endif
{
    (void)argc; (void)argv;
/*
The entire game lifecycle in four lines: construct → initialise → run (blocks until quit) → shutdown. 
Returning 1 on init failure signals to the OS that the process exited with an error — important for 
shell scripts and Android crash reporting.
*/    
    Mario::Game game;

    if (!game.init()) {
        std::cerr << "Failed to initialise game: "
                  << SDL_GetError() << "\n";
        return 1;
    }

    game.run();
    game.shutdown();
    return 0;
}
