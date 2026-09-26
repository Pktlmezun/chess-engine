#pragma once
#include "board.h"
#include "move.h"
#include <atomic>
#include <chrono>
#include <mutex>
#include <vector>

// ── Transposition Table ──────────────────────────────────────────────────────
enum TTFlag : uint8_t { TT_EXACT, TT_ALPHA, TT_BETA };

struct TTEntry {
    uint64_t key;
    Move     best_move;
    int16_t  depth;
    // Must be wide enough for mate scores. SCORE_MATE is 999000, which silently
    // truncated to 15960 when this field was int16_t — below MATE_THRESHOLD, so
    // a stored mate was read back as an ordinary +159 pawn evaluation.
    int32_t  score;
    uint8_t  flag;
    uint8_t  age;
};

class TranspositionTable {
public:
    void resize(size_t mb);
    void clear();
    void new_search() { ++current_age; }

    bool probe(uint64_t key, TTEntry& entry) const;
    // `score` must already be normalized for ply distance (see to_tt_score).
    void store(uint64_t key, Move best_move, int depth, int score, TTFlag flag);

    bool is_initialized() const { return mask > 0; }

    // Current search age for replacement strategy
    uint8_t current_age = 0;

private:
    std::vector<TTEntry> table;
    size_t mask = 0;
};

struct ScoredMove {
    Move move;
    int  score;
};

static constexpr int SEARCH_MAX_PLY = 128;

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
    Move pv[SEARCH_MAX_PLY];
    int pv_len;
};

class Search {
public:
    static std::atomic<bool> stopped;
    static std::mutex output_mutex;

    static void go(Board& b, const SearchLimits& limits = SearchLimits{});
    static void stop();
    static void clear_tables();
    static Move get_last_bestmove() { return last_validated_bestmove; }

private:
    static constexpr int MAX_PLY = SEARCH_MAX_PLY;

    static uint64_t nodes;
    static Move pv_table[MAX_PLY][MAX_PLY];
    static int pv_len[MAX_PLY];
    static SearchLimits limits;
    static std::chrono::steady_clock::time_point search_start;

    // Time control. `soft` gates starting another iteration, `hard` aborts the
    // search mid-node. Both already exclude the move-overhead reserve.
    static int64_t soft_limit_ms;
    static int64_t hard_limit_ms;
    static void    set_time_limits(Color side, int game_ply);
    static int64_t elapsed_ms();
    static void    check_time();

    // Move ordering tables
    static Move killer_moves[MAX_PLY][2];
    static int  history[COLOR_NB][64][64];

    static void update_killers(Move m, int ply);
    static void update_history(Color c, Move m, int depth);
    static int  score_move(const Board& b, Move m, int ply);

    // Transposition table
    static TranspositionTable tt;

    // Last validated best move (set by go(), read by UCI handler after join)
    static Move last_validated_bestmove;

    static int negamax(Board& b, int depth, int alpha, int beta, int ply, SearchInfo& info);
    static int quiescence(Board& b, int alpha, int beta, int ply);
    static void update_pv(int ply, Move m);
};
