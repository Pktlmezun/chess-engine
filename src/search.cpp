#include "search.h"
#include "eval.h"
#include "movegen.h"
#include <iostream>
#include <algorithm>
#include <cstring>

constexpr int MATE_SCORE    = SCORE_MATE;           // 999000
constexpr int MATE_THRESHOLD = SCORE_MATE / 2;      // 499500

constexpr int KILLER_SCORE  = 9000000;
constexpr int HISTORY_MAX   = 1 << 21;
constexpr int64_t MAX_TIME_MS = 300000;             // 5 minutes max

std::atomic<bool> Search::stopped{false};
std::mutex Search::output_mutex;

uint64_t Search::nodes = 0;
Move     Search::pv_table[64][64];
int      Search::pv_len[64];
SearchLimits Search::limits{};
std::chrono::steady_clock::time_point Search::search_start{};
Move     Search::killer_moves[MAX_PLY][2];
int      Search::history[COLOR_NB][64][64];
TranspositionTable Search::tt;

// ── Transposition Table implementation ───────────────────────────────────────

void TranspositionTable::resize(size_t mb) {
    size_t bytes = mb * 1024 * 1024;
    size_t entries = bytes / sizeof(TTEntry);
    // Round down to power of 2 for fast modulo
    mask = 1;
    while (mask * 2 <= entries) mask *= 2;
    table.resize(mask + 1);
    clear();
}

void TranspositionTable::clear() {
    std::fill(table.begin(), table.end(), TTEntry{});
    current_age = 0;
}

bool TranspositionTable::probe(uint64_t key, TTEntry& entry) const {
    entry = table[key & mask];
    return entry.key == key;
}

void TranspositionTable::store(uint64_t key, Move best_move, int depth, int score, TTFlag flag) {
    size_t idx = key & mask;
    TTEntry& existing = table[idx];

    // Replacement strategy: always replace if different position,
    // or if same position but we have equal or greater depth,
    // or if the entry is from an old search
    if (existing.key != key
        || existing.age < current_age
        || depth >= existing.depth)
    {
        existing.key       = key;
        existing.best_move = best_move;
        existing.depth     = static_cast<int16_t>(depth);
        existing.score     = static_cast<int16_t>(score);
        existing.flag      = static_cast<uint8_t>(flag);
        existing.age       = current_age;
    }
}

int64_t Search::alloc_time_ms(Color side, int game_ply) {
    if (limits.movetime > 0)
        return limits.movetime;

    if (limits.infinite)
        return MAX_TIME_MS;

    int64_t our_time = (side == WHITE) ? limits.wtime : limits.btime;
    int64_t our_inc  = (side == WHITE) ? limits.winc  : limits.binc;

    if (our_time <= 0)
        return 1000; // default 1 second

    int moves_left = std::max(20, 30 - game_ply / 2);
    int64_t t = our_time / moves_left + our_inc / 2;
    t = std::min(t, our_time / 2);
    return std::max(t, INT64_C(1));
}

void Search::stop() {
    stopped.store(true, std::memory_order_relaxed);
}

void Search::clear_tables() {
    memset(killer_moves, 0, sizeof(killer_moves));
    memset(history, 0, sizeof(history));
}

void Search::update_killers(Move m, int ply) {
    if (m != killer_moves[ply][0]) {
        killer_moves[ply][1] = killer_moves[ply][0];
        killer_moves[ply][0] = m;
    }
}

void Search::update_history(Color c, Move m, int depth) {
    history[c][m.from()][m.to()] += depth * depth;
    if (history[c][m.from()][m.to()] > HISTORY_MAX) {
        for (int f = 0; f < 64; ++f)
            for (int t = 0; t < 64; ++t)
                history[c][f][t] /= 2;
    }
}

int Search::score_move(const Board& b, Move m, int ply) {
    // MVV-LVA captures first — queen captures must outrank en passant
    Piece victim = b.piece_on(m.to());
    if (victim != NO_PIECE)
        return 10000 + PieceValue[type_of(victim)] * 10 - PieceValue[type_of(b.piece_on(m.from()))];

    if (m.type() == EN_PASSANT)
        return 8000 + PieceValue[PAWN] * 10 - PieceValue[PAWN];

    if (m.type() == PROMOTION)
        return 9000 + PieceValue[m.promotion()];

    if (ply < MAX_PLY) {
        if (m == killer_moves[ply][0]) return KILLER_SCORE;
        if (m == killer_moves[ply][1]) return KILLER_SCORE - 3;
    }

    return history[b.side_to_move][m.from()][m.to()];
}

void Search::update_pv(int ply, Move m) {
    pv_table[ply][0] = m;
    for (int j = 0; j < pv_len[ply + 1]; ++j)
        pv_table[ply][j + 1] = pv_table[ply + 1][j];
    pv_len[ply] = pv_len[ply + 1] + 1;
}

