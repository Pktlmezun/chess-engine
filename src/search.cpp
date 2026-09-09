#include "search.h"
#include "eval.h"
#include "movegen.h"
#include <iostream>
#include <algorithm>

constexpr int MATE_SCORE    = SCORE_MATE;           // 999000
constexpr int MATE_THRESHOLD = SCORE_MATE / 2;      // 499500

uint64_t Search::nodes = 0;
Move     Search::pv_table[64][64];
int      Search::pv_len[64];

void Search::update_pv(int ply, Move m) {
    pv_table[ply][0] = m;
    for (int j = 0; j < pv_len[ply + 1]; ++j)
        pv_table[ply][j + 1] = pv_table[ply + 1][j];
    pv_len[ply] = pv_len[ply + 1] + 1;
}

int Search::quiescence(Board& b, int alpha, int beta, int ply) {
    nodes++;

    int stand_pat = evaluate(b);

    if (stand_pat >= beta)
        return beta;

    if (stand_pat + 900 < alpha)
        return alpha;

    if (stand_pat > alpha)
        alpha = stand_pat;

    Move list[256];
    int count = MoveGen::generate(b, list, CAPTURES_ONLY);

    for (int i = 0; i < count; ++i) {
        b.make_move(list[i]);

        if (b.is_attacked(b.king_square(~b.side_to_move), b.side_to_move)) {
            b.unmake_move(list[i]);
            continue;
        }

        int score = -quiescence(b, -beta, -alpha, ply + 1);

        b.unmake_move(list[i]);

        if (score >= beta)
            return beta;
        if (score > alpha)
            alpha = score;
    }

    return alpha;
}

int Search::negamax(Board& b, int depth, int alpha, int beta, int ply, SearchInfo& info) {
    nodes++;

    if (depth == 0)
        return quiescence(b, alpha, beta, ply);

    Move list[256];
    int count = MoveGen::generate(b, list);

    Move best_move = Move::null();
    int best_score = -SCORE_INF;
    int old_alpha = alpha;

    for (int i = 0; i < count; ++i) {
        b.make_move(list[i]);

        // Check if the side that just moved left their king in check
        if (b.is_attacked(b.king_square(~b.side_to_move), b.side_to_move)) {
            b.unmake_move(list[i]);
            continue;
        }

        int score = -negamax(b, depth - 1, -beta, -alpha, ply + 1, info);

        b.unmake_move(list[i]);

        if (score >= beta)
            return beta;

        if (score > alpha) {
            alpha = score;
            best_move = list[i];
            update_pv(ply, list[i]);
        }
    }

    if (best_move.is_null()) {
        // No legal moves — check if it's checkmate or stalemate
        // We need to check if the side to move is in check (for mate detection)
        // Since no legal moves were found, check if the king is attacked by opponent
        if (b.is_attacked(b.king_square(b.side_to_move), ~b.side_to_move))
            return -MATE_SCORE + ply;
        else
            return 0;
    }

    if (ply == 0) {
        info.best_move = best_move;
        info.pv_len = pv_len[0];
        for (int i = 0; i < pv_len[0]; ++i)
            info.pv[i] = pv_table[0][i];
    }

    return alpha;
}

void Search::go(Board& b, int max_depth) {
    nodes = 0;
    SearchInfo info{};

    for (int d = 1; d <= max_depth; ++d) {
        info.depth = d;
        int score = negamax(b, d, -SCORE_INF, SCORE_INF, 0, info);

        std::cout << "info depth " << d
                  << " score cp " << score
                  << " nodes " << nodes
                  << " pv ";
        for (int i = 0; i < info.pv_len; ++i)
            std::cout << info.pv[i].to_string() << " ";
        std::cout << "\n";

        if (score > MATE_THRESHOLD || score < -MATE_THRESHOLD)
            break;
    }

    std::cout << "bestmove " << info.pv[0].to_string() << "\n";
}
