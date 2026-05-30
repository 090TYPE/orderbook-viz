# 📊 Orderbook Visualizer

Real-time cryptocurrency orderbook visualizer in the terminal — streams live bid/ask data from **Binance**, **OKX**, and **Bybit** simultaneously.

```
  +====================================================================================+
  |        CRYPTO ORDERBOOK VISUALIZER  [ BTC/USDT ]  -  Binance / OKX / Bybit       |
  +====================================================================================+

  BINANCE  ● LIVE        |   OKX  ● LIVE         |   BYBIT  ● LIVE
  Price(USDT)  Qty   Vol |   Price(USDT)  Qty Vol |   Price(USDT)  Qty   Vol
  ---------------------- |   -------------------- |   ----------------------
  95442.10  0.1200  11453 ████░░░░  |  ...
  95440.00  0.5500  52492 ██████████|  ...
  MID 95438.50  SPR 0.20
  95437.00  1.2000 114524 ██████████|  ...
  95435.10  0.3300  31493 ████░░░░░░|  ...

  Ctrl+C to exit  |  Depth: 15  |  Refresh: 100ms  |  WinHTTP WSS
```

## Features

- **3 exchanges side-by-side** — Binance, OKX, Bybit
- **Live WebSocket streams** — 100ms refresh
- **Color-coded** — asks in red, bids in green
- **Volume bars** — visual depth chart per level
- **Mid price & spread** — shown between asks and bids
- **Auto-reconnect** — handles disconnects silently
- **Any symbol** — BTC, ETH, SOL, etc. via CLI argument
- **Zero external dependencies** — uses Windows native WinHTTP for WSS

## Requirements

- Windows 10 / 11
- Visual Studio 2022+ with C++ workload (or Build Tools)
- CMake 3.20+ (included with Visual Studio)
- Internet connection

## Build

```bat
git clone https://github.com/090TYPE/orderbook-viz.git
cd orderbook-viz
build.bat
```

On first run, CMake automatically downloads [nlohmann/json](https://github.com/nlohmann/json) via FetchContent.

## Run

```bat
:: BTC/USDT (default)
build\Release\orderbook_viz.exe

:: ETH/USDT
build\Release\orderbook_viz.exe ETH

:: SOL/USDT
build\Release\orderbook_viz.exe SOL
```

Or use the shortcut:
```bat
run.bat
run.bat ETH
```

## Architecture

| File | Description |
|------|-------------|
| `include/ws_client.hpp` | WinHTTP-based WebSocket client with auto-reconnect |
| `include/orderbook.hpp` | Thread-safe order book data structure |
| `include/display.hpp` | ANSI terminal renderer (colors, bars, layout) |
| `include/binance_client.hpp` | Binance depth stream parser |
| `include/okx_client.hpp` | OKX books channel parser |
| `include/bybit_client.hpp` | Bybit orderbook stream parser |
| `src/main.cpp` | Entry point, render loop |

## WebSocket Endpoints

| Exchange | URL |
|----------|-----|
| Binance | `wss://stream.binance.com:9443/ws/btcusdt@depth20@100ms` |
| OKX | `wss://ws.okx.com:8443/ws/v5/public` |
| Bybit | `wss://stream.bybit.com/v5/public/spot` |

## License

MIT
