#include "board.h"
#include "movegen.h"
#include <iostream>
#include <string>
#include <vector>

int main() {
    Board board;
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    // Generate all legal moves
    Move list[256];
    int count = MoveGen::generate(board, list);

    std::cout << "Legal moves from startpos (" << count << "):" << std::endl;
    for (int i = 0; i < count; ++i) {
        std::cout << "  " << list[i].to_string() << " (data=0x" << std::hex << list[i].data << std::dec << ")" << std::endl;
    }

    // Try to find e2e4
    std::cout << "\nLooking for e2e4..." << std::endl;
    Move m = MoveGen::parse(board, "e2e4");
    if (m.is_null()) {
        std::cout << "NOT FOUND!" << std::endl;
        // Debug: check what squares are occupied
        std::cout << "Board state:" << std::endl;
        for (int sq = 0; sq < 64; ++sq) {
            Piece p = board.piece_on(Square(sq));
            if (p != NO_PIECE) {
                int f = sq % 8;
                int r = sq / 8;
                char c = 'a' + f;
                int rank = r + 1;
                std::cout << "  " << c << rank << ": piece=" << (int)p << std::endl;
            }
        }
    } else {
        std::cout << "FOUND: " << m.to_string() << std::endl;
    }

    return 0;
}
