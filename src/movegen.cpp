#include "movegen.h"
#include "bitboard.h"
#include <cstring>

// ── Internal helpers ──────────────────────────────────────────────────────────

static int add_promotions(Move* list, int n, Square from, Square to) {
    list[n++] = Move::make(from, to, PROMOTION, QUEEN);
    list[n++] = Move::make(from, to, PROMOTION, ROOK);
    list[n++] = Move::make(from, to, PROMOTION, BISHOP);
    list[n++] = Move::make(from, to, PROMOTION, KNIGHT);
    return n;
}

// ── Pawn moves ────────────────────────────────────────────────────────────────
static int gen_pawns(const Board& b, Move* list, int n, GenType type) {
    Color us   = b.side_to_move;
    Color them = ~us;
    Bitboard pawns  = b.pieces(us, PAWN);
    Bitboard occ    = b.all_pieces();
    Bitboard theirs = b.pieces(them);

    if (us == WHITE) {
        // Single push
        Bitboard push1 = (pawns << 8) & ~occ;
        // Double push from rank 2
        Bitboard push2 = ((push1 & RankBB[RANK_3]) << 8) & ~occ;

        if (type != CAPTURES_ONLY) {
            // Single push non-promotions
            Bitboard non_promo = push1 & ~RankBB[RANK_8];
            while (non_promo) {
                Square to = pop_lsb(non_promo);
                list[n++] = Move::make(Square(to - 8), to);
            }
            // Double push
            while (push2) {
                Square to = pop_lsb(push2);
                list[n++] = Move::make(Square(to - 16), to);
            }
        }

        // Promotions (pushes to rank 8)
        Bitboard promo_push = push1 & RankBB[RANK_8];
        while (promo_push) {
            Square to = pop_lsb(promo_push);
            n = add_promotions(list, n, Square(to - 8), to);
        }

        // Captures
        Bitboard cap_left  = ((pawns & ~FileBB[FILE_A]) << 7) & theirs;
        Bitboard cap_right = ((pawns & ~FileBB[FILE_H]) << 9) & theirs;

        // Capture-promotions
        Bitboard cl_promo = cap_left  & RankBB[RANK_8];
        Bitboard cr_promo = cap_right & RankBB[RANK_8];
        while (cl_promo) { Square to = pop_lsb(cl_promo); n = add_promotions(list, n, Square(to - 7), to); }
        while (cr_promo) { Square to = pop_lsb(cr_promo); n = add_promotions(list, n, Square(to - 9), to); }

        // Normal captures
        Bitboard cl_norm = cap_left  & ~RankBB[RANK_8];
        Bitboard cr_norm = cap_right & ~RankBB[RANK_8];
        while (cl_norm) { Square to = pop_lsb(cl_norm); list[n++] = Move::make(Square(to - 7), to); }
        while (cr_norm) { Square to = pop_lsb(cr_norm); list[n++] = Move::make(Square(to - 9), to); }

        // En passant
        Square ep = b.ep_square();
        if (ep != SQ_NONE) {
            Bitboard ep_pawns = PawnAttacks[BLACK][ep] & pawns;
            while (ep_pawns) {
                Square from = pop_lsb(ep_pawns);
                list[n++] = Move::make(from, ep, EN_PASSANT);
            }
        }

    } else { // BLACK
        Bitboard push1 = (pawns >> 8) & ~occ;
        Bitboard push2 = ((push1 & RankBB[RANK_6]) >> 8) & ~occ;

        if (type != CAPTURES_ONLY) {
            Bitboard non_promo = push1 & ~RankBB[RANK_1];
            while (non_promo) {
                Square to = pop_lsb(non_promo);
                list[n++] = Move::make(Square(to + 8), to);
            }
            while (push2) {
                Square to = pop_lsb(push2);
                list[n++] = Move::make(Square(to + 16), to);
            }
        }

        Bitboard promo_push = push1 & RankBB[RANK_1];
        while (promo_push) {
            Square to = pop_lsb(promo_push);
            n = add_promotions(list, n, Square(to + 8), to);
        }

        Bitboard cap_right = ((pawns & ~FileBB[FILE_H]) >> 7) & theirs;
        Bitboard cap_left  = ((pawns & ~FileBB[FILE_A]) >> 9) & theirs;

        Bitboard cr_promo = cap_right & RankBB[RANK_1];
        Bitboard cl_promo = cap_left  & RankBB[RANK_1];
        while (cr_promo) { Square to = pop_lsb(cr_promo); n = add_promotions(list, n, Square(to + 7), to); }
        while (cl_promo) { Square to = pop_lsb(cl_promo); n = add_promotions(list, n, Square(to + 9), to); }

        Bitboard cr_norm = cap_right & ~RankBB[RANK_1];
        Bitboard cl_norm = cap_left  & ~RankBB[RANK_1];
        while (cr_norm) { Square to = pop_lsb(cr_norm); list[n++] = Move::make(Square(to + 7), to); }
        while (cl_norm) { Square to = pop_lsb(cl_norm); list[n++] = Move::make(Square(to + 9), to); }

        Square ep = b.ep_square();
        if (ep != SQ_NONE) {
            Bitboard ep_pawns = PawnAttacks[WHITE][ep] & pawns;
            while (ep_pawns) {
                Square from = pop_lsb(ep_pawns);
                list[n++] = Move::make(from, ep, EN_PASSANT);
            }
        }
    }
    return n;
}

