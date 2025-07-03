#pragma once

#include <string>
#include <chrono>
#include <vector>
#include <map>
#include <memory>

namespace spe
{
    namespace market_data
    {

        using Timestamp = std::chrono::high_resolution_clock::time_point;
        using Price = double;
        using Volume = double;
        using InstrumentId = std::string;

        enum class InstrumentType
        {
            SPOT,
            FORWARD,
            OPTION,
            FUTURE,
            SWAP
        };

        enum class Side
        {
            BID,
            ASK
        };

        struct Quote
        {
            InstrumentId instrument_id;
            Price bid_price;
            Price ask_price;
            Volume bid_size;
            Volume ask_size;
            Timestamp timestamp;
            uint64_t sequence_number;

            Quote() = default;
            Quote(const InstrumentId &id, Price bid, Price ask, Volume bid_vol, Volume ask_vol)
                : instrument_id(id), bid_price(bid), ask_price(ask),
                  bid_size(bid_vol), ask_size(ask_vol),
                  timestamp(std::chrono::high_resolution_clock::now()),
                  sequence_number(0) {}
        };

        struct Trade
        {
            InstrumentId instrument_id;
            Price price;
            Volume size;
            Side side;
            Timestamp timestamp;
            uint64_t sequence_number;
            std::string trade_id;

            Trade() = default;
            Trade(const InstrumentId &id, Price p, Volume s, Side sd)
                : instrument_id(id), price(p), size(s), side(sd),
                  timestamp(std::chrono::high_resolution_clock::now()),
                  sequence_number(0) {}
        };

        struct Instrument
        {
            InstrumentId id;
            std::string symbol;
            InstrumentType type;
            std::string base_currency;
            std::string quote_currency;
            Price tick_size;
            Volume min_size;
            Timestamp expiry; // For derivatives
            Price strike;     // For options

            Instrument() = default;
            Instrument(const InstrumentId &instrument_id, const std::string &sym, InstrumentType t)
                : id(instrument_id), symbol(sym), type(t), tick_size(0.0001), min_size(1.0) {}
        };

        struct MarketDepth
        {
            InstrumentId instrument_id;
            std::vector<std::pair<Price, Volume>> bids;
            std::vector<std::pair<Price, Volume>> asks;
            Timestamp timestamp;

            MarketDepth() = default;
            MarketDepth(const InstrumentId &id) : instrument_id(id) {}
        };

        struct MarketSnapshot
        {
            std::map<InstrumentId, Quote> quotes;
            std::map<InstrumentId, std::vector<Trade>> recent_trades;
            std::map<InstrumentId, MarketDepth> depth;
            Timestamp snapshot_time;

            MarketSnapshot() : snapshot_time(std::chrono::high_resolution_clock::now()) {}
        };

    } // namespace market_data
} // namespace spe
