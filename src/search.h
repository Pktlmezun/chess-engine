#pragma once
#include "board.h"
#include "move.h"
#include <atomic>
#include <chrono>
#include <mutex>

struct ScoredMove {
    Move move;
    int  score;
};

struct SearchLimits {
    int depth = 64;
    int64_t wtime = 0;
    int64_t btime = 0;
    int64_t winc  = 0;
    int64_t binc  = 0;
    int64_t movetime = 0;
    bool infinite = false;
};

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
    static std::atomic<bool> stopped;
    static std::mutex output_mutex;

    static void go(Board& b, const SearchLimits& limits = SearchLimits{});
    static void stop();
    static void clear_tables();

private:
    static constexpr int MAX_PLY = 128;

    static uint64_t nodes;
    static Move pv_table[64][64];
    static int pv_len[64];
    static SearchLimits limits;
    static std::chrono::steady_clock::time_point search_start;
    static int64_t alloc_time_ms(Color side, int game_ply);

    // Move ordering tables
    static Move killer_moves[MAX_PLY][2];
    static int  history[COLOR_NB][64][64];

    static void update_killers(Move m, int ply);
    static void update_history(Color c, Move m, int depth);
    static int  score_move(const Board& b, Move m, int ply);

    static int negamax(Board& b, int depth, int alpha, int beta, int ply, SearchInfo& info);
    static int quiescence(Board& b, int alpha, int beta, int ply);
    static void update_pv(int ply, Move m);
};
