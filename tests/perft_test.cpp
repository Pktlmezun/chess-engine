#include "bitboard.h"
#include "board.h"
#include "movegen.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>

// ── Perft core
// ────────────────────────────────────────────────────────────────
static uint64_t perft(Board &b, int depth) {
  if (depth == 0)
    return 1;

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
static void divide(Board &b, int depth) {
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

// ── Test runner
// ───────────────────────────────────────────────────────────────
struct PerftTest {
  const char *name;
  const char *fen;
  int depth;
  uint64_t expected;
};

static const PerftTest TESTS[] = {
    // Standard perft positions (Chess Programming Wiki)
    {"Start pos d1", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
     1, 20},
    {"Start pos d2", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
     2, 400},
    {"Start pos d3", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
     3, 8902},
    {"Start pos d4", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
     4, 197281},
    {"Start pos d5", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
     5, 4865609},
    // Kiwipete (castling, en passant, promotions)
    {"Kiwipete  d1",
     "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 1, 48},
    {"Kiwipete  d2",
     "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 2,
     2039},
    {"Kiwipete  d3",
     "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 3,
     97862},
    {"Kiwipete  d4",
     "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 4,
     4085603},
    // Position 3 (en passant stress test)
    {"Pos3      d1", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -", 1, 14},
    {"Pos3      d2", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -", 2, 191},
    {"Pos3      d3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -", 3, 2812},
    {"Pos3      d4", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -", 4, 43238},
    {"Pos3      d5", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -", 5, 674624},
    // Position 4 (promotion and castling corner cases)
    {"Pos4      d1", "r3k2r/Pppp1ppp/1b6/8/8/1B6/pPPP1PPP/R3K2R w KQkq -", 1,
     27},
    {"Pos4      d2", "r3k2r/Pppp1ppp/1b6/8/8/1B6/pPPP1PPP/R3K2R w KQkq -", 2,
     725},
    {"Pos4      d3", "r3k2r/Pppp1ppp/1b6/8/8/1B6/pPPP1PPP/R3K2R w KQkq -", 3,
     19209},
    {"Pos4      d4", "r3k2r/Pppp1ppp/1b6/8/8/1B6/pPPP1PPP/R3K2R w KQkq -", 4,
     515398},
    // Position 5
    {"Pos5      d1", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -", 1,
     44},
    {"Pos5      d2", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -", 2,
     1486},
    {"Pos5      d3", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -", 3,
     62379},
    {"Pos5      d4", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -", 4,
     2103487},
    // Position 6
    {"Pos6      d1",
     "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P3/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
     1, 47},
    {"Pos6      d2",
     "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P3/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
     2, 1845},
    {"Pos6      d3",
     "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P3/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
     3, 81467},
    {"Pos6      d4",
     "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P3/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
     4, 3065277},
};

// ── Move serialization round-trip
// ──────────────────────────────────────────────
// Every generated move must survive to_string() -> parse() unchanged. A wrong
// promotion character here means the engine emits illegal moves to the GUI and
// silently rejects the opponent's promotions, desyncing its internal board.
static int roundtrip_walk(Board &b, int depth, int &failures) {
  Move list[256];
  int count = MoveGen::generate(b, list);
  int checked = 0;

  for (int i = 0; i < count; ++i) {
    Move m = list[i];
    std::string str = m.to_string();
    ++checked;

    // Validate the EXTERNAL format, not just the round trip. parse() compares
    // against to_string() output, so a round-trip check alone is tautological:
    // it passes even when to_string() emits a character no GUI accepts.
    const char *why = nullptr;
    bool is_promo = (m.type() == PROMOTION);
    size_t want_len = is_promo ? 5 : 4;

    if (str.size() != want_len)
      why = "wrong length";
    else if (str[0] < 'a' || str[0] > 'h' || str[2] < 'a' || str[2] > 'h')
      why = "bad file char";
    else if (str[1] < '1' || str[1] > '8' || str[3] < '1' || str[3] > '8')
      why = "bad rank char";
    else if (is_promo) {
      char c = str[4];
      PieceType want = m.promotion();
      char expect = (want == KNIGHT) ? 'n'
                  : (want == BISHOP) ? 'b'
                  : (want == ROOK)   ? 'r'
                  : (want == QUEEN)  ? 'q'
                                     : '?';
      if (c != expect)
        why = "wrong promotion char";
    }

    Move parsed = MoveGen::parse(b, str);
    if (!why && parsed != m)
      why = "parse() did not recover the move";

    if (why) {
      if (failures < 10) {
        std::cout << "  FAIL (" << why << "): from=" << int(m.from())
                  << " to=" << int(m.to()) << " type=" << int(m.type())
                  << " promo=" << int(m.promotion()) << "  to_string=\"" << str
                  << "\" len=" << str.size() << " bytes=[";
        for (size_t k = 0; k < str.size(); ++k)
          std::cout << int(static_cast<unsigned char>(str[k])) << (k + 1 < str.size() ? " " : "");
        std::cout << "]  reparsed="
                  << (parsed.is_null() ? std::string("NULL") : parsed.to_string())
                  << "\n    FEN: " << b.to_fen() << "\n";
      }
      ++failures;
    }

    if (depth > 1) {
      b.make_move(list[i]);
      if (!b.is_attacked(b.king_square(~b.side_to_move), b.side_to_move))
        checked += roundtrip_walk(b, depth - 1, failures);
      b.unmake_move(list[i]);
    }
  }
  return checked;
}

static bool run_roundtrip_tests() {
  std::cout << "\n── Move serialization round-trip ──\n";
  int total_failures = 0, total_checked = 0;

  for (const auto &t : TESTS) {
    if (t.depth != 3)
      continue; // one walk per distinct position is enough
    Board b;
    b.set_from_fen(t.fen);
    int failures = 0;
    int checked = roundtrip_walk(b, 3, failures);
    std::cout << (failures ? "FAIL" : "PASS") << "  " << std::left
              << std::setw(18) << t.name << "  checked=" << std::setw(10)
              << checked << "  failures=" << failures << "\n";
    total_failures += failures;
    total_checked += checked;
  }

  // Dedicated promotion position: every promotion piece, with and without capture.
  {
    Board b;
    b.set_from_fen("n7/1P6/8/8/8/8/6p1/K5nk w - - 0 1");
    int failures = 0;
    int checked = roundtrip_walk(b, 2, failures);
    std::cout << (failures ? "FAIL" : "PASS") << "  " << std::left
              << std::setw(18) << "Promotions" << "  checked=" << std::setw(10)
              << checked << "  failures=" << failures << "\n";
    total_failures += failures;
    total_checked += checked;
  }

  std::cout << "\n" << total_checked << " moves round-tripped, " << total_failures
            << " failures\n";
  return total_failures == 0;
}

static bool run_tests() {
  int passed = 0, failed = 0;
  for (const auto &t : TESTS) {
    Board b;
    b.set_from_fen(t.fen);
    auto t0 = std::chrono::steady_clock::now();
    uint64_t got = perft(b, t.depth);
    auto t1 = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    bool ok = (got == t.expected);
    std::cout << (ok ? "PASS" : "FAIL") << "  " << std::left << std::setw(18)
              << t.name << "  d" << t.depth << "  got=" << std::setw(10) << got
              << "  exp=" << std::setw(10) << t.expected << "  " << std::fixed
              << std::setprecision(1) << ms << "ms\n";
    ok ? ++passed : ++failed;
  }
  std::cout << "\n" << passed << "/" << (passed + failed) << " tests passed\n";
  return failed == 0;
}


// ── Opening book generator
// ────────────────────────────────────────────────────
// Emits EPD lines for cutechess-cli's -openings option. Without a book every
// game of a match is identical, so a match result carries almost no information.
static void gen_book(int count, int plies, unsigned seed) {
  uint64_t rng = seed ? seed : 1;
  auto next = [&rng]() {
    rng ^= rng << 13;
    rng ^= rng >> 7;
    rng ^= rng << 17;
    return rng;
  };

  int emitted = 0;
  while (emitted < count) {
    Board b;
    b.set_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    bool ok = true;

    for (int p = 0; p < plies; ++p) {
      Move list[256];
      int n = MoveGen::generate(b, list);
      // Keep only fully legal moves.
      Move legal[256];
      int ln = 0;
      for (int i = 0; i < n; ++i) {
        b.make_move(list[i]);
        if (!b.is_attacked(b.king_square(~b.side_to_move), b.side_to_move))
          legal[ln++] = list[i];
        b.unmake_move(list[i]);
      }
      if (ln == 0) { ok = false; break; }
      b.make_move(legal[next() % ln]);
    }

    if (!ok)
      continue;
    // Skip positions where someone is already in check: an unbalanced start.
    if (b.in_check())
      continue;

    std::cout << b.to_fen() << "\n";
    ++emitted;
  }
}

// ── main
// ──────────────────────────────────────────────────────────────────────
int main(int argc, char *argv[]) {
  init_bitboards();

  if (argc >= 2 && std::string(argv[1]) == "book") {
    // Usage: perft book [count] [plies] [seed]
    int count = (argc >= 3) ? std::stoi(argv[2]) : 50;
    int plies = (argc >= 4) ? std::stoi(argv[3]) : 8;
    unsigned seed = (argc >= 5) ? unsigned(std::stoul(argv[4])) : 20240926u;
    gen_book(count, plies, seed);
    return 0;
  }

  if (argc >= 2 && std::string(argv[1]) == "divide") {
    // Usage: perft divide <depth> [fen]
    int depth = (argc >= 3) ? std::stoi(argv[2]) : 1;
    std::string fen =
        (argc >= 4)
            ? std::string(argv[3])
            : "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    Board b;
    b.set_from_fen(fen);
    b.display();
    divide(b, depth);
    return 0;
  }

  // Default: run full test suite
  bool ok = run_tests();
  ok = run_roundtrip_tests() && ok;
  return ok ? 0 : 1;
}
