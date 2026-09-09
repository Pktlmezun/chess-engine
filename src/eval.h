#pragma once
#include "board.h"

// Returns evaluation in centipawns from White's perspective.
// Positive = White advantage, negative = Black advantage.
int evaluate(const Board& b);
