#pragma once
#include <cstdint>
#include <cassert>

// ── Colors ──────────────────────────────────────────────────────────────────
enum Color : uint8_t { WHITE, BLACK, COLOR_NB = 2 };
constexpr Color operator~(Color c) { return Color(c ^ 1); }

// ── Piece types ──────────────────────────────────────────────────────────────
enum PieceType : uint8_t {
    NO_PIECE_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
    PIECE_TYPE_NB = 7
};

// ── Pieces (color + type packed) ─────────────────────────────────────────────
enum Piece : uint8_t {
    NO_PIECE = 0,
    W_PAWN = 1, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN = 9, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
    PIECE_NB = 16
};

constexpr Piece make_piece(Color c, PieceType pt) {
    return Piece((c << 3) | pt);
}
constexpr PieceType type_of(Piece p) { return PieceType(p & 7); }
constexpr Color     color_of(Piece p) { return Color(p >> 3); }

// ── Squares ──────────────────────────────────────────────────────────────────
enum Square : uint8_t {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    SQ_NONE = 64, SQ_NB = 64
};

enum File : uint8_t { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H };
enum Rank : uint8_t { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8 };

constexpr Square make_square(File f, Rank r) { return Square(r * 8 + f); }
constexpr File   file_of(Square s)           { return File(s & 7); }
constexpr Rank   rank_of(Square s)           { return Rank(s >> 3); }
constexpr Square flip(Square s)              { return Square(s ^ 56); }

inline Square& operator++(Square& s) { return s = Square(int(s) + 1); }

// ── Bitboard ─────────────────────────────────────────────────────────────────
using Bitboard = uint64_t;

constexpr Bitboard sq_bb(Square s) { return Bitboard(1) << s; }

// ── Directions ───────────────────────────────────────────────────────────────
enum Direction : int {
    NORTH =  8, SOUTH = -8,
    EAST  =  1, WEST  = -1,
    NE = 9, NW = 7, SE = -7, SW = -9
};

// ── Castling rights ───────────────────────────────────────────────────────────
enum CastlingRight : uint8_t {
    NO_CASTLING  = 0,
    WHITE_OO     = 1,   // kingside white
    WHITE_OOO    = 2,   // queenside white
    BLACK_OO     = 4,   // kingside black
    BLACK_OOO    = 8,   // queenside black
    ANY_CASTLING = 15
};

// ── Move types ────────────────────────────────────────────────────────────────
enum MoveType : uint8_t {
    NORMAL, CASTLING, EN_PASSANT, PROMOTION
};

// ── Material values (centipawns) ──────────────────────────────────────────────
constexpr int PieceValue[PIECE_TYPE_NB] = { 0, 100, 320, 330, 500, 900, 20000 };

// ── Score constants ───────────────────────────────────────────────────────────
constexpr int SCORE_INF      = 1'000'000;
constexpr int SCORE_MATE     =   999'000;
constexpr int SCORE_NONE     = -SCORE_INF - 1;
