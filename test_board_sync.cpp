#include "board.h"
#include "movegen.h"
#include "bitboard.h"
#include <iostream>
#include <string>
#include <vector>

int main() {
    init_bitboards();

    Board board;
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    // Test: apply moves and check FEN at each step
    std::vector<std::string> moves = {
        "e2e4", "e7e5",
        "g1f3", "b8c6",
        "f1b5", "a7a6",
        "b5a4", "g8f6",
        "e1g1", // castling
    };

    std::vector<std::string> expected_fens = {
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
        "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2",
        "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2",
        "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3",
        "r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3",
        "r1bqkbnr/1ppp1ppp/p1n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 4",
        "r1bqkbnr/1ppp1ppp/p1n5/4p3/B3P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 1 4",
        "r1bqkb1r/1ppp1ppp/p1n2n2/4p3/B3P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 2 5",
        "r1bqkb1r/1ppp1ppp/p1n2n2/4p3/B3P3/5N2/PPPP1PPP/RNBQ1RK1 b kq - 3 5",
    };

    std::string all_moves;
    for (size_t i = 0; i < moves.size(); ++i) {
        Move m = MoveGen::parse(board, moves[i]);
        if (m.is_null()) {
            std::cout << "FAIL: Could not parse move " << moves[i] << " at step " << i << std::endl;
            std::cout << "  Board FEN: " << board.to_fen() << std::endl;
            board.display();
            return 1;
        }
        board.make_move(m);

        std::string fen = board.to_fen();
        if (i < expected_fens.size() && fen != expected_fens[i]) {
            std::cout << "FAIL after move " << moves[i] << ":" << std::endl;
            std::cout << "  Expected: " << expected_fens[i] << std::endl;
            std::cout << "  Got:      " << fen << std::endl;
            board.display();
            return 1;
        }
        std::cout << "OK: " << moves[i] << " -> " << fen << std::endl;
    }

    std::cout << "\nAll tests passed!" << std::endl;
    return 0;
}
