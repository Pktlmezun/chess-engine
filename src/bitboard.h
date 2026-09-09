#pragma once
#include "types.h"

// ── Compiler intrinsics ───────────────────────────────────────────────────────
inline int    popcount(Bitboard b) { return __builtin_popcountll(b); }
inline Square lsb(Bitboard b)      { return Square(__builtin_ctzll(b)); }
inline Square msb(Bitboard b)      { return Square(63 ^ __builtin_clzll(b)); }

inline Square pop_lsb(Bitboard& b) {
    Square s = lsb(b);
    b &= b - 1;
    return s;
}

// ── Pre-computed attack tables ────────────────────────────────────────────────
extern Bitboard PawnAttacks[COLOR_NB][SQ_NB];
extern Bitboard KnightAttacks[SQ_NB];
extern Bitboard KingAttacks[SQ_NB];

// Magic bitboard tables for sliding pieces
extern Bitboard BishopMasks[SQ_NB];
extern Bitboard RookMasks[SQ_NB];
extern Bitboard BishopTable[SQ_NB][512];
extern Bitboard RookTable[SQ_NB][4096];

extern const Bitboard BishopMagics[SQ_NB];
extern const Bitboard RookMagics[SQ_NB];
extern const int      BishopShifts[SQ_NB];
extern const int      RookShifts[SQ_NB];

// Ray / connectivity tables
extern Bitboard BetweenBB[SQ_NB][SQ_NB];
extern Bitboard LineBB[SQ_NB][SQ_NB];
extern Bitboard FileBB[8];
extern Bitboard RankBB[8];
extern Bitboard AdjacentFilesBB[8];

void init_bitboards();

// ── Sliding piece attack getters (O(1) magic lookup) ─────────────────────────
inline Bitboard bishop_attacks(Square s, Bitboard occ) {
    occ &= BishopMasks[s];
    occ  = (occ * BishopMagics[s]) >> BishopShifts[s];
    return BishopTable[s][occ];
}

inline Bitboard rook_attacks(Square s, Bitboard occ) {
    occ &= RookMasks[s];
    occ  = (occ * RookMagics[s]) >> RookShifts[s];
    return RookTable[s][occ];
}

inline Bitboard queen_attacks(Square s, Bitboard occ) {
    return bishop_attacks(s, occ) | rook_attacks(s, occ);
}

// ── Debug ─────────────────────────────────────────────────────────────────────
void print_bb(Bitboard b);
