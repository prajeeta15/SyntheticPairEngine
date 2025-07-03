# 🧠 Synthetic Pair Derivation Engine

A high-performance, C++-based arbitrage detection system for cryptocurrency markets. This engine calculates synthetic trading pair prices in real-time, compares them to actual market rates, and detects profitable arbitrage opportunities across exchanges like Binance, OKX, and Bybit.

---

## 📌 Features

- ✅ Real-time data from Binance, OKX, and Bybit via WebSocket
- ✅ Synthetic price calculation from multi-leg market formulas
- ✅ Arbitrage opportunity detection and spread evaluation
- ✅ Modular architecture with clean separation of components
- ✅ Risk & exposure management using portfolio metrics (VaR, ES)
- ✅ In-memory caching for high-speed access
- ✅ CLI terminal log output of arbitrage legs and metrics

---

## 🧱 Architecture Overview:
- ![Screenshot 2025-07-03 123844](https://github.com/user-attachments/assets/aabeee94-7c86-4916-96b3-fbce7e9412ce)

---

## ⚙️ Getting Started

### 1. Clone the Repo

bash
git clone https://github.com/yourusername/SyntheticPairEngine.git
cd SyntheticPairEngine

### 2. Install Dependencies
Ensure you have:
- C++20 compiler (g++ or MSVC)
- CMake 3.15+
- vcpkg or manually install:
- rapidjson
- WebSocket++ (or similar)
- Threads and networking libraries

  ### 3. Build Project
  mkdir build && cd build
cmake ..
cmake --build .

### 4. Run the Engine
./SyntheticPairEngine

---

## Exposure & Risk Management:
- Position sizing using configurable exposure caps
- Value-at-Risk (VaR) and Expected Shortfall (ES)
- Per-instrument exposure limits
- Strategy validation before opportunity execution

----

## Performance:
Tested on Intel i5, 8GB RAM:

Tick rate	~250 ticks/sec
Avg latency	10–20 ms
CPU usage (3 exchanges)	~35% peak
