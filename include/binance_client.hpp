#pragma once
#include "orderbook.hpp"
#include "ws_client.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <chrono>

using json = nlohmann::json;

class BinanceClient {
public:
    explicit BinanceClient(OrderBook& book, std::string symbol = "btcusdt")
        : book_(book)
    {
        book_.exchange = "BINANCE";

        ws_.set_on_status([this](bool c) {
            std::lock_guard<std::mutex> lk(book_.mtx);
            book_.connected = c;
            if (!c) book_.snapshot_ready = false;
        });

        ws_.set_on_message([this](const std::string& msg) {
            handle(msg);
        });

        // Convert to lowercase wstring for URL
        std::wstring wsym(symbol.begin(), symbol.end());
        path_ = L"/ws/" + wsym + L"@depth20@100ms";
    }

    void start() {
        // stream.binance.com port 9443
        ws_.start(L"stream.binance.com", 9443, path_);
    }
    void stop() { ws_.stop(); }

private:
    void handle(const std::string& raw) {
        try {
            auto j = json::parse(raw);
            std::lock_guard<std::mutex> lk(book_.mtx);
            book_.bids.clear();
            book_.asks.clear();

            for (auto& lv : j["bids"]) {
                double p = std::stod(lv[0].get<std::string>());
                double q = std::stod(lv[1].get<std::string>());
                if (q > 0) book_.bids[p] = q;
            }
            for (auto& lv : j["asks"]) {
                double p = std::stod(lv[0].get<std::string>());
                double q = std::stod(lv[1].get<std::string>());
                if (q > 0) book_.asks[p] = q;
            }
            book_.last_update_ms = ts_now();
            book_.snapshot_ready = true;
        } catch (...) {}
    }

    static uint64_t ts_now() {
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
    }

    OrderBook& book_;
    WsClient   ws_;
    std::wstring path_;
};
