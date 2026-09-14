#include "eval.h"
#include <algorithm>

// ── Michniewski Piece-Square Tables ──────────────────────────────────────────
// Indexed by [piece_type][square], White's perspective, A1=0..H8=63.
// For Black pieces, flip the square vertically.

// Midgame PSTs (bonus in centipawns)
static constexpr int PST_MG[PIECE_TYPE_NB][SQ_NB] = {
    {}, // NO_PIECE_TYPE
    // PAWN
    {
         0,  0,  0,  0,  0,  0,  0,  0,
        98,134, 61, 95, 68,126, 34,-11,
        -6,  7, 26, 31, 65, 56, 25,-20,
       -14, 13,  6, 21, 23, 12, 17,-23,
       -27, -2, -5, 12, 17,  6, 10,-25,
       -26, -4, -4,-10,  3,  3, 33,-12,
       -35, -1,-20,-23,-15, 24, 38,-22,
         0,  0,  0,  0,  0,  0,  0,  0,
    },
    // KNIGHT
    {
      -167,-89,-34,-49, 61,-97,-15,-107,
       -73,-41, 72, 36, 23, 62,  7, -17,
       -47, 60, 37, 65, 84,129, 73, 44,
        -9, 17, 19, 53, 37, 69, 18, 22,
       -13,  4, 16, 13, 28, 19, 21,  -8,
       -23,  9, 12,  9, 14, 10, 17, -24,
       -29,-13,-24, -9,-25, -2, 11, -26,
       -39,-32,-16,  3, 14, 23, -8,-41,
    },
    // BISHOP
    {
       -29,  4,-82,-37,-25,-42,  7, -8,
       -26, 16,-18,-13, 30, 59, 18,-44,
       -16, 37, 43, 40, 35, 50, 37, -2,
        -4,  5, 16, 13, 14,  9, 19,  7,
         0, 15, 15, 15, 14, 27, 18, 10,
         4, 15, 16,  0,  7, 21, 33,  1,
        -33, -3, -14, -21,-13, 12, 17, -4,
       -26, 16,-16, -5, -2, 18, -6,-31,
    },
    // ROOK
    {
        32, 42, 32, 51, 63,  9, 31, 43,
        27, 32, 58, 62, 80, 67, 26, 44,
        -5, 19, 26, 36, 17, 45, 61, 16,
       -24,-11,  7, 26, 24, 35,  8,-20,
       -36,-26,-12, -1,  9, -7,  6,-23,
       -45,-25,-16,-17,  3,  0, -5,-33,
       -44,-16,-20,-10,-12,  2, 11,-42,
       -19,-13,  1, 17, 16,  7,-37,-26,
    },
    // QUEEN
    {
       -28,-13, 22, 22, 22, 11,-18,-33,
       -24,-18, 17, 13, 15, 20, -5,-18,
        -2,-19, 10,  7, 12, 12,  6, -2,
       -16,-27,  3, 11, 14, 13,  5,-25,
       -12,-14, -2,  5, -1,  4, -9,-16,
        -9,-20, -6,  4, -4, 12, -8,-24,
       -25,-16,-17, -2, -1, -5,-18,-34,
       -29,-42,-14,-21,-13, -9,-34,-22,
    },
    // KING (midgame — stay safe in corner)
    {
      -90,-70,-70,-70,-70,-70,-70,-90,
      -60,-50,-50,-50,-50,-50,-50,-60,
      -40,-30,-30,-40,-40,-30,-30,-40,
      -30,-30,-10,-20,-20,-10,-30,-30,
      -20,-20,  0,  0,  0,  0,-20,-20,
      -10, 20, 30, 30, 30, 30, 20,-10,
       30, 40, 40, 50, 50, 40, 40, 30,
       -10, 20, 60, 60, 60, 60, 20,-10,
    },
};

// Endgame PSTs
static constexpr int PST_EG[PIECE_TYPE_NB][SQ_NB] = {
    {}, // NO_PIECE_TYPE
    // PAWN
    {
         0,  0,  0,  0,  0,  0,  0,  0,
       178,173,158,134,147,132,165,187,
        94,100, 85, 67, 56, 53, 82, 84,
        32, 24, 13,  5, -2,  4, 17, 17,
        13,  9, -3, -7, -7, -8,  3, -1,
         4,  7, -6,  1,  0, -5, -1, -8,
        13,  8,  8,  2, 21, 16, 13,  9,
         0,  0,  0,  0,  0,  0,  0,  0,
    },
    // KNIGHT
    {
       -58,-38,-13,-28,-31,-27,-63,-99,
       -25, -8,-25,-24,-34,-44,-58,-48,
       -42,-20,-10, -5, -2,-20,-24,-41,
       -13,  4, 16, 13, 14,  4, -5, -4,
       -17, 10, 17, 16, 15,  8, -1,-13,
       -24, -7, -1, -8, -5, -4, -5,-17,
       -28,-14,-21,-13, -9,-13,-28,-27,
       -39,-32,-16,  3, 14, 23, -8,-41,
    },
    // BISHOP
    {
       -14,-21,-11, -8, -7, -9,-17, -24,
        -8, -4,  7,-12, -3,-13, -4,-14,
         2, -8,  0, -1, -2,  6,  0,  4,
        -3,  9, 12,  9, 14, 10, 13, -1,
        -4,  3, 13, 19,  7, 10, -3, -9,
       -12, -3,  8, 10, 13,  3, -7, -15,
       -14,-18, -7, -1,  4, -9,-15,-27,
       -23, -9,-23, -5, -9,-16, -5,-17,
    },
    // ROOK
    {
        13, 10, 18, 15, 12, 12,  8,  5,
        11, 13, 13, 11, -3,  3,  8,  3,
         7,  7,  7,  5,  4, -3, -5, -3,
         4,  3,-13, -1, -2,  1, -1,  4,
        -2,-10, -6, -4, -2,  3, -3, -4,
        -7,  2, -2, -2, -2,  2,  6, -1,
        -9,  1, 13, 10, 13,  1,  2, -7,
        -1,  7,  7, 10,  7,  4,  5, -3,
    },
    // QUEEN
    {
        -9,-22,-23,-27,-30,-25,-18,-38,
       -16,-27,-15, -9, -8,-25,-11,-31,
        -6, -1, 13, 10,  9,  9, 14,-24,
        -9, 15, 17, 20, 18, 23, 25, 18,
         1, 17, 19, 27, 25, 22, 18, 10,
        -9, 11, 13, 20, 19, 15, 11,  1,
       -14, -5,  1,  8, 10,  5,  3,-13,
       -29,-42,-14,-21,-13, -9,-34,-22,
    },
    // KING (endgame — centralize the king)
    {
      -74,-35,-18,-18,-11, 15, 4,-17,
      -12, 17, 14, 17, 17, 38, 23, 11,
       10, 17, 23, 15, 20, 45, 44, 13,
       -8, 22, 24, 27, 26, 33, 26, 3,
       -18, -4, 21, 24, 27, 23, 9, -11,
      -19, -3, 11, 21, 23, 16, 7, -9,
      -27,-11,  4, 13, 14,  4, -5,-17,
      -53,-34,-21,-11,-28,-14,-24,-43,
    },
};

