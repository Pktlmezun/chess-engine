# ♟ Chess Engine

A high-performance chess engine written in C++17 with bitboard representation, alpha-beta search, and UCI protocol support.

```
╔══════════════════════════════════════════════════════════════════╗
║  ┌───┬───┬───┬───┬───┬───┬───┬───┐                            ║
║  │ r │   │   │   │ k │   │   │ r │  ← Magic Bitboards         ║
║  ├───┼───┼───┼───┼───┼───┼───┼───┤  ← Alpha-Beta Pruning      ║
║  │ p │   │ p │ p │ q │ p │ b │   │  ← Transposition Table      ║
║  ├───┼───┼───┼───┼───┼───┼───┼───┤  ← Null Move Pruning       ║
║  │   │ b │ n │   │ p │ n │ p │   │  ← Late Move Reductions     ║
║  ├───┼───┼───┼───┼───┼───┼───┼───┤  ← Quiescence Search       ║
║  │   │   │   │ P │ N │   │   │   │  ← PST Evaluation           ║
║  ├───┼───┼───┼───┼───┼───┼───┼───┤                            ║
║  │   │ p │   │   │ P │   │   │   │  UCI Protocol               ║
║  ├───┼───┼───┼───┼───┼───┼───┼───┤  C++17 / CMake             ║
║  │ P │ P │ P │ B │ P │   │ Q │ p │                            ║
║  ├───┼───┼───┼───┼───┼───┼───┼───┤  ~200KN/s @ 3.8GHz        ║
║  │ R │   │   │   │ K │   │   │ R │                            ║
║  └───┴───┴───┴───┴───┴───┴───┴───┘                            ║
╚══════════════════════════════════════════════════════════════════╝
```

## Features

| Category | Feature |
|----------|---------|
| **Board** | Bitboard representation with mailbox fallback |
| **Attacks** | Magic bitboards for sliding pieces (O(1) lookup) |
| **Search** | Iterative deepening with Principal Variation Search (PVS) |
| **Pruning** | Alpha-beta, null move, futility, late move reductions |
| **Eval** | Michniewski piece-square tables with tapered endgame |
| **TT** | Transposition table with depth-preferred replacement |
| **Protocol** | Full UCI protocol for GUI integration |
| **Testing** | Perft test suite against known positions |

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

## Build

```bash
# Clone and build
git clone https://github.com/Pktlmezun/chess-engine.git
cd chess-engine
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

**Requirements:**
- C++17 compiler (GCC 7+ / Clang 5+)
- CMake 3.16+
- POSIX threads

## Usage

### UCI Mode (with GUI)

```bash
./chess_engine
```

Connect to any UCI-compatible GUI (Arena, CuteChess, BanksiaGUI, etc.):

```
uci
isready
position startpos
go depth 20
```

### Command Line

```bash
# Run perft tests
./perft

# Run perft divide (debugging)
./perft divide 5
```

### Example UCI Session

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

## Search Techniques

```
                    ┌─────────────────┐
                    │  Iterative      │
                    │  Deepening      │
                    └────────┬────────┘
                             │
                    ┌────────▼────────┐
                    │  Negamax +      │
                    │  Alpha-Beta     │
                    └────────┬────────┘
                             │
         ┌───────────────────┼───────────────────┐
         │                   │                   │
┌────────▼────────┐ ┌────────▼────────┐ ┌────────▼────────┐
│  Null Move      │ │  Futility       │ │  Late Move      │
│  Pruning        │ │  Pruning        │ │  Reductions     │
│  (R=3)          │ │  (margin=200*d) │ │  (depth -= 2)   │
└─────────────────┘ └─────────────────┘ └─────────────────┘
         │                   │                   │
         └───────────────────┼───────────────────┘
                             │
                    ┌────────▼────────┐
                    │  Quiescence     │
                    │  Search         │
                    └─────────────────┘
```

## Evaluation

The evaluation function combines multiple factors:

| Factor | Description |
|--------|-------------|
| **Material** | Standard piece values (P=100, N=320, B=330, R=500, Q=900, K=20000) |
| **PST** | Michniewski piece-square tables for positional understanding |
| **Phase** | Tapered eval blending midgame and endgame scores |
| **Threats** | Penalty for hanging pieces (attacked but undefended) |

### Game Phase Calculation

```
Phase = Σ (piece_count × weight) / TOTAL_PHASE

Weights: Knight=1, Bishop=1, Rook=2, Queen=4
Total Phase = 24 (opening)
Phase 0     = endgame
```

## Testing

The engine includes a comprehensive perft test suite:

```bash
cd build
./perft
```

**Test positions:**
- Standard starting position (depth 1-5)
- Kiwipete (complex middlegame)
- Position 3 (en passant stress test)
- Position 4 (promotion + castling edge cases)
- Position 5 (asymmetric position)

All tests validate against known node counts from the [Chess Programming Wiki](https://www.chessprogramming.org/Perft_Results).

## Project Structure

```
chess-engine/
├── CMakeLists.txt          # Build configuration
├── src/
│   ├── main.cpp            # Entry point
│   ├── types.h             # Core types (Square, Piece, Bitboard)
│   ├── bitboard.h/cpp      # Attack tables & magic bitboards
│   ├── board.h/cpp         # Board representation & move making
│   ├── move.h              # Move encoding
│   ├── movegen.h/cpp       # Pseudo-legal move generation
│   ├── eval.h/cpp          # Position evaluation
│   ├── search.h/cpp        # Search algorithm + TT
│   └── uci.h/cpp           # UCI protocol handler
├── tests/
│   └── perft_test.cpp      # Perft validation suite
└── cutechess-cli           # Testing GUI binary
```

## Performance

Typical search speeds on modern hardware:

| Metric | Value |
|--------|-------|
| Nodes/second | ~200K NPS |
| TT Size | 16 MB default |
| Max Depth | 128 ply |
| Time Control | Supports wtime/btime/movetime |

## License

MIT

---

*Built with ❤️ and bitboards*
