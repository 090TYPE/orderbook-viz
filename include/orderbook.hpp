#pragma once
#include <map>
#include <string>
#include <mutex>
#include <vector>
#include <chrono>

struct OrderBook {
    // bids: highest price first
    std::map<double, double, std::greater<double>> bids;
    // asks: lowest price first
    std::map<double, double> asks;

    std::string exchange;
    uint64_t last_update_ms = 0;
    bool connected        = false;
    bool snapshot_ready   = false;

    mutable std::mutex mtx;

    void set_bid(double price, double qty) {
        if (qty == 0.0) bids.erase(price);
        else            bids[price] = qty;
    }
    void set_ask(double price, double qty) {
        if (qty == 0.0) asks.erase(price);
        else            asks[price] = qty;
    }

    // Returns top N levels as {price, qty}
    std::vector<std::pair<double,double>> top_bids(int n) const {
        std::vector<std::pair<double,double>> out;
        int i = 0;
        for (auto& [p, q] : bids) { if (i++ >= n) break; out.emplace_back(p, q); }
        return out;
    }
    std::vector<std::pair<double,double>> top_asks(int n) const {
        std::vector<std::pair<double,double>> out;
        int i = 0;
        for (auto& [p, q] : asks) { if (i++ >= n) break; out.emplace_back(p, q); }
        return out;
    }

    double mid_price() const {
        if (bids.empty() || asks.empty()) return 0.0;
        return (bids.begin()->first + asks.begin()->first) / 2.0;
    }

    double spread() const {
        if (bids.empty() || asks.empty()) return 0.0;
        return asks.begin()->first - bids.begin()->first;
    }

    // Clear all data (call before switching symbol)
    void reset() {
        std::lock_guard<std::mutex> lk(mtx);
        bids.clear();
        asks.clear();
        connected      = false;
        snapshot_ready = false;
        last_update_ms = 0;
    }
};
