#include "bitboard.h"
#include <cstring>
#include <iostream>
#include <iomanip>

// ── Table storage ─────────────────────────────────────────────────────────────
Bitboard PawnAttacks[COLOR_NB][SQ_NB];
Bitboard KnightAttacks[SQ_NB];
Bitboard KingAttacks[SQ_NB];
Bitboard BishopMasks[SQ_NB];
Bitboard RookMasks[SQ_NB];
Bitboard BishopTable[SQ_NB][512];
Bitboard RookTable[SQ_NB][4096];
Bitboard BetweenBB[SQ_NB][SQ_NB];
Bitboard LineBB[SQ_NB][SQ_NB];
Bitboard FileBB[8];
Bitboard RankBB[8];
Bitboard AdjacentFilesBB[8];

// ── Published magic numbers (Pradyumna Kannan) ────────────────────────────────
const Bitboard BishopMagics[SQ_NB] = {
    0x0002020202020200ULL, 0x0002020202020000ULL, 0x0004010202000000ULL,
    0x0004040080000000ULL, 0x0001104000000000ULL, 0x0000821040000000ULL,
    0x0000410410400000ULL, 0x0000104104104000ULL, 0x0000040404040400ULL,
    0x0000020202020200ULL, 0x0000040102020000ULL, 0x0000040400800000ULL,
    0x0000011040000000ULL, 0x0000008210400000ULL, 0x0000004104104000ULL,
    0x0000002082082000ULL, 0x0004000808080800ULL, 0x0002000404040400ULL,
    0x0001000202020200ULL, 0x0000800802004000ULL, 0x0000800400A00000ULL,
    0x0000200100884000ULL, 0x0000400082082000ULL, 0x0000200041041000ULL,
    0x0002080010101000ULL, 0x0001040008080800ULL, 0x0000208004010400ULL,
    0x0000404004010200ULL, 0x0000840000802000ULL, 0x0000404002011000ULL,
    0x0000808001041000ULL, 0x0000404000820800ULL, 0x0001041000202000ULL,
    0x0000820800101000ULL, 0x0000104400080800ULL, 0x0000020080080080ULL,
    0x0000404040040100ULL, 0x0000808100020100ULL, 0x0001010100020800ULL,
    0x0000808080010400ULL, 0x0000820820004000ULL, 0x0000410410002000ULL,
    0x0000082088001000ULL, 0x0000002011000800ULL, 0x0000080100400400ULL,
    0x0001010101000200ULL, 0x0002020202000400ULL, 0x0001010101000200ULL,
    0x0000410410400000ULL, 0x0000208208200000ULL, 0x0000002084000000ULL,
    0x0000000020880000ULL, 0x0000001002020000ULL, 0x0000040408020000ULL,
    0x0004040404040000ULL, 0x0002020202020000ULL, 0x0000104104104000ULL,
    0x0000002082082000ULL, 0x0000000020841000ULL, 0x0000000008220400ULL,
    0x0000000100800200ULL, 0x0000020080080200ULL, 0x0000404080101000ULL,
    0x0000200040200800ULL
};

const Bitboard RookMagics[SQ_NB] = {
    0x0080001020400080ULL, 0x0040001000200040ULL, 0x0080081000200080ULL,
    0x0080040800100080ULL, 0x0080020400080080ULL, 0x0080010200040080ULL,
    0x0080008001000200ULL, 0x0080002040800100ULL, 0x0000800020400080ULL,
    0x0000400020005000ULL, 0x0000801000200080ULL, 0x0000800800100080ULL,
    0x0000800400080080ULL, 0x0000800200040080ULL, 0x0000800100020080ULL,
    0x0000800040800100ULL, 0x0000208000400080ULL, 0x0000404000201000ULL,
    0x0000808010002000ULL, 0x0000808008001000ULL, 0x0000808004000800ULL,
    0x0000808002000400ULL, 0x0000010100020004ULL, 0x0000020000408104ULL,
    0x0000208080004000ULL, 0x0000200040005000ULL, 0x0000100080200080ULL,
    0x0000080080100080ULL, 0x0000040080080080ULL, 0x0000020080040080ULL,
    0x0000010080800200ULL, 0x0000800080004100ULL, 0x0000204000800080ULL,
    0x0000200040401000ULL, 0x0000100080802000ULL, 0x0000080080801000ULL,
    0x0000040080800800ULL, 0x0000020080800400ULL, 0x0000020001010004ULL,
    0x0000800040800100ULL, 0x0000204000808000ULL, 0x0000200040008080ULL,
    0x0000100020008080ULL, 0x0000080010008080ULL, 0x0000040008008080ULL,
    0x0000020004008080ULL, 0x0000010002008080ULL, 0x0000004081020004ULL,
    0x0000204000800080ULL, 0x0000200040008080ULL, 0x0000100020008080ULL,
    0x0000080010008080ULL, 0x0000040008008080ULL, 0x0000020004008080ULL,
    0x0000800100020080ULL, 0x0000800041000080ULL, 0x00FFFCDDFCED714AULL,
    0x007FFCDDFCED714AULL, 0x003FFFCDFFD88096ULL, 0x0000040810002101ULL,
    0x0000007F37E3FFC0ULL, 0x0000007FB0E4FEFCULL, 0x000000100800A804ULL,
    0x0000000012020408ULL
};

