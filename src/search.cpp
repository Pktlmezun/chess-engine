#include "search.h"
#include "eval.h"
#include "movegen.h"
#include <iostream>
#include <algorithm>
#include <cstring>
#include <cassert>

// Insertion sort — faster than std::sort for small arrays (chess move lists)
static void insertion_sort(ScoredMove* begin, ScoredMove* end) {
    for (ScoredMove* i = begin + 1; i < end; ++i) {
        ScoredMove key = *i;
        ScoredMove* j = i - 1;
        while (j >= begin && j->score < key.score) {
            *(j + 1) = *j;
            --j;
        }
        *(j + 1) = key;
    }
}

constexpr int MATE_SCORE    = SCORE_MATE;           // 999000
constexpr int MATE_THRESHOLD = SCORE_MATE / 2;      // 499500

// Mate scores are stored relative to the mating node, not to the root, so an
// entry stays valid whatever ply it is read back at.
static inline int to_tt_score(int score, int ply) {
    if (score >  MATE_THRESHOLD) return score + ply;
    if (score < -MATE_THRESHOLD) return score - ply;
    return score;
}

static inline int from_tt_score(int score, int ply) {
    if (score >  MATE_THRESHOLD) return score - ply;
    if (score < -MATE_THRESHOLD) return score + ply;
    return score;
}

constexpr int KILLER_SCORE  = 9000000;
constexpr int HISTORY_MAX   = 1 << 21;
constexpr int64_t MAX_TIME_MS = 300000;             // 5 minutes max

std::atomic<bool> Search::stopped{false};
std::mutex Search::output_mutex;

uint64_t Search::nodes = 0;
Move     Search::pv_table[MAX_PLY][MAX_PLY];
int      Search::pv_len[MAX_PLY];
SearchLimits Search::limits{};
std::chrono::steady_clock::time_point Search::search_start{};
int64_t  Search::soft_limit_ms = MAX_TIME_MS;
int64_t  Search::hard_limit_ms = MAX_TIME_MS;
Move     Search::killer_moves[MAX_PLY][2];
int      Search::history[COLOR_NB][64][64];
TranspositionTable Search::tt;
Move Search::last_validated_bestmove;

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

// Milliseconds reserved for process scheduling and GUI round-trip. Without it the
// engine sends its move at exactly the budget and loses on time.
static constexpr int64_t MOVE_OVERHEAD_MS = 50;

void Search::set_time_limits(Color side, int game_ply) {
    if (limits.infinite) {
        soft_limit_ms = hard_limit_ms = MAX_TIME_MS;
        return;
    }

    if (limits.movetime > 0) {
        hard_limit_ms = std::max(INT64_C(1), limits.movetime - MOVE_OVERHEAD_MS);
        soft_limit_ms = hard_limit_ms;
        return;
    }

    // No clock information at all: this is a pure `go depth N` / `go` search, so
    // the only limit is the depth the caller asked for.
    if (limits.wtime <= 0 && limits.btime <= 0) {
        soft_limit_ms = hard_limit_ms = MAX_TIME_MS;
        return;
    }

    int64_t our_time = (side == WHITE) ? limits.wtime : limits.btime;
    int64_t our_inc  = (side == WHITE) ? limits.winc  : limits.binc;

    if (our_time <= 0) {
        // We have a clock but our own side's is empty — move essentially at once.
        soft_limit_ms = hard_limit_ms = 10;
        return;
    }

    // An absolute reserve the search may never touch. It must NOT scale with the
    // remaining time: a proportional reserve shrinks as the clock drains, so the
    // clock's steady state is zero and the engine eventually flags.
    int64_t reserve = MOVE_OVERHEAD_MS + 200;
    int64_t usable  = std::max(INT64_C(0), our_time - reserve);

    if (usable == 0) {
        // Down to the reserve: move as good as instantly.
        soft_limit_ms = hard_limit_ms = 1;
        return;
    }

    // Assume a long game still to come. This floor sets where the clock settles:
    // a search runs to the hard limit (2 * base), so in a long game the steady
    // state is roughly where 2 * usable / moves_left equals the increment. A floor
    // of 40 keeps that around a second of buffer at 10s + 0.1s, whereas the old
    // floor of 20 combined with a 3/4-increment bonus settled near 500ms — thin
    // enough that a normal game of 80+ moves simply ran the clock out.
    int moves_left = std::max(40, 60 - game_ply / 2);

    // Only a quarter of the increment is added on top of the per-move share; the
    // increment is income, and spending all of it leaves the clock no way to grow.
    int64_t base = usable / moves_left + our_inc / 4;

    soft_limit_ms = std::min(base, usable / 4);
    // The hard limit lets one critical iteration run over the plan, but never far
    // enough to flag.
    hard_limit_ms = std::min(base * 2, usable / 2);

    soft_limit_ms = std::max(soft_limit_ms, INT64_C(1));
    hard_limit_ms = std::max(hard_limit_ms, soft_limit_ms);
}

