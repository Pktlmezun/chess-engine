#include "bitboard.h"
#include "board.h"
#include <iostream>

int main() {
    init_bitboards();

    Board b;
    b.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    b.display();

    std::cout << "FEN: " << b.to_fen() << "\n";
    std::cout << "Hash: 0x" << std::hex << b.hash() << std::dec << "\n";

    // Quick smoke test: e2e4, then unmake
    Move e2e4 = Move::make(E2, E4);
    std::cout << "\nAfter e2e4:\n";
    b.make_move(e2e4);
    b.display();

    std::cout << "After unmake:\n";
    b.unmake_move(e2e4);
    b.display();

    return 0;
}
