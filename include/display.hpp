#pragma once
// Must define NOMINMAX before any Windows header to prevent max/min macro conflicts
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "orderbook.hpp"
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cstdio>
#include <cmath>
#include <algorithm>

// ANSI color codes
namespace ansi {
    constexpr const char* RESET       = "\033[0m";
    constexpr const char* BOLD        = "\033[1m";
    constexpr const char* RED         = "\033[91m";
    constexpr const char* GREEN       = "\033[92m";
    constexpr const char* YELLOW      = "\033[93m";
    constexpr const char* CYAN        = "\033[96m";
    constexpr const char* WHITE       = "\033[97m";
    constexpr const char* GRAY        = "\033[90m";
    constexpr const char* BG_DARK     = "\033[40m";
    constexpr const char* CLEAR       = "\033[2J\033[H";
    constexpr const char* HIDE_CURSOR = "\033[?25l";
    constexpr const char* SHOW_CURSOR = "\033[?25h";
}

// UTF-8 block char for bar: U+2588 FULL BLOCK = "\xe2\x96\x88"
static const char* BLOCK = "\xe2\x96\x88";

static std::string make_bar(double fraction, int max_width = 10) {
    int filled = static_cast<int>(std::round(fraction * max_width));
    filled = (std::min)(filled, max_width);
    filled = (std::max)(filled, 0);
    std::string s;
    s.reserve(filled * 3 + (max_width - filled));
    for (int i = 0; i < filled; i++)          s += BLOCK;
    for (int i = filled; i < max_width; i++)  s += ' ';
    return s;
}

static std::string fmt_price(double p, int width = 11) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << std::setw(width) << p;
    return ss.str();
}

static std::string fmt_qty(double q, int width = 10) {
    std::ostringstream ss;
    if (q >= 1000.0)
        ss << std::fixed << std::setprecision(2) << std::setw(width) << q;
    else
        ss << std::fixed << std::setprecision(4) << std::setw(width) << q;
    return ss.str();
}

static std::string fmt_float(double v, int prec, int width) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(prec) << std::setw(width) << v;
    return ss.str();
}

// Render one exchange column
static std::vector<std::string> render_book(const OrderBook& book, int depth) {
    std::vector<std::string> lines;

    std::lock_guard<std::mutex> lk(book.mtx);

    // Header
    {
        std::string status = book.connected
            ? (std::string(ansi::GREEN) + "\xe2\x97\x8f LIVE")   // ●
            : (std::string(ansi::RED)   + "\xe2\x97\x8b DISC");  // ○
        std::ostringstream h;
        h << ansi::BOLD << ansi::CYAN
          << std::left << std::setw(12) << book.exchange
          << ansi::RESET << "  " << status << ansi::RESET;
        lines.push_back(h.str());
    }

    lines.push_back(std::string(ansi::GRAY) +
        "  Price(USDT)   Qty       Volume      " + ansi::RESET);
    lines.push_back(std::string(ansi::GRAY) + std::string(38, '-') + ansi::RESET);

    if (!book.snapshot_ready) {
        for (int i = 0; i < depth * 2 + 3; i++)
            lines.push_back(std::string(ansi::GRAY) + "  Waiting for data..." + ansi::RESET);
        return lines;
    }

    auto top_asks = book.top_asks(depth);
    auto top_bids = book.top_bids(depth);

    double max_vol = 1.0;
    for (auto& [p, q] : top_asks) max_vol = (std::max)(max_vol, p * q);
    for (auto& [p, q] : top_bids) max_vol = (std::max)(max_vol, p * q);

    // Asks reversed (highest first → lowest near mid)
    for (int i = static_cast<int>(top_asks.size()) - 1; i >= 0; i--) {
        auto [p, q] = top_asks[i];
        double vol = p * q;
        std::string bar = make_bar(vol / max_vol, 8);
        std::ostringstream row;
        row << ansi::RED
            << fmt_price(p) << "  " << fmt_qty(q)
            << "  " << fmt_float(vol, 0, 8) << "  "
            << ansi::BG_DARK << ansi::RED << bar << ansi::RESET;
        lines.push_back(row.str());
    }

    // Mid / spread
    {
        double mid  = book.mid_price();
        double sprd = book.spread();
        std::ostringstream ml;
        ml << ansi::BOLD << ansi::YELLOW
           << "  MID " << fmt_price(mid)
           << "  SPR " << std::fixed << std::setprecision(2) << sprd
           << ansi::RESET;
        lines.push_back(ml.str());
    }

    // Bids
    for (auto& [p, q] : top_bids) {
        double vol = p * q;
        std::string bar = make_bar(vol / max_vol, 8);
        std::ostringstream row;
        row << ansi::GREEN
            << fmt_price(p) << "  " << fmt_qty(q)
            << "  " << fmt_float(vol, 0, 8) << "  "
            << ansi::BG_DARK << ansi::GREEN << bar << ansi::RESET;
        lines.push_back(row.str());
    }

    return lines;
}

class Display {
public:
    Display() {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        GetConsoleMode(h, &mode);
        SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        printf("%s%s", ansi::HIDE_CURSOR, ansi::CLEAR);
    }

    ~Display() {
        printf("%s", ansi::SHOW_CURSOR);
    }

    void render(const std::vector<OrderBook*>& books, int depth,
                const std::string& coin = "BTC") {
        std::vector<std::vector<std::string>> cols;
        for (auto* b : books)
            cols.push_back(render_book(*b, depth));

        size_t max_rows = 0;
        for (auto& c : cols) max_rows = (std::max)(max_rows, c.size());

        std::ostringstream out;
        out << ansi::CLEAR;

        // Build dynamic title, padded to fixed width
        std::string pair = coin + "/USDT";
        std::string title_core = "CRYPTO ORDERBOOK VISUALIZER  [ " + pair + " ]  -  Binance / OKX / Bybit";
        // Total inner width = 84 chars
        const int INNER = 84;
        int pad = INNER - static_cast<int>(title_core.size());
        int lpad = (std::max)(0, pad / 2);
        int rpad = (std::max)(0, pad - lpad);
        std::string title_line = "  |" + std::string(lpad, ' ') + title_core
                               + std::string(rpad, ' ') + "|";

        out << ansi::BOLD << ansi::WHITE
            << "  +======================================================================================"
               "==+\n"
            << title_line << "\n"
            << "  +======================================================================================"
               "==+\n"
            << ansi::RESET << "\n";

        for (size_t r = 0; r < max_rows; r++) {
            out << "  ";
            for (size_t c = 0; c < cols.size(); c++) {
                std::string cell = r < cols[c].size() ? cols[c][r] : "";
                out << cell;
                out << "   |   ";
            }
            out << "\n";
        }

        out << "\n" << ansi::GRAY
            << "  [S] switch symbol  |  Ctrl+C to exit  |  Depth: " << depth
            << "  |  Refresh: 100ms  |  WinHTTP WSS"
            << ansi::RESET << "\n";

        printf("%s", out.str().c_str());
        fflush(stdout);
    }
};