// ── Phase weights for game phase calculation ─────────────────────────────────
static constexpr int PhaseWeight[PIECE_TYPE_NB] = {
    0,  // NO_PIECE_TYPE
    0,  // PAWN   (pawns don't count for phase)
    1,  // KNIGHT
    1,  // BISHOP
    2,  // ROOK
    4,  // QUEEN
    0,  // KING
};

static constexpr int TOTAL_PHASE = 24;  // 4 knights + 4 bishops + 4 rooks + 2 queens

static int calculate_phase(const Board& b) {
    int phase = 0;
    phase += popcount(b.pieces(WHITE, KNIGHT) | b.pieces(BLACK, KNIGHT)) * PhaseWeight[KNIGHT];
    phase += popcount(b.pieces(WHITE, BISHOP) | b.pieces(BLACK, BISHOP)) * PhaseWeight[BISHOP];
    phase += popcount(b.pieces(WHITE, ROOK)   | b.pieces(BLACK, ROOK))   * PhaseWeight[ROOK];
    phase += popcount(b.pieces(WHITE, QUEEN)  | b.pieces(BLACK, QUEEN))  * PhaseWeight[QUEEN];
    phase = std::min(phase, TOTAL_PHASE);
    // phase: 0 = endgame, TOTAL_PHASE = opening
    // We want: 0 = opening, 256 = endgame
    return (TOTAL_PHASE - phase) * 256 / TOTAL_PHASE;
}

// ── Hanging piece detection ──────────────────────────────────────────────────
// Penalises pieces that are attacked by enemy but not defended by friendly.
static int evaluate_threats(const Board& b) {
    int penalty = 0;
    Bitboard occ = b.all_pieces();

    for (Color us : {WHITE, BLACK}) {
        Color them = ~us;
        int sign = (us == WHITE) ? -1 : +1;
        Bitboard our_pieces = b.pieces(us) & ~b.pieces(us, PAWN) & ~b.pieces(us, KING);

        while (our_pieces) {
            Square sq = pop_lsb(our_pieces);
            Piece p = b.piece_on(sq);
            PieceType pt = type_of(p);

            Bitboard enemy_att = b.attackers_to(sq, occ) & b.pieces(them);
            if (!enemy_att) continue;

            Bitboard friendly_def = b.attackers_to(sq, occ) & b.pieces(us)
                                  & ~b.pieces(us, PAWN);

            if (!friendly_def) {
                penalty += sign * PieceValue[pt];
            } else if (popcount(enemy_att) > popcount(friendly_def)) {
                penalty += sign * PieceValue[pt] / 4;
            }
        }
    }

    return penalty;
}

// ── Main evaluation ──────────────────────────────────────────────────────────
int evaluate(const Board& b) {
    int mg_score = 0;
    int eg_score = 0;

    // Material + PST — iterate only occupied squares via bitboards
    Bitboard all = b.all_pieces();
    while (all) {
        Square sq = pop_lsb(all);
        Piece p = b.mailbox[sq];
        PieceType pt = type_of(p);
        Color c = color_of(p);
        int idx = (c == WHITE) ? sq : flip(Square(sq));

        int val = PieceValue[pt] + PST_MG[pt][idx];
        int val_eg = PieceValue[pt] + PST_EG[pt][idx];

        if (c == WHITE) {
            mg_score += val;
            eg_score += val_eg;
        } else {
            mg_score -= val;
            eg_score -= val_eg;
        }
    }

    // Hanging pieces / threats
    int threat_score = evaluate_threats(b);
    mg_score += threat_score;
    eg_score += threat_score;

    // Interpolate midgame/endgame
    int phase = calculate_phase(b);
    int final_score = (mg_score * (256 - phase) + eg_score * phase) / 256;

    // Return from side-to-move perspective
    return (b.side_to_move == WHITE) ? final_score : -final_score;
}
