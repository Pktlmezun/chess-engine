#pragma once
#include "types.h"
#include "move.h"
#include "bitboard.h"
#include <string>

static constexpr int MAX_GAME_PLY = 1024;

// Saved state for unmake_move
struct StateInfo {
    Square        ep_square;
    uint8_t       castling_rights;
    int           rule50;
    Piece         captured;
    uint64_t      hash;
};

class Board {
public:
    // ── Data (public for fast access in move gen / eval) ─────────────────────
    Piece     mailbox[SQ_NB];                    // piece on each square
    Bitboard  by_color[COLOR_NB];                // all pieces of each color
    Bitboard  by_type[PIECE_TYPE_NB];            // all pieces of each type (both colors)
    Color     side_to_move;
    int       game_ply;                          // half-moves from root

    StateInfo state_stack[MAX_GAME_PLY];
    int       state_idx;

    // Zobrist keys (initialized once)
    static uint64_t ZobristPiece[COLOR_NB][PIECE_TYPE_NB][SQ_NB];
    static uint64_t ZobristSide;
    static uint64_t ZobristCastle[16];
    static uint64_t ZobristEP[8];
    static bool     zobrist_initialized;

    // ── Interface ─────────────────────────────────────────────────────────────
    Board();
    void set_from_fen(const std::string& fen);
    std::string to_fen() const;
    void display() const;

    void make_move(Move m);
    void unmake_move(Move m);

    // ── Accessors ─────────────────────────────────────────────────────────────
    Piece    piece_on(Square s)          const { return mailbox[s]; }
    PieceType type_on(Square s)          const { return type_of(mailbox[s]); }
    Color    color_on(Square s)          const { return color_of(mailbox[s]); }
    Bitboard pieces(Color c)             const { return by_color[c]; }
    Bitboard pieces(PieceType pt)        const { return by_type[pt]; }
    Bitboard pieces(Color c, PieceType pt) const { return by_color[c] & by_type[pt]; }
    Bitboard all_pieces()                const { return by_color[WHITE] | by_color[BLACK]; }
    Square   king_square(Color c)        const { return lsb(pieces(c, KING)); }

    Square   ep_square()       const { return state_stack[state_idx].ep_square; }
    int      castling_rights() const { return state_stack[state_idx].castling_rights; }
    int      rule50()          const { return state_stack[state_idx].rule50; }
    uint64_t hash()            const { return state_stack[state_idx].hash; }

    bool can_castle(CastlingRight cr) const { return castling_rights() & cr; }

    // Attack queries
    Bitboard attackers_to(Square s, Bitboard occ) const;
    bool     is_attacked(Square s, Color by_color) const;
    bool     in_check()  const { return is_attacked(king_square(side_to_move), ~side_to_move); }

private:
    static void init_zobrist();
    // Hash-updating helpers (used in make_move)
    void put_piece(Color c, PieceType pt, Square s);
    void remove_piece(Square s);
    void move_piece(Square from, Square to);
    // No-hash helpers (used in unmake_move — hash is restored from state stack)
    void put_piece_nh(Color c, PieceType pt, Square s);
    void remove_piece_nh(Square s);
    void move_piece_nh(Square from, Square to);
};
