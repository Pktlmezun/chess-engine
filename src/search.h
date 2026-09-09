#pragma once
#include "board.h"
#include "move.h"

struct SearchInfo {
    int depth;
    int score;
    uint64_t nodes;
    Move best_move;
    Move pv[64];
    int pv_len;
};

class Search {
public:
    static void go(Board& b, int max_depth = 64);

private:
    static uint64_t nodes;
    static Move pv_table[64][64];
    static int pv_len[64];

    static int negamax(Board& b, int depth, int alpha, int beta, int ply, SearchInfo& info);
    static int quiescence(Board& b, int alpha, int beta, int ply);
    static void update_pv(int ply, Move m);
};
