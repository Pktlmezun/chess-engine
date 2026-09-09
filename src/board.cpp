#include "board.h"
#include <sstream>
#include <iostream>
#include <cstring>
#include <random>
#include <cassert>

// ── Static member definitions ─────────────────────────────────────────────────
uint64_t Board::ZobristPiece[COLOR_NB][PIECE_TYPE_NB][SQ_NB];
uint64_t Board::ZobristSide;
uint64_t Board::ZobristCastle[16];
uint64_t Board::ZobristEP[8];
bool     Board::zobrist_initialized = false;

void Board::init_zobrist() {
    std::mt19937_64 rng(0xDEADBEEFCAFEBABEULL);
    for (int c = 0; c < COLOR_NB; ++c)
        for (int pt = 0; pt < PIECE_TYPE_NB; ++pt)
            for (int s = 0; s < SQ_NB; ++s)
                ZobristPiece[c][pt][s] = rng();
    ZobristSide = rng();
    for (int i = 0; i < 16; ++i) ZobristCastle[i] = rng();
    for (int i = 0; i < 8;  ++i) ZobristEP[i]     = rng();
    zobrist_initialized = true;
}

// ── Piece helpers ─────────────────────────────────────────────────────────────
void Board::put_piece(Color c, PieceType pt, Square s) {
    Piece p = make_piece(c, pt);
    mailbox[s]  = p;
    by_color[c] |= sq_bb(s);
    by_type[pt] |= sq_bb(s);
    state_stack[state_idx].hash ^= ZobristPiece[c][pt][s];
}

void Board::remove_piece(Square s) {
    Piece p  = mailbox[s];
    Color c  = color_of(p);
    PieceType pt = type_of(p);
    mailbox[s]  = NO_PIECE;
    by_color[c] &= ~sq_bb(s);
    by_type[pt] &= ~sq_bb(s);
    state_stack[state_idx].hash ^= ZobristPiece[c][pt][s];
}

void Board::move_piece(Square from, Square to) {
    Piece p  = mailbox[from];
    Color c  = color_of(p);
    PieceType pt = type_of(p);
    mailbox[from]  = NO_PIECE;
    mailbox[to]    = p;
    Bitboard mask  = sq_bb(from) | sq_bb(to);
    by_color[c]   ^= mask;
    by_type[pt]   ^= mask;
    state_stack[state_idx].hash ^= ZobristPiece[c][pt][from] ^ ZobristPiece[c][pt][to];
}

// ── Constructor ───────────────────────────────────────────────────────────────
Board::Board() {
    if (!zobrist_initialized) init_zobrist();
    memset(mailbox, NO_PIECE, sizeof(mailbox));
    memset(by_color, 0, sizeof(by_color));
    memset(by_type,  0, sizeof(by_type));
    side_to_move = WHITE;
    game_ply = 0;
    state_idx = 0;
    state_stack[0] = {SQ_NONE, NO_CASTLING, 0, NO_PIECE, 0};
}

// ── FEN parsing ───────────────────────────────────────────────────────────────
void Board::set_from_fen(const std::string& fen) {
    // Reset
    memset(mailbox, NO_PIECE, sizeof(mailbox));
    memset(by_color, 0, sizeof(by_color));
    memset(by_type,  0, sizeof(by_type));
    state_idx = 0;
    game_ply  = 0;
    StateInfo& st = state_stack[0];
    st = {SQ_NONE, NO_CASTLING, 0, NO_PIECE, 0};

    std::istringstream ss(fen);
    std::string token;

    // Piece placement
    ss >> token;
    int rank = 7, file = 0;
    for (char c : token) {
        if (c == '/') { --rank; file = 0; }
        else if (c >= '1' && c <= '8') { file += c - '0'; }
        else {
            Color col  = (c >= 'a') ? BLACK : WHITE;
            char lc    = (c >= 'a') ? c : c + 32;
            PieceType pt;
            switch (lc) {
                case 'p': pt = PAWN;   break;
                case 'n': pt = KNIGHT; break;
                case 'b': pt = BISHOP; break;
                case 'r': pt = ROOK;   break;
                case 'q': pt = QUEEN;  break;
                case 'k': pt = KING;   break;
                default:  pt = NO_PIECE_TYPE; break;
            }
            if (pt != NO_PIECE_TYPE)
                put_piece(col, pt, make_square(File(file), Rank(rank)));
            ++file;
        }
    }

    // Side to move
    ss >> token;
    side_to_move = (token == "b") ? BLACK : WHITE;
    if (side_to_move == BLACK) st.hash ^= ZobristSide;

    // Castling rights
    ss >> token;
    st.castling_rights = NO_CASTLING;
    for (char c : token) {
        if (c == 'K') st.castling_rights |= WHITE_OO;
        if (c == 'Q') st.castling_rights |= WHITE_OOO;
        if (c == 'k') st.castling_rights |= BLACK_OO;
        if (c == 'q') st.castling_rights |= BLACK_OOO;
    }
    st.hash ^= ZobristCastle[st.castling_rights];

    // En passant
    ss >> token;
    if (token != "-") {
        File f = File(token[0] - 'a');
        Rank r = Rank(token[1] - '1');
        st.ep_square = make_square(f, r);
        st.hash ^= ZobristEP[f];
    } else {
        st.ep_square = SQ_NONE;
    }

    // Half-move clock
    int hmove = 0;
    ss >> hmove;
    st.rule50 = hmove;
}