const int BishopShifts[SQ_NB] = {
    58,59,59,59,59,59,59,58,
    59,59,59,59,59,59,59,59,
    59,59,57,57,57,57,59,59,
    59,59,57,55,55,57,59,59,
    59,59,57,55,55,57,59,59,
    59,59,57,57,57,57,59,59,
    59,59,59,59,59,59,59,59,
    58,59,59,59,59,59,59,58
};

const int RookShifts[SQ_NB] = {
    52,53,53,53,53,53,53,52,
    53,54,54,54,54,54,54,53,
    53,54,54,54,54,54,54,53,
    53,54,54,54,54,54,54,53,
    53,54,54,54,54,54,54,53,
    53,54,54,54,54,54,54,53,
    53,54,54,54,54,54,54,53,
    52,53,53,53,53,53,53,52
};

// ── Helpers for building attack masks ────────────────────────────────────────
static Bitboard bishop_attacks_slow(Square s, Bitboard occ) {
    Bitboard attacks = 0;
    int r = rank_of(s), f = file_of(s);
    for (int dr : {1, -1})
        for (int df : {1, -1}) {
            int cr = r + dr, cf = f + df;
            while (cr >= 0 && cr < 8 && cf >= 0 && cf < 8) {
                Square sq = make_square(File(cf), Rank(cr));
                attacks |= sq_bb(sq);
                if (occ & sq_bb(sq)) break;
                cr += dr; cf += df;
            }
        }
    return attacks;
}

static Bitboard rook_attacks_slow(Square s, Bitboard occ) {
    Bitboard attacks = 0;
    int r = rank_of(s), f = file_of(s);
    for (auto [dr, df] : std::initializer_list<std::pair<int,int>>{{1,0},{-1,0},{0,1},{0,-1}}) {
        int cr = r + dr, cf = f + df;
        while (cr >= 0 && cr < 8 && cf >= 0 && cf < 8) {
            Square sq = make_square(File(cf), Rank(cr));
            attacks |= sq_bb(sq);
            if (occ & sq_bb(sq)) break;
            cr += dr; cf += df;
        }
    }
    return attacks;
}

// Carry-Rippler: enumerate all subsets of mask
static Bitboard next_subset(Bitboard subset, Bitboard mask) {
    return (subset - mask) & mask;
}

