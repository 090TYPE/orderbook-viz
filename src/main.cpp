#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <conio.h>   // _kbhit, _getch

#include "orderbook.hpp"
#include "display.hpp"
#include "binance_client.hpp"
#include "okx_client.hpp"
#include "bybit_client.hpp"

#include <atomic>
#include <csignal>
#include <memory>
#include <thread>
#include <chrono>
#include <cstring>
#include <cctype>
#include <string>

static std::atomic<bool> g_running{true};

static void handle_signal(int) {
    g_running.store(false);
}

// ── Symbol helpers ────────────────────────────────────────────────────────────

struct Symbols {
    std::string binance;   // e.g. "btcusdt"
    std::string okx;       // e.g. "BTC-USDT"
    std::string bybit;     // e.g. "BTCUSDT"
};

static Symbols make_symbols(const std::string& coin) {
    Symbols s;
    s.binance = coin + "usdt";
    for (auto& c : s.binance) c = static_cast<char>(std::tolower(c));
    s.okx   = coin + "-USDT";
    s.bybit = coin + "USDT";
    return s;
}

// ── Client bundle (owns clients, books are external) ─────────────────────────

struct ClientBundle {
    std::unique_ptr<BinanceClient> binance;
    std::unique_ptr<OKXClient>     okx;
    std::unique_ptr<BybitClient>   bybit;

    void start(OrderBook& bb, OrderBook& ob, OrderBook& yb,
               const Symbols& sym) {
        binance = std::make_unique<BinanceClient>(bb, sym.binance);
        okx     = std::make_unique<OKXClient>    (ob, sym.okx);
        bybit   = std::make_unique<BybitClient>  (yb, sym.bybit);
        binance->start();
        okx->start();
        bybit->start();
    }

    void stop() {
        if (binance) binance->stop();
        if (okx)     okx->stop();
        if (bybit)   bybit->stop();
        binance.reset();
        okx.reset();
        bybit.reset();
    }
};

// ── Interactive symbol input ──────────────────────────────────────────────────
// Reads a coin ticker from the user while the display is paused.
// Returns the new coin in UPPERCASE, or empty string if user cancelled (Esc).

static std::string prompt_symbol(const std::string& current) {
    // Show cursor and prompt
    printf("%s\n", ansi::SHOW_CURSOR);
    printf("  Current: %s/USDT\n", current.c_str());
    printf("  Enter new coin (e.g. ETH, SOL, DOGE) [Enter to confirm, Esc to cancel]: ");
    fflush(stdout);

    std::string result;

    while (true) {
        int ch = _getch();

        if (ch == 27) {          // Esc — cancel
            result.clear();
            break;
        }
        if (ch == '\r' || ch == '\n') {  // Enter — confirm
            break;
        }
        if ((ch == '\b' || ch == 127) && !result.empty()) {  // Backspace
            result.pop_back();
            printf("\b \b");
            fflush(stdout);
            continue;
        }
        if (std::isalpha(ch)) {
            char upper = static_cast<char>(std::toupper(ch));
            result += upper;
            putchar(upper);
            fflush(stdout);
        }
        // ignore digits and other chars
    }

    printf("\n%s", ansi::HIDE_CURSOR);
    fflush(stdout);
    return result;
}

// ── Entry point ───────────────────────────────────────────────────────────────

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

    // Optional symbol arg: ./orderbook_viz ETH  →  ETH/USDT
    std::string current_coin = "BTC";
    if (argc >= 2) {
        current_coin = argv[1];
        for (auto& c : current_coin) c = static_cast<char>(std::toupper(c));
    }

    // Order books persist across symbol switches
    OrderBook binance_book, okx_book, bybit_book;
    std::vector<OrderBook*> books = {&binance_book, &okx_book, &bybit_book};

    ClientBundle clients;
    clients.start(binance_book, okx_book, bybit_book, make_symbols(current_coin));

    Display display;

    while (g_running.load()) {
        // ── Non-blocking keyboard check ─────────────────────────────────────
        if (_kbhit()) {
            int ch = _getch();
            if (ch == 's' || ch == 'S') {
                // Pause rendering, ask for a new symbol
                std::string new_coin = prompt_symbol(current_coin);

                if (!new_coin.empty() && new_coin != current_coin) {
                    // Stop current streams
                    clients.stop();

                    // Reset order books
                    binance_book.reset();
                    okx_book.reset();
                    bybit_book.reset();

                    // Start new streams
                    current_coin = new_coin;
                    clients.start(binance_book, okx_book, bybit_book,
                                  make_symbols(current_coin));
                }
                // Force a clean redraw immediately
                continue;
            }
        }

        // ── Render ──────────────────────────────────────────────────────────
        display.render(books, 15, current_coin);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    clients.stop();

    printf("\n%s%sOrderbook visualizer stopped.\n%s",
           ansi::SHOW_CURSOR, ansi::RESET, ansi::RESET);
    return 0;
}
