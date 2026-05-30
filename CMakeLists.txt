cmake_minimum_required(VERSION 3.20)
project(orderbook_viz CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# nlohmann/json — header-only, no TLS deps needed
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# ── Executable ─────────────────────────────────────────────────────────────────
add_executable(orderbook_viz src/main.cpp)

target_include_directories(orderbook_viz PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(orderbook_viz PRIVATE
    nlohmann_json::nlohmann_json
    winhttp      # Windows native WebSocket + TLS (WSS out of the box)
    ws2_32
)

if(MSVC)
    target_compile_options(orderbook_viz PRIVATE /W3 /O2 /utf-8)
endif()
