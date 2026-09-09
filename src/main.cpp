#include "bitboard.h"
#include "board.h"
#include "eval.h"
#include "search.h"
#include "movegen.h"
#include <iostream>

int main() {
    init_bitboards();

    Board b;
    b.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    b.display();

    std::cout << "FEN: " << b.to_fen() << "\n";
    std::cout << "Hash: 0x" << std::hex << b.hash() << std::dec << "\n";

    // Debug: e2e4, d7d5, Bb5+ — check detection
    b.make_move(Move::make(E2, E4));
    b.make_move(Move::make(D7, D5));
    b.make_move(Move::make(F1, B5));  // Bb5+

    b.display();

    Square ks = b.king_square(b.side_to_move);
    std::cout << "Side to move: " << (b.side_to_move == WHITE ? "WHITE" : "BLACK") << "\n";
    std::cout << "King square:  " << int(ks) << " (expected 60 = e8)\n";

    Bitboard occ = b.all_pieces();
    std::cout << "All pieces (occupancy):\n"; print_bb(occ);
    Bitboard wb  = b.pieces(WHITE, BISHOP);
    std::cout << "White bishops:\n"; print_bb(wb);

    Bitboard mask_e8 = BishopMasks[ks];
    std::cout << "BishopMasks[e8]:\n"; print_bb(mask_e8);

    Bitboard masked_occ = occ & mask_e8;
    std::cout << "occ & BishopMasks[e8] (relevant blockers):\n"; print_bb(masked_occ);

    int idx = int((masked_occ * BishopMagics[ks]) >> BishopShifts[ks]);
    std::cout << "Magic index: " << idx << "\n";

    // Check all subsets of BishopMasks[e8] that collide at the same index
    std::cout << "Subsets of BishopMasks[e8] that hash to index " << idx << ":\n";
    Bitboard m = BishopMasks[ks];
    Bitboard sub = 0;
    do {
        int i = int((sub * BishopMagics[ks]) >> BishopShifts[ks]);
        if (i == idx) { std::cout << "  subset:\n"; print_bb(sub); }
        sub = (sub - m) & m;
    } while (sub);

    Bitboard diag = bishop_attacks(ks, occ);
    std::cout << "Bishop attacks from king square:\n"; print_bb(diag);

    std::cout << "bishop_attacks & white bishops:\n";
    print_bb(diag & wb);

    std::cout << "in_check: " << b.in_check() << "\n";

    // ── Evaluation sanity checks ──────────────────────────────────────────
    std::cout << "\n=== Evaluation Tests ===\n";

    // Startpos (White to move)
    Board b1;
    b1.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    int score1 = evaluate(b1);
    std::cout << "Startpos:           " << score1 << " cp (expected ≈ 0)\n";

    // Same position, Black to move — should flip sign
    Board b1b;
    b1b.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    int score1b = evaluate(b1b);
    std::cout << "Startpos (black):   " << score1b << " cp (expected ≈ 0)\n";

    // Missing white rook (a1)
    Board b2;
    b2.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/1NBQKBNR w KQkq - 0 1");
    int score2 = evaluate(b2);
    std::cout << "White rook missing: " << score2 << " cp (expected ≈ -500)\n";

    // Missing black queen
    Board b3;
    b3.set_from_fen("rnb1kbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    int score3 = evaluate(b3);
    std::cout << "Black queen missing:" << score3 << " cp (expected ≈ +900)\n";

    // Missing both knights for white
    Board b4;
    b4.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/R1BQKB1R w KQkq - 0 1");
    int score4 = evaluate(b4);
    std::cout << "White knights gone: " << score4 << " cp (expected ≈ -640)\n";

    // ── Search test: Mate in 1 (Qxf7#) ────────────────────────────────────
    std::cout << "\n=== Search Test: Mate in 1 (Qxf7#) ===\n";
    Board mate;
    mate.set_from_fen("r1bqkb1r/pppp1ppp/2n5/4p3/2B1P3/5Q2/PPPP1PPP/RNB1K1NR w KQkq -");
    mate.display();
    std::cout << "Searching...\n";
    Search::go(mate, 2);

    return 0;
}
