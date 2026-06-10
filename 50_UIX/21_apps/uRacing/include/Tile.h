#pragma once
#include "Utils.h"

namespace Mario {
//Every cell in the level grid is one of these types. Using an enum (instead of raw integers) makes level data self-documenting and prevents invalid values.
enum class TileType {
    Empty    = 0,
    Ground,        /* solid ground block             */
    Brick,         /* breakable by big player        */
    QuestionBlock, /* contains coin/power-up         */
    HitBlock,      /* question block after hit       */
    Pipe,          /* pipe (impassable)              */
    PipeTop,
    CoinTile,      /* decorative coin in background  */
    Lava,          /* kills player                   */
    Flag,          /* level end flag                 */
    Decoration,    /* visual only, no collision      */
};
/*
solid — does this tile block movement?
deadly — does touching it kill the player (lava)?
texIdx — which sprite in the sprite sheet to draw
hitCount — how many times a Question Block has been hit (controls what it gives)
visible — allows hiding tiles without removing them (animation trick)
*/
struct Tile {
    TileType type   = TileType::Empty;
    bool     solid  = false;
    bool     deadly = false;
    int      texIdx = 0;      /* index into sprite sheet       */
    int      hitCount = 0;    /* for question block hits       */
    bool     visible = true;

    Tile() = default;
    explicit Tile(TileType t);

    Rect worldRect(int col, int row) const { //Computes the world-space Rect of a tile from its grid coordinates. Multiplying by TILE_SIZE converts grid index → pixel position.
        return { (float)(col * TILE_SIZE),
                 (float)(row * TILE_SIZE),
                 (float)TILE_SIZE,
                 (float)TILE_SIZE };
    }
};

} /* namespace Mario */
