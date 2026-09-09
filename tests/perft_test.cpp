#include "bitboard.h"
#include "board.h"
#include "movegen.h"
#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>

// ── Perft core ────────────────────────────────────────────────────────────────
static uint64_t perft(Board& b, int depth) {
    if (depth == 0) return 1;

    Move list[256];
    int count = MoveGen::generate(b, list);
    uint64_t nodes = 0;

    for (int i = 0; i < count; ++i) {
        b.make_move(list[i]);
        // Filter pseudo-legal: skip if own king is now in check
        if (!b.is_attacked(b.king_square(~b.side_to_move), b.side_to_move))
            nodes += perft(b, depth - 1);
        b.unmake_move(list[i]);
    }
    return nodes;
}

// Divide: show nodes per root move (helps isolate bugs)
static void divide(Board& b, int depth) {
    Move list[256];
    int count = MoveGen::generate(b, list);
    uint64_t total = 0;

    for (int i = 0; i < count; ++i) {
        b.make_move(list[i]);
        if (!b.is_attacked(b.king_square(~b.side_to_move), b.side_to_move)) {
            uint64_t n = (depth > 1) ? perft(b, depth - 1) : 1;
            std::cout << list[i].to_string() << ": " << n << "\n";
            total += n;
        }
        b.unmake_move(list[i]);
    }
    std::cout << "\nTotal: " << total << "\n";
}

// ── Test runner ───────────────────────────────────────────────────────────────
struct PerftTest {
    const char* name;
    const char* fen;
    int         depth;
    uint64_t    expected;
};

static const PerftTest TESTS[] = {
    // Standard perft positions (Chess Programming Wiki)
    {"Start pos d1",  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",        1,        20},
    {"Start pos d2",  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",        2,       400},
    {"Start pos d3",  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",        3,      8902},
    {"Start pos d4",  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",        4,    197281},
    {"Start pos d5",  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",        5,   4865609},
    // Kiwipete (castling, en passant, promotions)
    {"Kiwipete  d1",  "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 1,       48},
    {"Kiwipete  d2",  "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 2,     2039},
    {"Kiwipete  d3",  "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 3,    97862},
    {"Kiwipete  d4",  "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 4,  4085603},
    // Position 3 (en passant stress test)
    {"Pos3      d1",  "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -",                             1,       14},
    {"Pos3      d2",  "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -",                             2,      191},
    {"Pos3      d3",  "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -",                             3,     2812},
    {"Pos3      d4",  "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -",                             4,    43238},
    {"Pos3      d5",  "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -",                             5,   674624},
    // Position 4 (promotion and castling corner cases)
    {"Pos4      d1",  "r3k2r/Pppp1ppp/1b6/8/8/1B6/pPPP1PPP/R3K2R w KQkq -",               1,        6},
    {"Pos4      d2",  "r3k2r/Pppp1ppp/1b6/8/8/1B6/pPPP1PPP/R3K2R w KQkq -",               2,      264},
    {"Pos4      d3",  "r3k2r/Pppp1ppp/1b6/8/8/1B6/pPPP1PPP/R3K2R w KQkq -",               3,     9467},
    {"Pos4      d4",  "r3k2r/Pppp1ppp/1b6/8/8/1B6/pPPP1PPP/R3K2R w KQkq -",               4,   422333},
    // Position 5
    {"Pos5      d1",  "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -",            1,       44},
    {"Pos5      d2",  "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -",            2,     1486},
    {"Pos5      d3",  "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -",            3,    62379},
    {"Pos5      d4",  "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -",            4,  2103487},
};

static void run_tests() {
    int passed = 0, failed = 0;
    for (const auto& t : TESTS) {
        Board b;
        b.set_from_fen(t.fen);
        auto t0 = std::chrono::steady_clock::now();
        uint64_t got = perft(b, t.depth);
        auto t1 = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        bool ok = (got == t.expected);
        std::cout << (ok ? "PASS" : "FAIL") << "  " << std::left << std::setw(18) << t.name
                  << "  d" << t.depth
                  << "  got=" << std::setw(10) << got
                  << "  exp=" << std::setw(10) << t.expected
                  << "  " << std::fixed << std::setprecision(1) << ms << "ms\n";
        ok ? ++passed : ++failed;
    }
    std::cout << "\n" << passed << "/" << (passed + failed) << " tests passed\n";
}

// ── main ──────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    init_bitboards();

    if (argc >= 2 && std::string(argv[1]) == "divide") {
        // Usage: perft divide <depth> [fen]
        int depth = (argc >= 3) ? std::stoi(argv[2]) : 1;
        std::string fen = (argc >= 4)
            ? std::string(argv[3])
            : "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        Board b;
        b.set_from_fen(fen);
        b.display();
        divide(b, depth);
        return 0;
    }

    // Default: run full test suite
    run_tests();
    return 0;
}
