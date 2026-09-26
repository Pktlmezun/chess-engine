#include "board.h"
#include "movegen.h"
#include "search.h"
#include "bitboard.h"
#include <iostream>
#include <string>
#include <vector>

// Simulate a cutechess game and check if engine outputs match expected
int main() {
    init_bitboards();

    Board board;
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    // Simulate: engine plays as White against itself
    for (int move_num = 0; move_num < 10; ++move_num) {
        // Generate legal moves
        Move legal[256];
        int legal_count = MoveGen::generate(board, legal);

        if (legal_count == 0) {
            std::cout << "No legal moves at move " << move_num << std::endl;
            break;
        }

        // Search for best move
        SearchLimits limits{};
        limits.depth = 4;
        Search::go(board, limits);

        // Get the validated best move
        Move best = Search::get_last_bestmove();
        if (best.is_null()) {
            std::cout << "ERROR: null bestmove at move " << move_num << std::endl;
            break;
        }

        // Check if bestmove is in legal list
        bool found = false;
        for (int i = 0; i < legal_count; ++i) {
            if (legal[i] == best) { found = true; break; }
        }

        std::string fen = board.to_fen();
        std::string move_str = best.to_string();

        if (!found) {
            std::cout << "ILLEGAL MOVE at move " << move_num << ": " << move_str << std::endl;
            std::cout << "  FEN: " << fen << std::endl;
            std::cout << "  Legal moves:";
            for (int i = 0; i < legal_count; ++i)
                std::cout << " " << legal[i].to_string();
            std::cout << std::endl;
            board.display();
            return 1;
        }

        std::cout << "Move " << move_num << ": " << move_str << " (FEN: " << fen << ")" << std::endl;

        // Apply the move
        board.make_move(best);
    }

    std::cout << "\nAll moves were legal!" << std::endl;
    return 0;
}
