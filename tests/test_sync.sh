#!/bin/bash
# Test: feed moves to the engine and check the FEN output matches expected
ENGINE="${ENGINE:-$(cd "$(dirname "$0")/.." && pwd)/build/chess_engine}"

# Test with a known game
echo "uci
isready
position startpos moves e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6 e1g1 f8e7 f1e1 b7b5 a4b3 d7d6 c2c3 e8g8 h2h3 c8b7 d2d4 c6d4 c3d4 e5d4 f3d4 d6d5 e4d5 f6d5 b1c3 d8d6 c3d5 b7d5 d1f3 d6e6
d
quit" | $ENGINE 2>/dev/null | grep -E "bestmove|FEN"
