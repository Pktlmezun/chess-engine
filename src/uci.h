#pragma once
#include "board.h"
#include "search.h"
#include <thread>
#include <atomic>
#include <mutex>

class UCI {
public:
    static void loop();

private:
    static Board board;
    static std::thread search_thread;

    static void handle_position(std::istringstream& iss);
    static void handle_go(std::istringstream& iss);
    static void stop_search();
    static void send(const std::string& msg);
};
