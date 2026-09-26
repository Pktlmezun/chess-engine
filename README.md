<h1 align="center">♟ Chess Engine</h1>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17-blue?logo=cplusplus" alt="C++17">
  <img src="https://img.shields.io/badge/Protocol-UCI-green" alt="UCI">
  <img src="https://img.shields.io/badge/Search-Alpha--Beta-orange" alt="Alpha-Beta">
  <img src="https://img.shields.io/badge/Bitboards-Magic-red" alt="Magic Bitboards">
</p>

<p align="center">
  A chess engine written in C++17 with bitboard representation,<br>
  alpha-beta search, and UCI protocol support.
</p>

## Features

| Category | Feature |
|----------|---------|
| **Board** | Bitboard representation with mailbox fallback |
| **Attacks** | Magic bitboards for sliding pieces (O(1) lookup) |
| **Search** | Iterative deepening with Principal Variation Search (PVS) |
| **Pruning** | Alpha-beta, null move (R=3), futility (margin = 200×depth), late move reductions (depth −= 2) |
| **Eval** | Michniewski piece-square tables with tapered endgame |
| **TT** | Transposition table with depth-preferred replacement, 16 MB default |
| **Protocol** | Full UCI protocol for GUI integration |
| **Testing** | Perft test suite against known positions |

## Build

```bash
git clone https://github.com/Pktlmezun/chess-engine.git
cd chess-engine
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu 2>/dev/null || nproc)
```

**Requirements:**
- C++17 compiler (GCC 7+ / Clang 5+)
- CMake 3.16+
- POSIX threads

## Usage

Run the engine and connect it to any UCI-compatible GUI (Arena, CuteChess, BanksiaGUI):

```bash
./chess_engine
```

Example session:

```
uci
id name ChessEngine
id author pktlmezun
uciok
isready
readyok
position startpos
go movetime 5000
info depth 1 score cp 20 nodes 20 nps 10000 time 2 pv e2e4
info depth 2 score cp 35 nodes 400 nps 200000 time 2 pv e2e4 e7e5
...
bestmove e2e4
```

Perft from the command line:

```bash
./perft            # run the full test suite
./perft divide 5   # per-move node counts at depth 5
```

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         main.cpp                                │
│                        (UCI loop)                               │
└─────────────────────┬───────────────────────────────────────────┘
                      │
┌─────────────────────▼───────────────────────────────────────────┐
│                          uci.cpp                                │
│  ┌─────────┐  ┌──────────┐  ┌──────────┐  ┌──────────────┐    │
│  │ position │  │    go    │  │   stop   │  │  isready     │    │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └──────┬───────┘    │
└───────┼──────────────┼──────────────┼───────────────┼───────────┘
        │              │              │               │
┌───────▼──────────────▼──────────────▼───────────────▼───────────┐
│                       search.cpp                                │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │              Iterative Deepening Loop                    │    │
│  │  ┌─────────────────────────────────────────────────┐    │    │
│  │  │           Negamax + Alpha-Beta                   │    │    │
│  │  │  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────────────┐  │    │    │
│  │  │  │ TT   │ │ Null │ │ Futil│ │    LMR       │  │    │    │
│  │  │  │Probe │ │ Move │ │ Prune│ │  Reductions  │  │    │    │
│  │  │  └──────┘ └──────┘ └──────┘ └──────────────┘  │    │    │
│  │  └─────────────────────────────────────────────────┘    │    │
│  │  ┌─────────────────────────────────────────────────┐    │    │
│  │  │           Quiescence Search                     │    │    │
│  │  │         (captures only, stand pat)              │    │    │
│  │  └─────────────────────────────────────────────────┘    │    │
│  └─────────────────────────────────────────────────────────┘    │
└─────────────────────┬───────────────────────────────────────────┘
                      │
┌─────────────────────▼───────────────────────────────────────────┐
│                         eval.cpp                                │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐  │
│  │  Material    │  │  PST Tables  │  │  Threat Evaluation   │  │
│  │  Values      │  │  (MG + EG)   │  │  (hanging pieces)    │  │
│  └──────────────┘  └──────────────┘  └──────────────────────┘  │
└─────────────────────┬───────────────────────────────────────────┘
                      │
┌─────────────────────▼───────────────────────────────────────────┐
│                       board.cpp                                 │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐  │
│  │  Make/Unmake │  │  Zobrist     │  │  Attack Queries      │  │
│  │  Moves       │  │  Hashing     │  │  (is_attacked)       │  │
│  └──────────────┘  └──────────────┘  └──────────────────────┘  │
└─────────────────────┬───────────────────────────────────────────┘
                      │
┌─────────────────────▼───────────────────────────────────────────┐
│                      bitboard.cpp                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐  │
│  │  Pawn        │  │  Knight/King │  │  Magic Bitboards     │  │
│  │  Attacks     │  │  Attacks     │  │  (Bishop + Rook)     │  │
│  └──────────────┘  └──────────────┘  └──────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

## Evaluation

| Factor | Description |
|--------|-------------|
| **Material** | Standard piece values (P=100, N=320, B=330, R=500, Q=900, K=20000) |
| **PST** | Michniewski piece-square tables for positional understanding |
| **Phase** | Tapered eval blending midgame and endgame scores |
| **Threats** | Penalty for hanging pieces (attacked but undefended) |

Game phase is interpolated from remaining material:

```
Phase = Σ (piece_count × weight) / TOTAL_PHASE

Weights: Knight=1, Bishop=1, Rook=2, Queen=4
Total Phase = 24 (opening)
Phase 0     = endgame
```

## Testing

```bash
cd build
ctest --output-on-failure
```

The perft suite validates move generation against known node counts from the
[Chess Programming Wiki](https://www.chessprogramming.org/Perft_Results):

- Standard starting position (depth 1–5)
- Kiwipete (complex middlegame)
- Position 3 (en passant stress test)
- Position 4 (promotion + castling edge cases)
- Position 5 (asymmetric position)

`test_make_unmake` additionally checks that `make_move` / `unmake_move` round-trips
restore the board exactly, and `test_board_sync` verifies bitboard/mailbox agreement.

## Project Structure

```
chess-engine/
├── CMakeLists.txt              # Build configuration
├── src/
│   ├── main.cpp                # Entry point
│   ├── types.h                 # Core types (Square, Piece, Bitboard)
│   ├── bitboard.h/cpp          # Attack tables & magic bitboards
│   ├── board.h/cpp             # Board representation & move making
│   ├── move.h                  # Move encoding
│   ├── movegen.h/cpp           # Pseudo-legal move generation
│   ├── eval.h/cpp              # Position evaluation
│   ├── search.h/cpp            # Search algorithm + TT
│   └── uci.h/cpp               # UCI protocol handler
└── tests/
    ├── perft_test.cpp          # Perft validation suite
    ├── test_make_unmake.cpp    # Make/unmake round-trip assertions
    ├── test_board_sync.cpp     # Bitboard/mailbox consistency
    ├── test_game.cpp           # Full-game simulation harness
    ├── test_parse.cpp          # Move generation / parsing diagnostics
    ├── test_debug.cpp          # Board state dump
    └── test_sync.sh            # UCI replay scaffold (needs a `d` debug command)
```

## License

MIT — see [LICENSE](LICENSE).
