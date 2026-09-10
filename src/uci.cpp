#include "uci.h"
#include "bitboard.h"
#include "movegen.h"
#include <iostream>
#include <sstream>

Board      UCI::board;
std::thread UCI::search_thread;

static const char* STARTPOS = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

void UCI::send(const std::string& msg) {
    std::lock_guard<std::mutex> lock(Search::output_mutex);
    std::cout << msg << "\n";
    std::cout.flush();
}

void UCI::stop_search() {
    Search::stop();
    if (search_thread.joinable())
        search_thread.join();
}

void UCI::handle_position(std::istringstream& iss) {
    stop_search();

    std::string token;
    iss >> token;

    if (token == "startpos") {
        board.set_from_fen(STARTPOS);
        iss >> token; // try to consume "moves"
    } else if (token == "fen") {
        std::string fen;
        while (iss >> token && token != "moves") {
            if (!fen.empty()) fen += " ";
            fen += token;
        }
        board.set_from_fen(fen);
    }

    if (token == "moves") {
        while (iss >> token) {
            Move m = MoveGen::parse(board, token);
            if (m.is_null()) {
                std::lock_guard<std::mutex> lock(Search::output_mutex);
                std::cerr << "info string invalid move: " << token << "\n";
                break;
            }
            board.make_move(m);
        }
    }
}

void UCI::handle_go(std::istringstream& iss) {
    stop_search();

    SearchLimits limits{};
    std::string token;

    while (iss >> token) {
        if (token == "depth")       iss >> limits.depth;
        else if (token == "wtime")  iss >> limits.wtime;
        else if (token == "btime")  iss >> limits.btime;
        else if (token == "winc")   iss >> limits.winc;
        else if (token == "binc")   iss >> limits.binc;
        else if (token == "movetime") iss >> limits.movetime;
        else if (token == "infinite") limits.infinite = true;
    }

    Search::stopped.store(false, std::memory_order_relaxed);
    Board board_copy = board;

    search_thread = std::thread([board_copy, limits]() mutable {
        Search::go(board_copy, limits);
    });
}

void UCI::loop() {
    init_bitboards();
    board.set_from_fen(STARTPOS);

    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string token;
        iss >> token;

        if (token == "uci") {
            send("id name ChessEngine");
            send("id author OpenCode");
            send("uciok");
        } else if (token == "isready") {
            send("readyok");
        } else if (token == "ucinewgame") {
            stop_search();
            Search::clear_tables();
        } else if (token == "position") {
            handle_position(iss);
        } else if (token == "go") {
            handle_go(iss);
        } else if (token == "stop") {
            stop_search();
        } else if (token == "quit") {
            stop_search();
            break;
        }
    }

    if (search_thread.joinable())
        search_thread.join();
}
