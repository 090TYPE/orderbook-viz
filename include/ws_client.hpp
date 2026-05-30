#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

#include "orderbook.hpp"
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <vector>
#include <stdexcept>

// WinHTTP native WebSocket client — no external TLS deps, WSS works out of the box
class WsClient {
public:
    using MessageCb = std::function<void(const std::string&)>;
    using StatusCb  = std::function<void(bool connected)>;

    WsClient() = default;
    ~WsClient() { stop(); }

    void set_on_message(MessageCb cb) { on_message_ = std::move(cb); }
    void set_on_status (StatusCb  cb) { on_status_  = std::move(cb); }

    // host e.g. L"stream.binance.com", port e.g. 9443, path e.g. L"/ws/btcusdt@depth20@100ms"
    void start(const std::wstring& host, INTERNET_PORT port, const std::wstring& path) {
        host_ = host; port_ = port; path_ = path;
        running_ = true;
        thread_ = std::thread([this]{ loop(); });
    }

    void stop() {
        running_ = false;
        // Close socket to unblock receive
        if (hws_) { WinHttpWebSocketClose(hws_, WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, nullptr, 0); }
        if (thread_.joinable()) thread_.join();
        cleanup();
    }

    bool send_text(const std::string& msg) {
        if (!hws_) return false;
        DWORD r = WinHttpWebSocketSend(hws_,
            WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
            (PVOID)msg.data(), (DWORD)msg.size());
        return r == ERROR_SUCCESS;
    }

private:
    void loop() {
        while (running_) {
            if (!connect()) {
                if (on_status_) on_status_(false);
                Sleep(3000);
                continue;
            }
            if (on_status_) on_status_(true);
            receive_loop();
            if (on_status_) on_status_(false);
            cleanup();
            if (running_) Sleep(2000); // reconnect delay
        }
    }

    bool connect() {
        cleanup();

        hsession_ = WinHttpOpen(L"OrderbookViz/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hsession_) return false;

        // 10s connect + 30s receive timeouts
        DWORD timeout = 10000;
        WinHttpSetOption(hsession_, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
        timeout = 30000;
        WinHttpSetOption(hsession_, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

        hconnect_ = WinHttpConnect(hsession_, host_.c_str(), port_, 0);
        if (!hconnect_) return false;

        DWORD flags = WINHTTP_FLAG_SECURE;
        hrequest_ = WinHttpOpenRequest(hconnect_, L"GET", path_.c_str(),
            nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
        if (!hrequest_) return false;

        // Request WebSocket upgrade
        if (!WinHttpSetOption(hrequest_, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, nullptr, 0))
            return false;

        if (!WinHttpSendRequest(hrequest_, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
            return false;

        if (!WinHttpReceiveResponse(hrequest_, nullptr))
            return false;

        hws_ = WinHttpWebSocketCompleteUpgrade(hrequest_, 0);
        return hws_ != nullptr;
    }

    void receive_loop() {
        std::vector<BYTE> buf(128 * 1024);
        std::string frame_data;

        while (running_) {
            DWORD bytes_read = 0;
            WINHTTP_WEB_SOCKET_BUFFER_TYPE buf_type;

            DWORD r = WinHttpWebSocketReceive(hws_,
                buf.data(), (DWORD)buf.size(), &bytes_read, &buf_type);

            if (r != ERROR_SUCCESS) break;

            if (buf_type == WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE) {
                frame_data.append((char*)buf.data(), bytes_read);
            } else if (buf_type == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE) {
                frame_data.append((char*)buf.data(), bytes_read);
                if (on_message_) on_message_(frame_data);
                frame_data.clear();
            } else if (buf_type == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE) {
                break;
            }
        }
    }

    void cleanup() {
        if (hws_)      { WinHttpCloseHandle(hws_);      hws_      = nullptr; }
        if (hrequest_) { WinHttpCloseHandle(hrequest_); hrequest_ = nullptr; }
        if (hconnect_) { WinHttpCloseHandle(hconnect_); hconnect_ = nullptr; }
        if (hsession_) { WinHttpCloseHandle(hsession_); hsession_ = nullptr; }
    }

    std::wstring  host_;
    INTERNET_PORT port_ = 443;
    std::wstring  path_;

    HINTERNET hsession_ = nullptr;
    HINTERNET hconnect_ = nullptr;
    HINTERNET hrequest_ = nullptr;
    HINTERNET hws_      = nullptr;

    std::thread   thread_;
    std::atomic<bool> running_{false};
    MessageCb on_message_;
    StatusCb  on_status_;
};
