#include "board.h"
#include "movegen.h"
#include "bitboard.h"
#include <iostream>
#include <string>
#include <vector>
#include <cassert>

// Test: make_move then unmake_move should restore the board perfectly
bool test_make_unmake_roundtrip(Board& original, Move m, const std::string& label) {
    // Save original state
    std::string orig_fen = original.to_fen();
    uint64_t orig_hash = original.hash();
    Bitboard orig_white = original.pieces(WHITE);
    Bitboard orig_black = original.pieces(BLACK);
    Piece orig_mailbox[64];
    for (int i = 0; i < 64; ++i)
        orig_mailbox[i] = original.piece_on(Square(i));

    // Make the move
    original.make_move(m);

    // Unmake the move
    original.unmake_move(m);

    // Check if board is restored
    bool ok = true;
    if (original.to_fen() != orig_fen) {
        std::cerr << "FAIL FEN " << label << ": " << m.to_string() << "\n";
        std::cerr << "  Before: " << orig_fen << "\n";
        std::cerr << "  After:  " << original.to_fen() << "\n";
        ok = false;
    }
    if (original.hash() != orig_hash) {
        std::cerr << "FAIL HASH " << label << ": " << m.to_string() << "\n";
        std::cerr << "  Before: " << orig_hash << "\n";
        std::cerr << "  After:  " << original.hash() << "\n";
        ok = false;
    }
    if (original.pieces(WHITE) != orig_white || original.pieces(BLACK) != orig_black) {
        std::cerr << "FAIL BITBOARD " << label << ": " << m.to_string() << "\n";
        ok = false;
    }
    for (int i = 0; i < 64; ++i) {
        if (original.piece_on(Square(i)) != orig_mailbox[i]) {
            std::cerr << "FAIL MAILBOX " << label << ": " << m.to_string()
                      << " at square " << i << "\n";
            ok = false;
            break;
        }
    }
    if (!ok) {
        std::cerr << "  Position: " << orig_fen << "\n";
        original.display();
    }
    return ok;
}

int main() {
    init_bitboards();

    // Test positions including tricky ones
    std::vector<std::string> test_fens = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1",        // Castling both sides
        "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2", // En passant
        "rnbqkb1r/pppppppp/5n2/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 1 2",
        "r1bqkbnr/pppppppp/2n5/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 2 3",
        "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3",
        "r1bqkb1r/pppppppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", // Complex
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",                    // Endgame
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 b kq - 0 1",
        "rnbq1k1r/pppp1ppp/5n2/2b1p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQ - 4 4",
    };

    int total_moves_tested = 0;
    int total_failures = 0;

    for (auto& fen : test_fens) {
        Board board;
        board.set_from_fen(fen);

        Move list[256];
        int count = MoveGen::generate(board, list);

        std::cout << "Testing FEN: " << fen.substr(0, 40) << "... (" << count << " moves)" << std::endl;

        for (int i = 0; i < count; ++i) {
            if (!test_make_unmake_roundtrip(board, list[i], "single")) {
                total_failures++;
            }
            total_moves_tested++;
        }
    }

    // Test double make/unmake (make A, make B, unmake B, unmake A)
    std::cout << "\nTesting double make/unmake..." << std::endl;
    Board board;
    board.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    Move list1[256];
    int count1 = MoveGen::generate(board, list1);
    for (int i = 0; i < count1 && i < 5; ++i) {
        board.make_move(list1[i]);
        std::string fen_after_first = board.to_fen();

        Move list2[256];
        int count2 = MoveGen::generate(board, list2);
        for (int j = 0; j < count2; ++j) {
            board.make_move(list2[j]);
            board.unmake_move(list2[j]);

            if (!test_make_unmake_roundtrip(board, list1[i], "double-inner")) {
                total_failures++;
            }
            total_moves_tested++;
        }
        board.unmake_move(list1[i]);

        if (board.to_fen() != "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1") {
            std::cerr << "FAIL: Board not restored after double make/unmake!" << std::endl;
            total_failures++;
        }
    }

    std::cout << "\n" << total_moves_tested << " moves tested, " << total_failures << " failures" << std::endl;
    return total_failures > 0 ? 1 : 0;
}
