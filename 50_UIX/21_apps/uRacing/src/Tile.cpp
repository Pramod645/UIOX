#include "Tile.h"

namespace Mario {
//The constructor sets solid and deadly automatically based on type — so callers just write Tile(TileType::Brick) and get the correct physics properties without manually setting flags.
Tile::Tile(TileType t) : type(t)
{
    switch (t) {
        case TileType::Ground://texIdx maps each tile type to a row/column in the sprite sheet. The renderer uses this index to select the correct source rectangle when drawing.
        case TileType::Brick://texIdx maps each tile type to a row/column in the sprite sheet. The renderer uses this index to select the correct source rectangle when drawing.
        case TileType::Pipe:
        case TileType::PipeTop:
            solid = true;  break;
        case TileType::Lava:
            deadly = true; break;
        default: break;
    }

    switch (t) {
        case TileType::Ground:        texIdx = 0;  break;
        case TileType::Brick:         texIdx = 1;  break;
        case TileType::QuestionBlock: texIdx = 2;  break;
        case TileType::HitBlock:      texIdx = 3;  break;
        case TileType::Pipe:          texIdx = 4;  break;
        case TileType::PipeTop:       texIdx = 5;  break;
        case TileType::Lava:          texIdx = 6;  break;
        case TileType::Flag:          texIdx = 7;  break;
        default:                      texIdx = -1; break;
    }
}

} /* namespace Mario */
