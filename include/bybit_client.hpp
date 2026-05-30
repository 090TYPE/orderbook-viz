#pragma once
#include "orderbook.hpp"
#include "ws_client.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <chrono>

using json = nlohmann::json;

class BybitClient {
public:
    explicit BybitClient(OrderBook& book, std::string symbol = "BTCUSDT")
        : book_(book), symbol_(std::move(symbol))
    {
        book_.exchange = "BYBIT";

        ws_.set_on_message([this](const std::string& msg) {
            handle(msg);
        });
    }

    void start() {
        ws_.set_on_status([this](bool c) {
            {
                std::lock_guard<std::mutex> lk(book_.mtx);
                book_.connected = c;
                if (!c) book_.snapshot_ready = false;
            }
            if (c) subscribe();
        });
        ws_.start(L"stream.bybit.com", 443, L"/v5/public/spot");
    }

    void stop() { ws_.stop(); }

private:
    void subscribe() {
        json sub = {
            {"op",   "subscribe"},
            {"args", json::array({"orderbook.50." + symbol_})}
        };
        ws_.send_text(sub.dump());
    }

    void handle(const std::string& raw) {
        try {
            auto j = json::parse(raw);
            if (!j.contains("data") || !j.contains("type")) return;

            std::string type = j["type"];
            auto& data = j["data"];

            std::lock_guard<std::mutex> lk(book_.mtx);
            if (type == "snapshot") { book_.bids.clear(); book_.asks.clear(); }

            if (data.contains("b")) {
                for (auto& lv : data["b"]) {
                    double p = std::stod(lv[0].get<std::string>());
                    double q = std::stod(lv[1].get<std::string>());
                    book_.set_bid(p, q);
                }
            }
            if (data.contains("a")) {
                for (auto& lv : data["a"]) {
                    double p = std::stod(lv[0].get<std::string>());
                    double q = std::stod(lv[1].get<std::string>());
                    book_.set_ask(p, q);
                }
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
    std::string symbol_;
};