// ── Piece moves (knights, bishops, rooks, queens, king) ───────────────────────
static int gen_piece(const Board& b, Move* list, int n, PieceType pt, GenType type) {
    Color us    = b.side_to_move;
    Bitboard ours  = b.pieces(us);
    Bitboard occ   = b.all_pieces();
    Bitboard pcs   = b.pieces(us, pt);

    while (pcs) {
        Square from = pop_lsb(pcs);
        Bitboard attacks;
        switch (pt) {
            case KNIGHT: attacks = KnightAttacks[from]; break;
            case BISHOP: attacks = bishop_attacks(from, occ); break;
            case ROOK:   attacks = rook_attacks(from, occ);   break;
            case QUEEN:  attacks = queen_attacks(from, occ);  break;
            case KING:   attacks = KingAttacks[from];          break;
            default:     attacks = 0; break;
        }
        attacks &= ~ours;
        if (type == CAPTURES_ONLY) attacks &= b.pieces(~us);

        while (attacks) {
            Square to = pop_lsb(attacks);
            list[n++] = Move::make(from, to);
        }
    }
    return n;
}

// ── Castling ──────────────────────────────────────────────────────────────────
static int gen_castling(const Board& b, Move* list, int n) {
    Color us  = b.side_to_move;
    Bitboard occ = b.all_pieces();

    if (us == WHITE) {
        // Kingside: e1-f1-g1 must be empty, e1/f1/g1 must not be attacked
        if (b.can_castle(WHITE_OO)
            && !(occ & (sq_bb(F1) | sq_bb(G1)))
            && !b.is_attacked(E1, BLACK)
            && !b.is_attacked(F1, BLACK)
            && !b.is_attacked(G1, BLACK))
        {
            list[n++] = Move::make(E1, G1, CASTLING);
        }
        // Queenside: b1-c1-d1 must be empty, e1/d1/c1 must not be attacked
        if (b.can_castle(WHITE_OOO)
            && !(occ & (sq_bb(B1) | sq_bb(C1) | sq_bb(D1)))
            && !b.is_attacked(E1, BLACK)
            && !b.is_attacked(D1, BLACK)
            && !b.is_attacked(C1, BLACK))
        {
            list[n++] = Move::make(E1, C1, CASTLING);
        }
    } else {
        if (b.can_castle(BLACK_OO)
            && !(occ & (sq_bb(F8) | sq_bb(G8)))
            && !b.is_attacked(E8, WHITE)
            && !b.is_attacked(F8, WHITE)
            && !b.is_attacked(G8, WHITE))
        {
            list[n++] = Move::make(E8, G8, CASTLING);
        }
        if (b.can_castle(BLACK_OOO)
            && !(occ & (sq_bb(B8) | sq_bb(C8) | sq_bb(D8)))
            && !b.is_attacked(E8, WHITE)
            && !b.is_attacked(D8, WHITE)
            && !b.is_attacked(C8, WHITE))
        {
            list[n++] = Move::make(E8, C8, CASTLING);
        }
    }
    return n;
}

// ── Public interface ──────────────────────────────────────────────────────────
int MoveGen::generate(const Board& b, Move* list, GenType type) {
    int n = 0;
    n = gen_pawns(b, list, n, type);
    n = gen_piece(b, list, n, KNIGHT, type);
    n = gen_piece(b, list, n, BISHOP, type);
    n = gen_piece(b, list, n, ROOK,   type);
    n = gen_piece(b, list, n, QUEEN,  type);
    n = gen_piece(b, list, n, KING,   type);
    if (type != CAPTURES_ONLY) n = gen_castling(b, list, n);
    return n;
}

Move MoveGen::parse(const Board& b, const std::string& str) {
    Move list[256];
    int count = generate(b, list);
    for (int i = 0; i < count; ++i)
        if (list[i].to_string() == str) return list[i];
    return Move::null();
}