int Search::quiescence(Board& b, int alpha, int beta, int ply) {
    nodes++;

    if ((nodes & 2047) == 0 && stopped.load(std::memory_order_relaxed))
        return 0;

    int stand_pat = evaluate(b);

    if (stand_pat >= beta)
        return beta;

    if (stand_pat + 900 < alpha)
        return alpha;

    if (stand_pat > alpha)
        alpha = stand_pat;

    Move list[256];
    int count = MoveGen::generate(b, list, CAPTURES_ONLY);

    ScoredMove scored[256];
    for (int i = 0; i < count; ++i) {
        scored[i].move = list[i];
        Piece victim   = b.piece_on(list[i].to());
        Piece attacker = b.piece_on(list[i].from());
        if (list[i].type() == EN_PASSANT)
            scored[i].score = PieceValue[PAWN] * 10 - PieceValue[PAWN];
        else
            scored[i].score = PieceValue[type_of(victim)] * 10 - PieceValue[type_of(attacker)];
    }

    std::sort(scored, scored + count,
        [](const ScoredMove& a, const ScoredMove& b) { return a.score > b.score; });

    for (int i = 0; i < count; ++i) {
        b.make_move(scored[i].move);

        if (b.is_attacked(b.king_square(~b.side_to_move), b.side_to_move)) {
            b.unmake_move(scored[i].move);
            continue;
        }

        int score = -quiescence(b, -beta, -alpha, ply + 1);

        b.unmake_move(scored[i].move);

        if (score >= beta)
            return beta;
        if (score > alpha)
            alpha = score;
    }

    return alpha;
}

int Search::negamax(Board& b, int depth, int alpha, int beta, int ply, SearchInfo& info) {
    nodes++;

    if ((nodes & 2047) == 0 && stopped.load(std::memory_order_relaxed))
        return 0;

    // Draw detection
    if (ply > 0 && b.rule50() >= 100)
        return 0;

    bool is_root = (ply == 0);
    bool in_check = b.in_check();

    // Check extension: search one ply deeper when in check
    if (in_check) depth++;

    if (depth <= 0)
        return quiescence(b, alpha, beta, ply);

    // ── TT probe ──────────────────────────────────────────────────────────
    TTEntry tt_entry;
    Move tt_move = Move::null();
    bool tt_hit = tt.probe(b.hash(), tt_entry);
    if (tt_hit) {
        tt_move = tt_entry.best_move;
        if (!is_root) {
            int tt_score = tt_entry.score;
            // Adjust mate scores for ply distance
            if (tt_score > MATE_THRESHOLD) tt_score -= ply;
            else if (tt_score < -MATE_THRESHOLD) tt_score += ply;

            if (tt_entry.depth >= depth) {
                if (tt_entry.flag == TT_EXACT)
                    return tt_score;
                if (tt_entry.flag == TT_ALPHA && tt_score <= alpha)
                    return alpha;
                if (tt_entry.flag == TT_BETA && tt_score >= beta)
                    return beta;
            }
        }
    }

    // ── Null Move Pruning ─────────────────────────────────────────────────
    // Skip our turn; if we're still winning with depth reduced, prune.
    if (!is_root && !in_check && depth >= 3 && ply > 0) {
        // Evaluate static position — if already >= beta, skip null move
        int eval = evaluate(b);
        if (eval >= beta) {
            int R = 3;
            // Temporarily flip side, clear EP
            Color orig_side = b.side_to_move;
            Square orig_ep = b.ep_square();
            b.side_to_move = ~orig_side;
            b.state_stack[b.state_idx].ep_square = SQ_NONE;
            b.state_stack[b.state_idx].hash ^= Board::ZobristSide;
            if (orig_ep != SQ_NONE)
                b.state_stack[b.state_idx].hash ^= Board::ZobristEP[file_of(orig_ep)];

            int null_score = -negamax(b, depth - 1 - R, -beta, -beta + 1, ply + 1, info);

            // Restore
            b.state_stack[b.state_idx].hash ^= Board::ZobristSide;
            b.side_to_move = orig_side;
            b.state_stack[b.state_idx].ep_square = orig_ep;
            if (orig_ep != SQ_NONE)
                b.state_stack[b.state_idx].hash ^= Board::ZobristEP[file_of(orig_ep)];

            if (null_score >= beta)
                return beta;
        }
    }

    Move list[256];
    int count = MoveGen::generate(b, list);

    Move best_move = Move::null();
    int best_score = -SCORE_INF;
    int old_alpha = alpha;

    ScoredMove scored[256];
    for (int i = 0; i < count; ++i)
        scored[i] = { list[i], score_move(b, list[i], ply) };

    // If we have a TT move, promote it to the front
    if (!tt_move.is_null()) {
        for (int i = 0; i < count; ++i) {
            if (scored[i].move == tt_move) {
                scored[i].score += 10000000;  // ensure TT move is searched first
                break;
            }
        }
    }

    std::sort(scored, scored + count,
        [](const ScoredMove& a, const ScoredMove& b) { return a.score > b.score; });

    int moves_searched = 0;

    for (int i = 0; i < count; ++i) {
        b.make_move(scored[i].move);

        if (b.is_attacked(b.king_square(~b.side_to_move), b.side_to_move)) {
            b.unmake_move(scored[i].move);
            continue;
        }

        // ── Futility Pruning ──────────────────────────────────────────────
        // At low depth, skip quiet moves if eval + margin is below alpha.
        if (depth <= 3 && moves_searched > 0 && !in_check
            && scored[i].move.type() != PROMOTION
            && b.piece_on(scored[i].move.to()) == NO_PIECE)
        {
            int futility_margin = 200 * depth;
            int futility_eval = evaluate(b);
            // Note: evaluate returns from side-to-move perspective (opponent after make_move)
            // so we negate to get our perspective
            if (-futility_eval + futility_margin < alpha) {
                b.unmake_move(scored[i].move);
                ++moves_searched;
                continue;
            }
        }

        int score;

        // Late Move Reductions: reduce depth for moves sorted late that aren't tactical
        if (moves_searched >= 4 && depth >= 3 && !in_check
            && scored[i].move.type() != PROMOTION
            && b.piece_on(scored[i].move.to()) == NO_PIECE
            && !b.is_attacked(b.king_square(b.side_to_move), ~b.side_to_move))
        {
            score = -negamax(b, depth - 2, -alpha - 1, -alpha, ply + 1, info);
            if (score <= alpha) {
                b.unmake_move(scored[i].move);
                ++moves_searched;
                continue;
            }
        }

        // PVS: full window for first move, zero window for rest
        if (moves_searched == 0) {
            score = -negamax(b, depth - 1, -beta, -alpha, ply + 1, info);
        } else {
            score = -negamax(b, depth - 1, -alpha - 1, -alpha, ply + 1, info);
            if (score > alpha && score < beta)
                score = -negamax(b, depth - 1, -beta, -alpha, ply + 1, info);
        }

        b.unmake_move(scored[i].move);
        ++moves_searched;

        if (score >= beta) {
            if (scored[i].move.type() != EN_PASSANT
                && b.piece_on(scored[i].move.to()) == NO_PIECE)
            {
                update_killers(scored[i].move, ply);
                update_history(~b.side_to_move, scored[i].move, depth);
            }
            // TT store
            tt.store(b.hash(), scored[i].move, depth, score, TT_BETA);
            return beta;
        }

        if (score > alpha) {
            alpha = score;
            best_move = scored[i].move;
            update_pv(ply, scored[i].move);
        }
    }

    if (best_move.is_null()) {
        if (in_check)
            return -MATE_SCORE + ply;
        else
            return 0;
    }

    // TT store
    TTFlag flag = (alpha == old_alpha) ? TT_ALPHA : TT_EXACT;
    tt.store(b.hash(), best_move, depth, alpha, flag);

    if (ply == 0) {
        info.best_move = best_move;
        info.pv_len = pv_len[0];
        for (int i = 0; i < pv_len[0]; ++i)
            info.pv[i] = pv_table[0][i];
    }

    return alpha;
}

