#include "board.h"
#include "movegen.h"
#include "bitboard.h"
#include <iostream>

int main() {
    init_bitboards();

    Board board;
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    // Check basic board state
    std::cout << "Side to move: " << (board.side_to_move == 0 ? "WHITE" : "BLACK") << std::endl;
    std::cout << "White pawns: 0x" << std::hex << board.pieces(WHITE, PAWN) << std::dec << std::endl;
    std::cout << "All pieces:  0x" << std::hex << board.all_pieces() << std::dec << std::endl;

    // Check if RankBB is initialized
    std::cout << "RankBB[0] (rank1): 0x" << std::hex << RankBB[0] << std::dec << std::endl;
    std::cout << "RankBB[1] (rank2): 0x" << std::hex << RankBB[1] << std::dec << std::endl;
    std::cout << "RankBB[2] (rank3): 0x" << std::hex << RankBB[2] << std::dec << std::endl;
    std::cout << "RankBB[7] (rank8): 0x" << std::hex << RankBB[7] << std::dec << std::endl;

    // Expected: RankBB[1] should be 0xFF00 (rank 2 = squares 8-15)
    std::cout << "Expected RankBB[1]: 0xff00" << std::endl;

    // Generate moves
    Move list[256];
    int count = MoveGen::generate(board, list);

    std::cout << "\nLegal moves (" << count << "):" << std::endl;
    for (int i = 0; i < count; ++i) {
        std::cout << "  " << list[i].to_string() << std::endl;
    }

    return 0;
}