int64_t Search::elapsed_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - search_start).count();
}

// Abort the search once the hard budget is spent. Previously the budget was only
// honoured *between* iterations, so a single deep iteration could overrun it
// without bound and flag on the clock.
void Search::check_time() {
    if (limits.infinite)
        return;
    if (elapsed_ms() >= hard_limit_ms)
        stopped.store(true, std::memory_order_relaxed);
}

void Search::stop() {
    stopped.store(true, std::memory_order_relaxed);
}

void Search::clear_tt() {
    if (!tt.is_initialized())
        tt.resize(16);
    tt.clear();
}

void Search::set_hash_size(size_t mb) {
    if (mb < 1)    mb = 1;
    if (mb > 4096) mb = 4096;
    tt.resize(mb);
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

    if ((nodes & 2047) == 0) {
        check_time();
        if (stopped.load(std::memory_order_relaxed))
            return 0;
    }

    // Never recurse past the arrays indexed by ply.
    if (ply >= MAX_PLY - 1)
        return evaluate(b);

    int stand_pat = evaluate(b);
    int best_score = stand_pat;

    if (stand_pat >= beta)
        return stand_pat;

    if (stand_pat + 900 < alpha)
        return stand_pat;

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

    insertion_sort(scored, scored + count);

    for (int i = 0; i < count; ++i) {
        b.make_move(scored[i].move);

        if (b.is_attacked(b.king_square(~b.side_to_move), b.side_to_move)) {
            b.unmake_move(scored[i].move);
            continue;
        }

        int score = -quiescence(b, -beta, -alpha, ply + 1);

        b.unmake_move(scored[i].move);

        if (score > best_score) {
            best_score = score;
            if (score >= beta)
                return best_score;
            if (score > alpha)
                alpha = score;
        }
    }

    return best_score;
}

