#pragma once
#include "board.h"
#include "move.h"

enum GenType { ALL_MOVES, CAPTURES_ONLY };

struct MoveGen {
    // Generates pseudo-legal moves into list, returns count.
    // Legality (king not in check) is checked by the caller after make_move.
    static int generate(const Board& b, Move* list, GenType type = ALL_MOVES);

    // Convenience: parse a move string (e.g. "e2e4", "e7e8q") against a generated list
    static Move parse(const Board& b, const std::string& str);
};
