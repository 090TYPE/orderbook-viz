#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "orderbook.hpp"
#include "display.hpp"
#include "binance_client.hpp"
#include "okx_client.hpp"
#include "bybit_client.hpp"

#include <atomic>
#include <csignal>
#include <thread>
#include <chrono>
#include <cstring>
#include <cctype>

static std::atomic<bool> g_running{true};

static void handle_signal(int) {
    g_running.store(false);
}

int main(int argc, char* argv[]) {
    // Enable UTF-8 + ANSI on Windows console
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    std::signal(SIGINT,  handle_signal);
    std::signal(SIGTERM, handle_signal);

    // Optional symbol arg: ./orderbook_viz ETH → ETH/USDT
    std::string coin = "BTC";
    if (argc >= 2) {
        coin = argv[1];
        for (auto& c : coin) c = static_cast<char>(std::toupper(c));
    }

    std::string binance_sym = coin + "usdt";
    for (auto& c : binance_sym) c = static_cast<char>(std::tolower(c));
    std::string okx_sym   = coin + "-USDT";
    std::string bybit_sym = coin + "USDT";

    OrderBook binance_book, okx_book, bybit_book;

    BinanceClient binance(binance_book, binance_sym);
    OKXClient     okx    (okx_book,     okx_sym);
    BybitClient   bybit  (bybit_book,   bybit_sym);

    binance.start();
    okx.start();
    bybit.start();

    Display display;
    std::vector<OrderBook*> books = {&binance_book, &okx_book, &bybit_book};

    while (g_running.load()) {
        display.render(books, 15);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    binance.stop();
    okx.stop();
    bybit.stop();

    printf("\n%s%sOrderbook visualizer stopped.\n%s",
           ansi::SHOW_CURSOR, ansi::RESET, ansi::RESET);
    return 0;
}