// ── make_move ────────────────────────────────────────────────────────────────
void Board::make_move(Move m) {
    // Copy state forward
    StateInfo& prev = state_stack[state_idx];
    StateInfo& cur  = state_stack[++state_idx];
    cur.hash            = prev.hash;
    cur.castling_rights = prev.castling_rights;
    cur.rule50          = prev.rule50 + 1;
    cur.ep_square       = SQ_NONE;
    cur.captured        = NO_PIECE;

    Square from    = m.from();
    Square to      = m.to();
    MoveType mtype = m.type();
    Piece    moved = mailbox[from];
    Color    us    = side_to_move;
    Color    them  = ~us;

    // Remove old ep hash contribution
    if (prev.ep_square != SQ_NONE)
        cur.hash ^= ZobristEP[file_of(prev.ep_square)];

    // Remove old castling hash contribution
    cur.hash ^= ZobristCastle[prev.castling_rights];

    // Handle captures (normal capture or en passant)
    if (mtype == EN_PASSANT) {
        Square cap_sq = (us == WHITE) ? Square(to - 8) : Square(to + 8);
        cur.captured  = mailbox[cap_sq];
        remove_piece(cap_sq);
        cur.rule50 = 0;
    } else if (mailbox[to] != NO_PIECE) {
        cur.captured = mailbox[to];
        remove_piece(to);
        cur.rule50 = 0;
    }

    // Pawn move resets rule50
    if (type_of(moved) == PAWN) cur.rule50 = 0;

    // Set en passant square for double pawn push
    if (type_of(moved) == PAWN && abs(int(to) - int(from)) == 16) {
        cur.ep_square = Square((int(from) + int(to)) / 2);
        cur.hash ^= ZobristEP[file_of(cur.ep_square)];
    }

    // Move the piece
    if (mtype == PROMOTION) {
        remove_piece(from);
        put_piece(us, m.promotion(), to);
    } else if (mtype == CASTLING) {
        // King move
        move_piece(from, to);
        // Rook move
        Square rook_from, rook_to;
        if (to > from) { // kingside
            rook_from = (us == WHITE) ? H1 : H8;
            rook_to   = (us == WHITE) ? F1 : F8;
        } else {         // queenside
            rook_from = (us == WHITE) ? A1 : A8;
            rook_to   = (us == WHITE) ? D1 : D8;
        }
        move_piece(rook_from, rook_to);
    } else {
        move_piece(from, to);
    }

    // Update castling rights
    auto castling_mask = [](Square s) -> uint8_t {
        switch (s) {
            case A1: return uint8_t(~WHITE_OOO);
            case H1: return uint8_t(~WHITE_OO);
            case E1: return uint8_t(~(WHITE_OO | WHITE_OOO));
            case A8: return uint8_t(~BLACK_OOO);
            case H8: return uint8_t(~BLACK_OO);
            case E8: return uint8_t(~(BLACK_OO | BLACK_OOO));
            default: return 0xFF;
        }
    };
    uint8_t cr = cur.castling_rights;
    cr &= castling_mask(from);
    cr &= castling_mask(to);
    cur.castling_rights = cr;
    cur.hash ^= ZobristCastle[cr];

    // Flip side
    side_to_move = them;
    cur.hash ^= ZobristSide;
    ++game_ply;
}

// ── Low-level piece helpers that do NOT touch the hash ───────────────────────
// Used during unmake_move since the hash is restored from the stack.
void Board::put_piece_nh(Color c, PieceType pt, Square s) {
    mailbox[s]  = make_piece(c, pt);
    by_color[c] |= sq_bb(s);
    by_type[pt] |= sq_bb(s);
}

void Board::remove_piece_nh(Square s) {
    Piece p = mailbox[s];
    by_color[color_of(p)] &= ~sq_bb(s);
    by_type[type_of(p)]   &= ~sq_bb(s);
    mailbox[s] = NO_PIECE;
}

void Board::move_piece_nh(Square from, Square to) {
    Piece p = mailbox[from];
    Bitboard mask = sq_bb(from) | sq_bb(to);
    by_color[color_of(p)] ^= mask;
    by_type[type_of(p)]   ^= mask;
    mailbox[to]   = p;
    mailbox[from] = NO_PIECE;
}