void Search::go(Board& b, const SearchLimits& lim) {
    nodes = 0;
    limits = lim;
    stopped.store(false, std::memory_order_relaxed);
    search_start = std::chrono::steady_clock::now();
    clear_tables();
    tt.new_search();

    // Initialize TT with 16MB on first use
    if (!tt.is_initialized())
        tt.resize(16);

    SearchInfo info{};

    int64_t time_budget = alloc_time_ms(b.side_to_move, b.game_ply);

    for (int d = 1; d <= limits.depth; ++d) {
        info.depth = d;
        int score = negamax(b, d, -SCORE_INF, SCORE_INF, 0, info);

        if (stopped.load(std::memory_order_relaxed))
            break;

        auto now = std::chrono::steady_clock::now();
        int elapsed_ms = int(std::chrono::duration_cast<std::chrono::milliseconds>(now - search_start).count());
        int nps = (elapsed_ms > 0) ? int(nodes * 1000 / elapsed_ms) : 0;

        std::lock_guard<std::mutex> lock(output_mutex);
        std::cout << "info depth " << d
                  << " score cp " << score
                  << " nodes " << nodes
                  << " nps " << nps
                  << " time " << elapsed_ms
                  << " pv ";
        for (int i = 0; i < info.pv_len; ++i)
            std::cout << info.pv[i].to_string() << " ";
        std::cout << "\n";
        std::cout.flush();

        if (score > MATE_THRESHOLD || score < -MATE_THRESHOLD)
            break;

        if (!limits.infinite) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - search_start).count();
            if (elapsed * 4 > time_budget)
                break;
        }
    }

    {
        std::lock_guard<std::mutex> lock(output_mutex);
        std::cout << "bestmove " << info.pv[0].to_string() << "\n";
        std::cout.flush();
    }
}
