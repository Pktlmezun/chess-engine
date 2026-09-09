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
    0x00601102020c0022ULL, 0x0002380804808000ULL, 0x8816080101048100ULL, 0x0004040880406000ULL, 0x40a4242004500902ULL, 0x0081042084410010ULL, 0x0014809010505000ULL, 0x0228202808341014ULL,
    0x0010202042424040ULL, 0x0210210401220024ULL, 0x0000260825030200ULL, 0x040344040a904080ULL, 0x4420040420010488ULL, 0xc000610108410a00ULL, 0x402800410808c000ULL, 0x0000004208040200ULL,
    0x18080421081000c0ULL, 0x00210a0204010200ULL, 0xd001081004138250ULL, 0x024800008200400bULL, 0x0202000420210010ULL, 0x0041000210020120ULL, 0x0084000049245028ULL, 0x1000820042009081ULL,
    0x4a08c01104100248ULL, 0x020110c204040800ULL, 0x2081010050140020ULL, 0x2001004014040002ULL, 0x0081001101004001ULL, 0x0008080820808401ULL, 0x02a0812000841000ULL, 0x840a002402048a08ULL,
    0x0030108400090800ULL, 0x0400848412101004ULL, 0x000c020102020401ULL, 0x2011080800120a00ULL, 0x2040002020020080ULL, 0x1201012201c10040ULL, 0x180a840d0124108aULL, 0x04848e0842088400ULL,
    0x102484444008a210ULL, 0x010202035c002101ULL, 0x0040882808001402ULL, 0x0225120124000200ULL, 0x000040110a020101ULL, 0x0002220041000200ULL, 0x8222020206048410ULL, 0x0008080100424424ULL,
    0x0401041004040820ULL, 0x0000820082a04000ULL, 0x0010003402080048ULL, 0x0004001184240000ULL, 0x0000001202020000ULL, 0x0000102001410508ULL, 0x8104840404040000ULL, 0x0030410214044200ULL,
    0x0002008848125040ULL, 0x0048812404442430ULL, 0x8002200025180840ULL, 0x0000005000840401ULL, 0x0000000012202210ULL, 0x40c0414018014111ULL, 0x0000640848080090ULL, 0x0028020408420e00ULL
};

const Bitboard RookMagics[SQ_NB] = {
    0x2080002080104000ULL, 0x0040001000200044ULL, 0x0880200188900080ULL, 0x2480041000080080ULL, 0x1180020800802400ULL, 0x0300040001001208ULL, 0x1080120001004080ULL, 0x0100012240860100ULL,
    0x2212800080204008ULL, 0x8004400020100244ULL, 0x0801001100482000ULL, 0x0025000900201000ULL, 0x0008808008000400ULL, 0x1222808024000200ULL, 0x00c4006104081002ULL, 0x20820010a1020444ULL,
    0x0080004020004000ULL, 0x8000810021004000ULL, 0x1110008010802002ULL, 0x0000848010000800ULL, 0x8522020020081004ULL, 0x2000808004000201ULL, 0x1061240001080230ULL, 0x00a2020004208041ULL,
    0x0590800100204109ULL, 0x0028410a00220080ULL, 0x0000410100102000ULL, 0x0030100080800800ULL, 0x8000040080080082ULL, 0x1000040080800200ULL, 0x0100100c00080a21ULL, 0x4000019a00024401ULL,
    0x0080004000402000ULL, 0x0180401000c02000ULL, 0x0004200501001440ULL, 0x0800100021000904ULL, 0x2000040081802800ULL, 0x0040800400800200ULL, 0x1250100214002811ULL, 0x2880290046001094ULL,
    0x0210802040008000ULL, 0x0000200050024000ULL, 0x4890008220048010ULL, 0x00020021400a0010ULL, 0x0124008008008005ULL, 0x0002001804060010ULL, 0x0c0a004801820004ULL, 0x0000008100420004ULL,
    0x4908800100205500ULL, 0x0008804000200a80ULL, 0x4020001020490100ULL, 0x2028080010008080ULL, 0x0200040080080080ULL, 0x4803000804000300ULL, 0x40a4020810010400ULL, 0x0322040452890200ULL,
    0x9020488002702101ULL, 0x00070180c0009021ULL, 0x1408204008820012ULL, 0x0010040810010021ULL, 0x1106000408102002ULL, 0x000b002400188211ULL, 0x010001082200b004ULL, 0x0000a04408830066ULL
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