void init_bitboards() {
    // File and rank masks
    for (int f = 0; f < 8; ++f) {
        FileBB[f] = 0;
        for (int r = 0; r < 8; ++r)
            FileBB[f] |= sq_bb(make_square(File(f), Rank(r)));
    }
    for (int r = 0; r < 8; ++r) {
        RankBB[r] = 0;
        for (int f = 0; f < 8; ++f)
            RankBB[r] |= sq_bb(make_square(File(f), Rank(r)));
    }
    for (int f = 0; f < 8; ++f) {
        AdjacentFilesBB[f] = 0;
        if (f > 0) AdjacentFilesBB[f] |= FileBB[f-1];
        if (f < 7) AdjacentFilesBB[f] |= FileBB[f+1];
    }

    // Leaper attacks
    for (int s = 0; s < SQ_NB; ++s) {
        Square sq = Square(s);
        int r = rank_of(sq), f = file_of(sq);
        Bitboard b = sq_bb(sq);

        // Pawn attacks
        PawnAttacks[WHITE][s] = 0;
        PawnAttacks[BLACK][s] = 0;
        if (f > 0) {
            if (r < 7) PawnAttacks[WHITE][s] |= sq_bb(Square(s + 7));
            if (r > 0) PawnAttacks[BLACK][s] |= sq_bb(Square(s - 9));
        }
        if (f < 7) {
            if (r < 7) PawnAttacks[WHITE][s] |= sq_bb(Square(s + 9));
            if (r > 0) PawnAttacks[BLACK][s] |= sq_bb(Square(s - 7));
        }

        // Knight attacks
        KnightAttacks[s] = 0;
        const int kd[][2] = {{2,1},{2,-1},{-2,1},{-2,-1},{1,2},{1,-2},{-1,2},{-1,-2}};
        for (auto [dr, df] : kd) {
            int nr = r + dr, nf = f + df;
            if (nr >= 0 && nr < 8 && nf >= 0 && nf < 8)
                KnightAttacks[s] |= sq_bb(make_square(File(nf), Rank(nr)));
        }

        // King attacks
        KingAttacks[s] = 0;
        const int kgd[][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
        for (auto [dr, df] : kgd) {
            int nr = r + dr, nf = f + df;
            if (nr >= 0 && nr < 8 && nf >= 0 && nf < 8)
                KingAttacks[s] |= sq_bb(make_square(File(nf), Rank(nr)));
        }

        (void)b;
    }

    // Sliding piece masks (exclude edges)
    const Bitboard not_a = ~FileBB[FILE_A];
    const Bitboard not_h = ~FileBB[FILE_H];
    const Bitboard not_r1 = ~RankBB[RANK_1];
    const Bitboard not_r8 = ~RankBB[RANK_8];

    for (int s = 0; s < SQ_NB; ++s) {
        Square sq = Square(s);
        BishopMasks[s] = bishop_attacks_slow(sq, 0) & not_a & not_h & not_r1 & not_r8;
        RookMasks[s]   = rook_attacks_slow(sq, 0)
                         & ~( (file_of(sq) != FILE_A ? FileBB[FILE_A] : 0)
                            | (file_of(sq) != FILE_H ? FileBB[FILE_H] : 0)
                            | (rank_of(sq) != RANK_1 ? RankBB[RANK_1] : 0)
                            | (rank_of(sq) != RANK_8 ? RankBB[RANK_8] : 0));
    }

    // Build magic tables
    for (int s = 0; s < SQ_NB; ++s) {
        Square sq = Square(s);

        // Bishop
        {
            Bitboard mask = BishopMasks[s];
            Bitboard subset = 0;
            do {
                Bitboard attacks = bishop_attacks_slow(sq, subset);
                int idx = int((subset * BishopMagics[s]) >> BishopShifts[s]);
                BishopTable[s][idx] = attacks;
                subset = next_subset(subset, mask);
            } while (subset);
        }

        // Rook
        {
            Bitboard mask = RookMasks[s];
            Bitboard subset = 0;
            do {
                Bitboard attacks = rook_attacks_slow(sq, subset);
                int idx = int((subset * RookMagics[s]) >> RookShifts[s]);
                RookTable[s][idx] = attacks;
                subset = next_subset(subset, mask);
            } while (subset);
        }
    }

    // BetweenBB and LineBB
    memset(BetweenBB, 0, sizeof(BetweenBB));
    memset(LineBB,    0, sizeof(LineBB));

    for (int s1 = 0; s1 < SQ_NB; ++s1) {
        for (int s2 = 0; s2 < SQ_NB; ++s2) {
            if (s1 == s2) continue;
            Square sq1 = Square(s1), sq2 = Square(s2);

            // Check if on same ray
            Bitboard b_attacks = bishop_attacks_slow(sq1, 0);
            Bitboard r_attacks = rook_attacks_slow(sq1, 0);

            if (b_attacks & sq_bb(sq2)) {
                BetweenBB[s1][s2] = bishop_attacks_slow(sq1, sq_bb(sq2))
                                   & bishop_attacks_slow(sq2, sq_bb(sq1));
                LineBB[s1][s2] = (bishop_attacks_slow(sq1, 0) & bishop_attacks_slow(sq2, 0))
                                 | sq_bb(sq1) | sq_bb(sq2);
            } else if (r_attacks & sq_bb(sq2)) {
                BetweenBB[s1][s2] = rook_attacks_slow(sq1, sq_bb(sq2))
                                   & rook_attacks_slow(sq2, sq_bb(sq1));
                LineBB[s1][s2] = (rook_attacks_slow(sq1, 0) & rook_attacks_slow(sq2, 0))
                                 | sq_bb(sq1) | sq_bb(sq2);
            }
        }
    }
}

void print_bb(Bitboard b) {
    for (int r = 7; r >= 0; --r) {
        for (int f = 0; f < 8; ++f)
            std::cout << ((b >> make_square(File(f), Rank(r))) & 1 ? '1' : '.');
        std::cout << '\n';
    }
    std::cout << '\n';
}
