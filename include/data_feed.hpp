#pragma once

#include "market_data.hpp"
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace spe {
namespace data_feed {

using namespace market_data;

// Callback function types
using QuoteCallback = std::function<void(const Quote&)>;
using TradeCallback = std::function<void(const Trade&)>;
using DepthCallback = std::function<void(const MarketDepth&)>;
using ErrorCallback = std::function<void(const std::string&)>;

enum class FeedStatus {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    ERROR
};

class IDataFeed {
public:
    virtual ~IDataFeed() = default;
    
    // Connection management
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual FeedStatus get_status() const = 0;
    
    // Subscription management
    virtual bool subscribe_quotes(const std::vector<InstrumentId>& instruments) = 0;
    virtual bool subscribe_trades(const std::vector<InstrumentId>& instruments) = 0;
    virtual bool subscribe_depth(const std::vector<InstrumentId>& instruments) = 0;
    virtual bool unsubscribe(const InstrumentId& instrument) = 0;
    
    // Callback registration
    virtual void set_quote_callback(QuoteCallback callback) = 0;
    virtual void set_trade_callback(TradeCallback callback) = 0;
    virtual void set_depth_callback(DepthCallback callback) = 0;
    virtual void set_error_callback(ErrorCallback callback) = 0;
    
    // Data retrieval
    virtual Quote get_latest_quote(const InstrumentId& instrument) const = 0;
    virtual std::vector<Trade> get_recent_trades(const InstrumentId& instrument, size_t count = 100) const = 0;
    virtual MarketDepth get_market_depth(const InstrumentId& instrument) const = 0;
};

class SimulatedDataFeed : public IDataFeed {
private:
    std::atomic<FeedStatus> status_;
    std::thread feed_thread_;
    std::atomic<bool> running_;
    
    // Callbacks
    QuoteCallback quote_callback_;
    TradeCallback trade_callback_;
    DepthCallback depth_callback_;
    ErrorCallback error_callback_;
    
    // Data storage
    mutable std::mutex data_mutex_;
    std::map<InstrumentId, Quote> latest_quotes_;
    std::map<InstrumentId, std::queue<Trade>> trade_history_;
    std::map<InstrumentId, MarketDepth> market_depths_;
    
    // Subscriptions
    std::set<InstrumentId> subscribed_instruments_;
    
    void feed_loop();
    void generate_sample_data();
    Quote generate_quote(const InstrumentId& instrument_id);
    Trade generate_trade(const InstrumentId& instrument_id);
    
public:
    SimulatedDataFeed();
    ~SimulatedDataFeed();
    
    // IDataFeed implementation
    bool connect() override;
    void disconnect() override;
    FeedStatus get_status() const override;
    
    bool subscribe_quotes(const std::vector<InstrumentId>& instruments) override;
    bool subscribe_trades(const std::vector<InstrumentId>& instruments) override;
    bool subscribe_depth(const std::vector<InstrumentId>& instruments) override;
    bool unsubscribe(const InstrumentId& instrument) override;
    
    void set_quote_callback(QuoteCallback callback) override;
    void set_trade_callback(TradeCallback callback) override;
    void set_depth_callback(DepthCallback callback) override;
    void set_error_callback(ErrorCallback callback) override;
    
    Quote get_latest_quote(const InstrumentId& instrument) const override;
    std::vector<Trade> get_recent_trades(const InstrumentId& instrument, size_t count = 100) const override;
    MarketDepth get_market_depth(const InstrumentId& instrument) const override;
};

} // namespace data_feed
} // namespace spe
