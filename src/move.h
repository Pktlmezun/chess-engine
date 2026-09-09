#pragma once
#include "types.h"
#include <array>
#include <string>

// Move is packed into 32 bits:
//   bits  0-5  : from square
//   bits  6-11 : to square
//   bits 12-13 : MoveType
//   bits 14-16 : promotion PieceType (KNIGHT..QUEEN when type == PROMOTION)
struct Move {
    uint32_t data = 0;

    Move() = default;
    explicit Move(uint32_t d) : data(d) {}

    static Move make(Square from, Square to,
                     MoveType mt = NORMAL,
                     PieceType promo = NO_PIECE_TYPE) {
        return Move(uint32_t(from)
                  | uint32_t(to)   << 6
                  | uint32_t(mt)   << 12
                  | uint32_t(promo)<< 14);
    }

    Square    from()      const { return Square( data        & 0x3F); }
    Square    to()        const { return Square((data >> 6)  & 0x3F); }
    MoveType  type()      const { return MoveType((data >> 12) & 0x3); }
    PieceType promotion() const { return PieceType((data >> 14) & 0x7); }

    bool is_null()   const { return data == 0; }
    bool operator==(Move o) const { return data == o.data; }
    bool operator!=(Move o) const { return data != o.data; }

    static Move null() { return Move(0); }

    // Long algebraic notation: e2e4, e1g1, e7e8q
    std::string to_string() const;
};

struct MoveList {
    std::array<Move, 256> moves;
    int count = 0;

    void push(Move m) { moves[count++] = m; }
    Move* begin() { return moves.data(); }
    Move* end()   { return moves.data() + count; }
    const Move* begin() const { return moves.data(); }
    const Move* end()   const { return moves.data() + count; }
    int  size()   const { return count; }
};

inline std::string Move::to_string() const {
    if (is_null()) return "0000";
    static const char* sq_names[] = {
        "a1","b1","c1","d1","e1","f1","g1","h1",
        "a2","b2","c2","d2","e2","f2","g2","h2",
        "a3","b3","c3","d3","e3","f3","g3","h3",
        "a4","b4","c4","d4","e4","f4","g4","h4",
        "a5","b5","c5","d5","e5","f5","g5","h5",
        "a6","b6","c6","d6","e6","f6","g6","h6",
        "a7","b7","c7","d7","e7","f7","g7","h7",
        "a8","b8","c8","d8","e8","f8","g8","h8"
    };
    static const char promo_chars[] = " nbrq";
    std::string s = sq_names[from()];
    s += sq_names[to()];
    if (type() == PROMOTION)
        s += promo_chars[promotion()];
    return s;
}
