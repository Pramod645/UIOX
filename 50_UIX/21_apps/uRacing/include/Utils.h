#pragma once
//#include <SDL2/SDL.h>
#include <SDL.h>
#include <cmath>
#include <string>
#include <algorithm>

namespace Mario {

/* ── Constants ──────────────────────────────────────────────── */
static constexpr int   SCREEN_W      = 1280;
static constexpr int   SCREEN_H      = 720;
static constexpr int   TILE_SIZE     = 32; //Every tile in the world is a 32×32 pixel square. Changing this one constant rescales the entire world.
static constexpr float GRAVITY       = 1800.0f;   /* px/s²  */ //Downward acceleration in pixels per second squared. A higher value makes the game feel heavier and more arcade-like.
static constexpr float JUMP_FORCE    = -750.0f;   /* px/s   */ //Negative because the SDL Y axis points downward — a negative velocity moves the player upward.
static constexpr float PLAYER_SPEED  = 220.0f;    /* px/s   */ //Two speed tiers: walking and running (when the Shift / Z key is held).
static constexpr float PLAYER_RUN    = 360.0f; //Two speed tiers: walking and running (when the Shift / Z key is held).
//FIXED_DT (≈ 0.01667 seconds) is the physics timestep. Using a fixed step makes physics deterministic — it gives the same result regardless of the real frame rate.
static constexpr int   FPS_TARGET    = 60;
static constexpr float FIXED_DT      = 1.0f / FPS_TARGET;

/* ── Colours ─────────────────────────────────────────────────── */
static constexpr SDL_Color COL_SKY    = {107,140,255,255}; //FIXED_DT (≈ 0.01667 seconds) is the physics timestep. Using a fixed step makes physics deterministic — it gives the same result regardless of the real frame rate.
static constexpr SDL_Color COL_WHITE  = {255,255,255,255};
static constexpr SDL_Color COL_BLACK  = {0,0,0,255};
static constexpr SDL_Color COL_RED    = {220,50,50,255};
static constexpr SDL_Color COL_YELLOW = {255,210,0,255};

/* ── Vec2 ────────────────────────────────────────────────────── */
struct Vec2 { //A 2D vector. = 0 default-initialises both fields so an uninitialised Vec2 is always {0,0} — prevents random garbage values.
    float x = 0, y = 0;
    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}
    //Operator overloads so you can write vel + acc * dt instead of Vec2{vel.x + acc.x*dt, vel.y + acc.y*dt}.
    Vec2 operator+(const Vec2& o) const { return {x+o.x, y+o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x-o.x, y-o.y}; }
    Vec2 operator*(float s)       const { return {x*s,   y*s};   }
    //In-place addition — modifies the vector and returns a reference to itself so you can chain: pos += vel += acc.
    Vec2& operator+=(const Vec2& o) { x+=o.x; y+=o.y; return *this; }
    Vec2& operator*=(float s)       { x*=s;   y*=s;   return *this; }
    float length() const { return std::sqrt(x*x + y*y); }//Euclidean length (magnitude) of the vector — used for normalising directions.
};

/* ── AABB ────────────────────────────────────────────────────── */
struct Rect { //Axis-aligned bounding box in world space with floating-point coordinates (unlike SDL_Rect which is integer-only). Stores position (x,y) and size (w,h).
    float x=0, y=0, w=0, h=0;
    bool overlaps(const Rect& o) const { //Separating Axis Theorem (SAT) for AABBs — the two rectangles overlap if and only if they are NOT separated on either axis. Returns true when they intersect.
        return x < o.x+o.w && x+w > o.x &&
               y < o.y+o.h && y+h > o.y;
    }
    SDL_Rect toSDL() const { //Converts our float Rect to SDL2's integer SDL_Rect needed for drawing calls, truncating sub-pixel positions.
        return {(int)x, (int)y, (int)w, (int)h};
    }
    //Convenience getters — avoids repeating bounds.x + bounds.w everywhere and prevents arithmetic mistakes.
    float right () const { return x + w; }
    float bottom() const { return y + h; }
    float centerX()const { return x + w/2; }
    float centerY()const { return y + h/2; }
};

/* ── Colour helper ───────────────────────────────────────────── */
inline void setColor(SDL_Renderer* r, SDL_Color c) { //A thin wrapper so we can pass an SDL_Color struct directly instead of unpacking its four fields every time.
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
}

} /* namespace Mario */