int Search::negamax(Board& b, int depth, int alpha, int beta, int ply, SearchInfo& info) {
    nodes++;

    if ((nodes & 2047) == 0) {
        check_time();
        if (stopped.load(std::memory_order_relaxed))
            return 0;
    }

    // Draw detection: fifty-move rule and repetition. Without the repetition
    // check the engine cannot see repetition draws and will walk into them.
    if (ply > 0 && (b.rule50() >= 100 || b.is_repetition()))
        return 0;

    bool is_root = (ply == 0);
    bool in_check = b.in_check();

    // Guard every array indexed by ply. update_pv() touches pv_table[ply + 1],
    // so running past MAX_PLY is an out-of-bounds write, not just a deep search.
    if (ply >= MAX_PLY - 2)
        return in_check ? evaluate(b) : quiescence(b, alpha, beta, ply);

    // Check extension: search one ply deeper when in check. Capped, because an
    // uncapped extension never lets depth fall in a long forcing sequence.
    if (in_check && ply + depth < MAX_PLY - 4)
        depth++;

    if (depth <= 0)
        return quiescence(b, alpha, beta, ply);

    // Reset PV length for this ply so stale data from previous iterations
    // doesn't leak into the PV when subtrees are pruned (null move, futility, etc.)
    pv_len[ply] = 0;

    // ── TT probe ──────────────────────────────────────────────────────────
    TTEntry tt_entry;
    Move tt_move = Move::null();
    bool tt_hit = tt.probe(b.hash(), tt_entry);
    if (tt_hit) {
        tt_move = tt_entry.best_move;
        if (!is_root) {
            int tt_score = from_tt_score(tt_entry.score, ply);

            if (tt_entry.depth >= depth) {
                if (tt_entry.flag == TT_EXACT)
                    return tt_score;
                if (tt_entry.flag == TT_ALPHA && tt_score <= alpha)
                    return tt_score;
                if (tt_entry.flag == TT_BETA && tt_score >= beta)
                    return tt_score;
            }
        }
    }

    // ── Null Move Pruning ─────────────────────────────────────────────────
    // Skip our turn; if we're still winning with depth reduced, prune.
    // One static eval per node, shared by null-move and futility pruning.
    int static_eval = in_check ? SCORE_NONE : evaluate(b);

    // Null move is unsound in zugzwang, where passing is better than any legal
    // move. Require the side to move to have a piece besides king and pawns.
    bool has_pieces = (b.pieces(b.side_to_move) &
                       ~(b.pieces(b.side_to_move, PAWN) | b.pieces(b.side_to_move, KING))) != 0;

    if (!is_root && !in_check && depth >= 3 && ply > 0 && has_pieces) {
        int eval = static_eval;
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

    // If we have a TT move, verify it's in our move list (catches hash collisions)
    if (!tt_move.is_null()) {
        bool found = false;
        for (int i = 0; i < count; ++i) {
            if (scored[i].move == tt_move) { found = true; break; }
        }
        if (!found) tt_move = Move::null();
    }

    // If we have a TT move, promote it to the front
    if (!tt_move.is_null()) {
        for (int i = 0; i < count; ++i) {
            if (scored[i].move == tt_move) {
                scored[i].score += 10000000;  // ensure TT move is searched first
                break;
            }
        }
    }

    insertion_sort(scored, scored + count);

    int moves_searched = 0;
    int legal_moves = 0;

    for (int i = 0; i < count; ++i) {
        Move m = scored[i].move;

        // Classify the move BEFORE making it. Testing piece_on(m.to()) after
        // make_move always sees the piece that just moved there, never the
        // captured one, so the old checks were never true and both futility
        // pruning and LMR silently never fired.
        bool is_capture = (b.piece_on(m.to()) != NO_PIECE) || m.type() == EN_PASSANT;
        bool is_quiet   = !is_capture && m.type() != PROMOTION;

        b.make_move(m);

        if (b.is_attacked(b.king_square(~b.side_to_move), b.side_to_move)) {
            b.unmake_move(m);
            continue;
        }
        ++legal_moves;

        // side_to_move is the opponent here, so this asks "does our move check them".
        bool gives_check = b.in_check();
        bool tactical = !is_quiet || gives_check;

        // ── Futility Pruning ──────────────────────────────────────────────
        // At low depth, skip quiet moves that cannot plausibly reach alpha.
        if (depth <= 3 && moves_searched > 0 && !in_check && !tactical
            && static_eval != SCORE_NONE
            && static_eval + 100 * depth <= alpha)
        {
            b.unmake_move(m);
            ++moves_searched;
            continue;
        }

        int score;

        // Late Move Reductions: search late quiet moves shallower, then re-search
        // at full depth if the reduced search suggests the move is actually good.
        int reduction = 0;
        if (moves_searched >= 4 && depth >= 3 && !in_check && !tactical) {
            reduction = 1;
            if (moves_searched >= 8) reduction++;
            if (depth >= 6)          reduction++;
            if (reduction > depth - 2) reduction = depth - 2;
        }

        if (moves_searched == 0) {
            score = -negamax(b, depth - 1, -beta, -alpha, ply + 1, info);
        } else {
            score = -negamax(b, depth - 1 - reduction, -alpha - 1, -alpha, ply + 1, info);
            // A reduced search that beats alpha is not trustworthy — verify it.
            if (reduction > 0 && score > alpha)
                score = -negamax(b, depth - 1, -alpha - 1, -alpha, ply + 1, info);
            if (score > alpha && score < beta)
                score = -negamax(b, depth - 1, -beta, -alpha, ply + 1, info);
        }

        b.unmake_move(m);
        ++moves_searched;

        if (score > best_score) {
            best_score = score;
            if (score > alpha) {
                alpha = score;
                best_move = m;
                update_pv(ply, m);
            }
        }

        if (score >= beta) {
            if (is_quiet) {
                update_killers(m, ply);
                // unmake_move() already ran, so side_to_move is the side that
                // made this move — indexing with ~side_to_move wrote the
                // opponent's history table.
                update_history(b.side_to_move, m, depth);
            }
            // TT store
            tt.store(b.hash(), m, depth, to_tt_score(best_score, ply), TT_BETA);
            return best_score;
        }
    }

    // Mate or stalemate — only when there was genuinely no legal move. This used
    // to test best_move.is_null(), which is also true at every fail-low node
    // (no move beat alpha), so such nodes returned 0 and claimed a draw.
    if (legal_moves == 0)
        return in_check ? -MATE_SCORE + ply : 0;

    // TT store
    TTFlag flag = (alpha == old_alpha) ? TT_ALPHA : TT_EXACT;
    tt.store(b.hash(), best_move, depth, to_tt_score(best_score, ply), flag);

    if (ply == 0) {
        info.best_move = best_move;
        info.pv_len = pv_len[0];
        for (int i = 0; i < pv_len[0]; ++i)
            info.pv[i] = pv_table[0][i];
    }

    return best_score;
}

// Returns true if `m` is fully legal in `b` (pseudo-legal *and* does not leave
// our own king in check). MoveGen::generate is only pseudo-legal, so membership
// in its output is not enough.
static bool is_legal_move(Board& b, Move m) {
    Move list[256];
    int count = MoveGen::generate(b, list);
    bool found = false;
    for (int i = 0; i < count; ++i)
        if (list[i] == m) { found = true; break; }
    if (!found)
        return false;

    b.make_move(m);
    bool leaves_king_in_check =
        b.is_attacked(b.king_square(~b.side_to_move), b.side_to_move);
    b.unmake_move(m);
    return !leaves_king_in_check;
}

// First fully legal move in the position, or a null move if there is none.
static Move first_legal_move(Board& b) {
    Move list[256];
    int count = MoveGen::generate(b, list);
    for (int i = 0; i < count; ++i) {
        b.make_move(list[i]);
        bool safe = !b.is_attacked(b.king_square(~b.side_to_move), b.side_to_move);
        b.unmake_move(list[i]);
        if (safe)
            return list[i];
    }
    return Move::null();
}

void Search::go(Board& b, const SearchLimits& lim) {
    nodes = 0;
    limits = lim;
    // NOTE: `stopped` is deliberately NOT reset here. The caller clears it before
    // starting this thread; clearing it again would race with a stop request that
    // arrived in between and silently discard it, leaving the search unstoppable.
    search_start = std::chrono::steady_clock::now();
    tt.new_search();

    if (!tt.is_initialized())
        tt.resize(16);

    set_time_limits(b.side_to_move, b.game_ply);

    SearchInfo info{};
    Move best_move = Move::null();

#ifndef NDEBUG
    // Cheap invariant: the incremental hash must survive a whole search.
    uint64_t expected_hash = b.hash();
    assert(expected_hash == b.compute_hash());
#endif

    int prev_score = 0;

    for (int d = 1; d <= limits.depth; ++d) {
        SearchInfo depth_info{};
        depth_info.depth = d;

        // ── Aspiration window ────────────────────────────────────────────────
        // Search a narrow window around the previous iteration's score; most of
        // the time the score lands inside it and the narrow bounds cut far more
        // than a full window would. Widen and re-search on the misses. Only
        // meaningful because the search is fail-soft: a fail-hard search returns
        // the window edge, which carries no information to widen towards.
        int window = 25;
        int alpha = -SCORE_INF, beta = SCORE_INF;
        if (d >= 4) {
            alpha = prev_score - window;
            beta  = prev_score + window;
        }

        int score;
        bool aborted;
        while (true) {
            SearchInfo attempt{};
            attempt.depth = d;
            score = negamax(b, d, alpha, beta, 0, attempt);
            aborted = stopped.load(std::memory_order_relaxed);

            if (aborted) {
                depth_info = attempt;
                break;
            }

            if (score <= alpha && alpha > -SCORE_INF) {
                // Fail low: the move is worse than we hoped. Keep beta so the
                // re-search stays narrow on one side.
                window *= 2;
                alpha = (window > 6400) ? -SCORE_INF : score - window;
                continue;
            }
            if (score >= beta && beta < SCORE_INF) {
                window *= 2;
                beta = (window > 6400) ? SCORE_INF : score + window;
                continue;
            }

            depth_info = attempt;
            break;
        }

        // The root searches every move, so a completed root move sets best_move.
        // That makes a non-null best_move usable even from an aborted iteration.
        if (!depth_info.best_move.is_null())
            best_move = depth_info.best_move;

        if (aborted)
            break;

        prev_score = score;

        info = depth_info;

        int64_t elapsed = elapsed_ms();
        int nps = (elapsed > 0) ? int(nodes * 1000 / elapsed) : 0;

        // Truncate the reported PV at the first move that is not fully legal.
        // Checking only pseudo-legality here is what let illegal PV moves reach
        // the GUI.
        int pv_len_out = 0;
        {
            Board pv_board = b;
            for (int i = 0; i < depth_info.pv_len; ++i) {
                if (!is_legal_move(pv_board, depth_info.pv[i]))
                    break;
                pv_board.make_move(depth_info.pv[i]);
                ++pv_len_out;
            }
        }

        {
            std::lock_guard<std::mutex> lock(output_mutex);
            std::cout << "info depth " << d
                      << " score cp " << score
                      << " nodes " << nodes
                      << " nps " << nps
                      << " time " << elapsed
                      << " pv";
            for (int i = 0; i < pv_len_out; ++i)
                std::cout << " " << depth_info.pv[i].to_string();
            std::cout << "\n";
            std::cout.flush();
        }

        if (score > MATE_THRESHOLD || score < -MATE_THRESHOLD)
            break;

        // Don't start an iteration we almost certainly cannot finish.
        if (!limits.infinite && elapsed * 3 >= soft_limit_ms * 2)
            break;
    }

#ifndef NDEBUG
    assert(b.hash() == expected_hash && "search corrupted the board");
#endif

    // A bestmove is always required, even if the search was cut off before it
    // finished a single root move.
    if (best_move.is_null())
        best_move = first_legal_move(b);

    {
        std::lock_guard<std::mutex> lock(output_mutex);
        last_validated_bestmove = best_move;
        std::cout << "bestmove " << best_move.to_string() << "\n";
        std::cout.flush();
    }
}
