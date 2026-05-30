#pragma once
#include "orderbook.hpp"
#include "ws_client.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <chrono>

using json = nlohmann::json;

class OKXClient {
public:
    explicit OKXClient(OrderBook& book, std::string inst_id = "BTC-USDT")
        : book_(book), inst_id_(std::move(inst_id))
    {
        book_.exchange = "OKX";

        ws_.set_on_status([this](bool c) {
            std::lock_guard<std::mutex> lk(book_.mtx);
            book_.connected = c;
            if (!c) book_.snapshot_ready = false;
        });

        ws_.set_on_message([this](const std::string& msg) {
            handle(msg);
        });
    }

    void start() {
        ws_.start(L"ws.okx.com", 8443, L"/ws/v5/public");
        // Subscribe after connect — handled in on_status via a flag
        // WsClient calls on_status(true) then we need to subscribe.
        // We override on_status to also send subscription.
        ws_.set_on_status([this](bool c) {
            {
                std::lock_guard<std::mutex> lk(book_.mtx);
                book_.connected = c;
                if (!c) book_.snapshot_ready = false;
            }
            if (c) subscribe();
        });
    }

    void stop() { ws_.stop(); }

private:
    void subscribe() {
        json sub = {
            {"op", "subscribe"},
            {"args", json::array({{{"channel","books"},{"instId", inst_id_}}})}
        };
        ws_.send_text(sub.dump());
    }

    void handle(const std::string& raw) {
        try {
            auto j = json::parse(raw);
            if (!j.contains("data") || !j.contains("action")) return;

            std::string action = j["action"];
            auto& data = j["data"][0];

            std::lock_guard<std::mutex> lk(book_.mtx);
            if (action == "snapshot") { book_.bids.clear(); book_.asks.clear(); }

            for (auto& lv : data["bids"]) {
                double p = std::stod(lv[0].get<std::string>());
                double q = std::stod(lv[1].get<std::string>());
                book_.set_bid(p, q);
            }
            for (auto& lv : data["asks"]) {
                double p = std::stod(lv[0].get<std::string>());
                double q = std::stod(lv[1].get<std::string>());
                book_.set_ask(p, q);
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
    std::string inst_id_;
};