// ── unmake_move ──────────────────────────────────────────────────────────────
void Board::unmake_move(Move m) {
    --game_ply;
    side_to_move = ~side_to_move;
    Color us   = side_to_move;
    Color them = ~us;

    Square from    = m.from();
    Square to      = m.to();
    MoveType mtype = m.type();

    // Restore the saved state (hash included) FIRST, then fix piece positions.
    StateInfo& cur = state_stack[state_idx--];

    if (mtype == PROMOTION) {
        remove_piece_nh(to);
        put_piece_nh(us, PAWN, from);
    } else if (mtype == CASTLING) {
        move_piece_nh(to, from);
        Square rook_from, rook_to;
        if (to > from) { // kingside
            rook_from = (us == WHITE) ? H1 : H8;
            rook_to   = (us == WHITE) ? F1 : F8;
        } else {         // queenside
            rook_from = (us == WHITE) ? A1 : A8;
            rook_to   = (us == WHITE) ? D1 : D8;
        }
        move_piece_nh(rook_to, rook_from);
    } else {
        move_piece_nh(to, from);
    }

    // Restore captured piece
    if (cur.captured != NO_PIECE) {
        if (mtype == EN_PASSANT) {
            Square cap_sq = (us == WHITE) ? Square(to - 8) : Square(to + 8);
            put_piece_nh(them, type_of(cur.captured), cap_sq);
        } else {
            put_piece_nh(them, type_of(cur.captured), to);
        }
    }
}

// ── Attack queries ────────────────────────────────────────────────────────────
Bitboard Board::attackers_to(Square s, Bitboard occ) const {
    return (PawnAttacks[BLACK][s]    & pieces(WHITE, PAWN))
         | (PawnAttacks[WHITE][s]    & pieces(BLACK, PAWN))
         | (KnightAttacks[s]         & by_type[KNIGHT])
         | (KingAttacks[s]           & by_type[KING])
         | (bishop_attacks(s, occ)   & (by_type[BISHOP] | by_type[QUEEN]))
         | (rook_attacks(s, occ)     & (by_type[ROOK]   | by_type[QUEEN]));
}

bool Board::is_attacked(Square s, Color by) const {
    Bitboard occ = all_pieces();
    return (PawnAttacks[~by][s]        & pieces(by, PAWN))
         | (KnightAttacks[s]           & pieces(by, KNIGHT))
         | (KingAttacks[s]             & pieces(by, KING))
         | (bishop_attacks(s, occ)     & (pieces(by, BISHOP) | pieces(by, QUEEN)))
         | (rook_attacks(s, occ)       & (pieces(by, ROOK)   | pieces(by, QUEEN)));
}

// ── Display ──────────────────────────────────────────────────────────────────
void Board::display() const {
    static const char piece_chars[] = " PNBRQKxxpnbrqk";
    std::cout << "\n  +---+---+---+---+---+---+---+---+\n";
    for (int r = 7; r >= 0; --r) {
        std::cout << r + 1 << " |";
        for (int f = 0; f < 8; ++f) {
            Piece p = mailbox[make_square(File(f), Rank(r))];
            char c  = (p == NO_PIECE) ? ' ' : piece_chars[p];
            std::cout << ' ' << c << " |";
        }
        std::cout << "\n  +---+---+---+---+---+---+---+---+\n";
    }
    std::cout << "    a   b   c   d   e   f   g   h\n\n";
    std::cout << "Side: " << (side_to_move == WHITE ? "White" : "Black") << "\n";
    std::cout << "Hash: 0x" << std::hex << hash() << std::dec << "\n\n";
}

// ── FEN output ────────────────────────────────────────────────────────────────
std::string Board::to_fen() const {
    static const char pc[] = " PNBRQKxxpnbrqk";
    std::string fen;
    for (int r = 7; r >= 0; --r) {
        int empty = 0;
        for (int f = 0; f < 8; ++f) {
            Piece p = mailbox[make_square(File(f), Rank(r))];
            if (p == NO_PIECE) { ++empty; }
            else {
                if (empty) { fen += char('0' + empty); empty = 0; }
                fen += pc[p];
            }
        }
        if (empty) fen += char('0' + empty);
        if (r > 0) fen += '/';
    }
    fen += ' ';
    fen += (side_to_move == WHITE ? 'w' : 'b');
    fen += ' ';
    int cr = castling_rights();
    if (!cr) fen += '-';
    else {
        if (cr & WHITE_OO)  fen += 'K';
        if (cr & WHITE_OOO) fen += 'Q';
        if (cr & BLACK_OO)  fen += 'k';
        if (cr & BLACK_OOO) fen += 'q';
    }
    fen += ' ';
    Square ep = ep_square();
    if (ep == SQ_NONE) fen += '-';
    else {
        fen += char('a' + file_of(ep));
        fen += char('1' + rank_of(ep));
    }
    fen += ' ';
    fen += std::to_string(rule50());
    fen += ' ';
    fen += std::to_string(1 + game_ply / 2);
    return fen;
}
