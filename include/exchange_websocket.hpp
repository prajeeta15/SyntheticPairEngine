#pragma once

#include "market_data.hpp"
#include "data_feed.hpp"
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <chrono>

namespace spe
{
    namespace exchange_ws
    {

        using namespace market_data;
        using namespace data_feed;
        using json = nlohmann::json;

        // WebSocket client type
        using WebSocketClient = websocketpp::client<websocketpp::config::asio_tls_client>;
        using WebSocketMessage = websocketpp::config::asio_tls_client::message_type::ptr;
        using WebSocketConnectionHdl = websocketpp::connection_hdl;

        enum class ExchangeType
        {
            OKX,
            BINANCE,
            BYBIT
        };

        enum class DataType
        {
            ORDERBOOK_L2,
            TRADES,
            TICKERS,
            FUNDING_RATES,
            MARK_PRICE,
            INDEX_PRICE,
            KLINES,
            LIQUIDATIONS
        };

        enum class InstrumentType
        {
            SPOT,
            FUTURES,
            PERPETUAL,
            OPTIONS
        };

        struct ExchangeConfig 
        {
            std::string base_url;
            std::string spot_ws_url;
            std::string futures_ws_url;
            std::string options_ws_url;
            std::map<DataType, std::string> endpoints;
            int ping_interval_ms = 20000;
            int reconnect_delay_ms = 5000;
            int max_reconnect_attempts = 10;
        };

        struct SubscriptionRequest // This is used for both spot and futures subscriptions
        {
            std::string symbol;
            InstrumentType instrument_type;
            DataType data_type;
            std::map<std::string, std::string> params;
        };

        struct OrderBookLevel 
        {
            double price;
            double quantity;
            Timestamp timestamp;

            OrderBookLevel(double p = 0.0, double q = 0.0)
                : price(p), quantity(q), timestamp(std::chrono::high_resolution_clock::now()) {}
        };

        struct OrderBookSnapshot // This is used for both spot and futures subscriptions
        {
            InstrumentId instrument_id;
            std::string exchange;
            std::vector<OrderBookLevel> bids;
            std::vector<OrderBookLevel> asks;
            Timestamp timestamp;
            uint64_t sequence_number;

            OrderBookSnapshot() : sequence_number(0), timestamp(std::chrono::high_resolution_clock::now()) {}
        };

        struct FundingRateData 
        {
            InstrumentId instrument_id;
            std::string exchange;
            double funding_rate;
            double predicted_funding_rate;
            Timestamp funding_time;
            Timestamp next_funding_time;
            Timestamp timestamp;

            FundingRateData() : funding_rate(0.0), predicted_funding_rate(0.0),
                                timestamp(std::chrono::high_resolution_clock::now()) {}
        };

        struct MarkPriceData 
        {
            InstrumentId instrument_id;
            std::string exchange;
            double mark_price;
            double index_price;
            double funding_rate;
            Timestamp timestamp;

            MarkPriceData() : mark_price(0.0), index_price(0.0), funding_rate(0.0),
                              timestamp(std::chrono::high_resolution_clock::now()) {}
        };

        struct TickerData
        {
            InstrumentId instrument_id;
            std::string exchange;
            double last_price;
            double bid_price;
            double ask_price;
            double bid_size;
            double ask_size;
            double volume_24h;
            double price_change_24h;
            double price_change_percent_24h;
            double high_24h;
            double low_24h;
            Timestamp timestamp;

            TickerData() : last_price(0.0), bid_price(0.0), ask_price(0.0), bid_size(0.0),
                           ask_size(0.0), volume_24h(0.0), price_change_24h(0.0),
                           price_change_percent_24h(0.0), high_24h(0.0), low_24h(0.0),
                           timestamp(std::chrono::high_resolution_clock::now()) {}
        };

        // Callback types for different data types
        using OrderBookCallback = std::function<void(const OrderBookSnapshot &)>;
        using FundingRateCallback = std::function<void(const FundingRateData &)>;
        using MarkPriceCallback = std::function<void(const MarkPriceData &)>;
        using TickerCallback = std::function<void(const TickerData &)>;

        class IExchangeWebSocket  // base interface for exchange-specific WebSocket client
        {
        public:
            virtual ~IExchangeWebSocket() = default;

            // Connection management
            virtual bool connect() = 0;
            virtual void disconnect() = 0;
            virtual bool is_connected() const = 0;
            virtual FeedStatus get_status() const = 0;

            // Subscription management
            virtual bool subscribe_orderbook(const std::string &symbol, InstrumentType type = InstrumentType::SPOT) = 0;
            virtual bool subscribe_trades(const std::string &symbol, InstrumentType type = InstrumentType::SPOT) = 0;
            virtual bool subscribe_tickers(const std::string &symbol, InstrumentType type = InstrumentType::SPOT) = 0;
            virtual bool subscribe_funding_rates(const std::string &symbol) = 0;
            virtual bool subscribe_mark_price(const std::string &symbol) = 0;
            virtual bool subscribe_index_price(const std::string &symbol) = 0;
            virtual bool unsubscribe(const std::string &symbol, DataType data_type, InstrumentType type = InstrumentType::SPOT) = 0;

            // Callback registration
            virtual void set_orderbook_callback(OrderBookCallback callback) = 0;
            virtual void set_trade_callback(TradeCallback callback) = 0;
            virtual void set_ticker_callback(TickerCallback callback) = 0;
            virtual void set_funding_rate_callback(FundingRateCallback callback) = 0;
            virtual void set_mark_price_callback(MarkPriceCallback callback) = 0;
            virtual void set_error_callback(ErrorCallback callback) = 0;

            // Data access
            virtual OrderBookSnapshot get_latest_orderbook(const std::string &symbol, InstrumentType type = InstrumentType::SPOT) const = 0;
            virtual FundingRateData get_latest_funding_rate(const std::string &symbol) const = 0;
            virtual MarkPriceData get_latest_mark_price(const std::string &symbol) const = 0;
            virtual TickerData get_latest_ticker(const std::string &symbol, InstrumentType type = InstrumentType::SPOT) const = 0;

            // Configuration
            virtual ExchangeType get_exchange_type() const = 0;
            virtual void set_config(const ExchangeConfig &config) = 0;
        };

        class BaseExchangeWebSocket : public IExchangeWebSocket // base class abstract implementing common functionality for all exchanges
        {
        protected:
            ExchangeType exchange_type_;
            ExchangeConfig config_;
            std::atomic<FeedStatus> status_;

            // WebSocket client
            std::unique_ptr<WebSocketClient> client_;
            WebSocketConnectionHdl connection_hdl_;
            std::thread ws_thread_;
            std::atomic<bool> running_;

            // Data storage
            mutable std::mutex data_mutex_;
            std::map<std::string, OrderBookSnapshot> orderbook_snapshots_;
            std::map<std::string, FundingRateData> funding_rates_;
            std::map<std::string, MarkPriceData> mark_prices_;
            std::map<std::string, TickerData> tickers_;
            std::map<std::string, std::queue<Trade>> trade_history_;

            // Subscriptions tracking
            std::set<std::string> active_subscriptions_;
            std::map<std::string, SubscriptionRequest> subscription_requests_;

            // Callbacks
            OrderBookCallback orderbook_callback_;
            TradeCallback trade_callback_;
            TickerCallback ticker_callback_;
            FundingRateCallback funding_rate_callback_;
            MarkPriceCallback mark_price_callback_;
            ErrorCallback error_callback_;

            // Connection management
            std::mutex connection_mutex_;
            std::condition_variable connection_cv_;
            std::atomic<bool> connection_established_;
            std::chrono::steady_clock::time_point last_ping_;
            std::thread ping_thread_;

            // Reconnection
            std::atomic<int> reconnect_attempts_;

            // Virtual methods for exchange-specific implementation
            virtual std::string get_websocket_url(InstrumentType type) const = 0;
            virtual json create_subscription_message(const SubscriptionRequest &request) const = 0;
            virtual json create_unsubscription_message(const std::string &symbol, DataType data_type, InstrumentType type) const = 0;
            virtual void process_message(const std::string &message) = 0;
            virtual std::string get_subscription_key(const std::string &symbol, DataType data_type, InstrumentType type) const = 0;

            // Common methods
            void ws_thread_func();
            void ping_thread_func();
            void on_open(WebSocketConnectionHdl hdl);
            void on_close(WebSocketConnectionHdl hdl);
            void on_fail(WebSocketConnectionHdl hdl);
            void on_message(WebSocketConnectionHdl hdl, WebSocketMessage msg);
            void send_message(const json &message);
            void resubscribe_all();
            bool attempt_reconnect();
            void handle_error(const std::string &error_msg);

        public:
            BaseExchangeWebSocket(ExchangeType type, const ExchangeConfig &config);
            virtual ~BaseExchangeWebSocket();

            // IExchangeWebSocket implementation
            bool connect() override;
            void disconnect() override;
            bool is_connected() const override;
            FeedStatus get_status() const override;

            void set_orderbook_callback(OrderBookCallback callback) override { orderbook_callback_ = callback; }
            void set_trade_callback(TradeCallback callback) override { trade_callback_ = callback; }
            void set_ticker_callback(TickerCallback callback) override { ticker_callback_ = callback; }
            void set_funding_rate_callback(FundingRateCallback callback) override { funding_rate_callback_ = callback; }
            void set_mark_price_callback(MarkPriceCallback callback) override { mark_price_callback_ = callback; }
            void set_error_callback(ErrorCallback callback) override { error_callback_ = callback; }

            OrderBookSnapshot get_latest_orderbook(const std::string &symbol, InstrumentType type = InstrumentType::SPOT) const override;
            FundingRateData get_latest_funding_rate(const std::string &symbol) const override;
            MarkPriceData get_latest_mark_price(const std::string &symbol) const override;
            TickerData get_latest_ticker(const std::string &symbol, InstrumentType type = InstrumentType::SPOT) const override;

            ExchangeType get_exchange_type() const override { return exchange_type_; }
            void set_config(const ExchangeConfig &config) override { config_ = config; }
        };
      // exchnage specific implementation
        class OKXWebSocket : public BaseExchangeWebSocket
        {
        private:
            std::map<std::string, std::string> symbol_mapping_;

            std::string get_websocket_url(InstrumentType type) const override;
            json create_subscription_message(const SubscriptionRequest &request) const override;
            json create_unsubscription_message(const std::string &symbol, DataType data_type, InstrumentType type) const override;
            void process_message(const std::string &message) override;
            std::string get_subscription_key(const std::string &symbol, DataType data_type, InstrumentType type) const override;

            // OKX-specific message processors
            void process_orderbook_message(const json &data);
            void process_trade_message(const json &data);
            void process_ticker_message(const json &data);
            void process_funding_rate_message(const json &data);
            void process_mark_price_message(const json &data);

            std::string normalize_symbol(const std::string &symbol, InstrumentType type) const;

        public:
            OKXWebSocket(const ExchangeConfig &config = get_okx_config());

            bool subscribe_orderbook(const std::string &symbol, InstrumentType type = InstrumentType::SPOT) override;
            bool subscribe_trades(const std::string &symbol, InstrumentType type = InstrumentType::SPOT) override;
            bool subscribe_tickers(const std::string &symbol, InstrumentType type = InstrumentType::SPOT) override;
            bool subscribe_funding_rates(const std::string &symbol) override;
            bool subscribe_mark_price(const std::string &symbol) override;
            bool subscribe_index_price(const std::string &symbol) override;
            bool unsubscribe(const std::string &symbol, DataType data_type, InstrumentType type = InstrumentType::SPOT) override;

            static ExchangeConfig get_okx_config();
        };

        class BinanceWebSocket : public BaseExchangeWebSocket
        {
        private:
            std::map<std::string, uint64_t> orderbook_sequence_numbers_;

            std::string get_websocket_url(InstrumentType type) const override;
            json create_subscription_message(const SubscriptionRequest &request) const override;
            json create_unsubscription_message(const std::string &symbol, DataType data_type, InstrumentType type) const override;
            void process_message(const std::string &message) override;
            std::string get_subscription_key(const std::string &symbol, DataType data_type, InstrumentType type) const override;

            // Binance-specific message processors
            void process_orderbook_message(const json &data);
            void process_trade_message(const json &data);
            void process_ticker_message(const json &data);
            void process_funding_rate_message(const json &data);
            void process_mark_price_message(const json &data);

            std::string normalize_symbol(const std::string &symbol, InstrumentType type) const;
            std::string get_stream_name(const std::string &symbol, DataType data_type, InstrumentType type) const;

        public:
            BinanceWebSocket(const ExchangeConfig &config = get_binance_config());

            bool subscribe_orderbook(const std::string &symbol, InstrumentType type = InstrumentType::SPOT) override;
            bool subscribe_trades(const std::string &symbol, InstrumentType type = InstrumentType::SPOT) override;
            bool subscribe_tickers(const std::string &symbol, InstrumentType type = InstrumentType::SPOT) override;
            bool subscribe_funding_rates(const std::string &symbol) override;
            bool subscribe_mark_price(const std::string &symbol) override;
            bool subscribe_index_price(const std::string &symbol) override;
            bool unsubscribe(const std::string &symbol, DataType data_type, InstrumentType type = InstrumentType::SPOT) override;

            static ExchangeConfig get_binance_config();
        };

        class BybitWebSocket : public BaseExchangeWebSocket
        {
        private:
            std::map<std::string, std::string> subscription_topics_;

            std::string get_websocket_url(InstrumentType type) const override;
            json create_subscription_message(const SubscriptionRequest &request) const override;
            json create_unsubscription_message(const std::string &symbol, DataType data_type, InstrumentType type) const override;
            void process_message(const std::string &message) override;
            std::string get_subscription_key(const std::string &symbol, DataType data_type, InstrumentType type) const override;

            // Bybit-specific message processors
            void process_orderbook_message(const json &data);
            void process_trade_message(const json &data);
            void process_ticker_message(const json &data);
            void process_funding_rate_message(const json &data);
            void process_mark_price_message(const json &data);

            std::string normalize_symbol(const std::string &symbol, InstrumentType type) const;
            std::string get_topic_name(const std::string &symbol, DataType data_type, InstrumentType type) const;

        public:
            BybitWebSocket(const ExchangeConfig &config = get_bybit_config());

            bool subscribe_orderbook(const std::string &symbol, InstrumentType type = InstrumentType::SPOT) override;
            bool subscribe_trades(const std::string &symbol, InstrumentType type = InstrumentType::SPOT) override;
            bool subscribe_tickers(const std::string &symbol, InstrumentType type = InstrumentType::SPOT) override;
            bool subscribe_funding_rates(const std::string &symbol) override;
            bool subscribe_mark_price(const std::string &symbol) override;
            bool subscribe_index_price(const std::string &symbol) override;
            bool unsubscribe(const std::string &symbol, DataType data_type, InstrumentType type = InstrumentType::SPOT) override;

            static ExchangeConfig get_bybit_config();
        };

        // Multi-exchange aggregator
        class MultiExchangeWebSocketManager // for managing multiple exchange connections simultaneously
        {
        private:
            std::map<ExchangeType, std::unique_ptr<IExchangeWebSocket>> exchanges_;
            std::map<std::string, std::set<ExchangeType>> symbol_subscriptions_;

            // Unified callbacks
            OrderBookCallback unified_orderbook_callback_;
            TradeCallback unified_trade_callback_;
            TickerCallback unified_ticker_callback_;
            FundingRateCallback unified_funding_rate_callback_;
            MarkPriceCallback unified_mark_price_callback_;
            ErrorCallback unified_error_callback_;

            // Data synchronization
            mutable std::mutex sync_mutex_;
            std::map<std::string, std::map<ExchangeType, OrderBookSnapshot>> unified_orderbooks_;
            std::map<std::string, std::map<ExchangeType, TickerData>> unified_tickers_;
            std::map<std::string, std::map<ExchangeType, FundingRateData>> unified_funding_rates_;

            void setup_exchange_callbacks(ExchangeType exchange);

        public:
            MultiExchangeWebSocketManager();
            ~MultiExchangeWebSocketManager();

            // Exchange management
            bool add_exchange(ExchangeType exchange, const ExchangeConfig &config = {});
            void remove_exchange(ExchangeType exchange);
            bool connect_exchange(ExchangeType exchange);
            void disconnect_exchange(ExchangeType exchange);
            bool is_exchange_connected(ExchangeType exchange) const;

            // Multi-exchange subscriptions
            bool subscribe_orderbook_all_exchanges(const std::string &symbol, InstrumentType type = InstrumentType::SPOT);
            bool subscribe_tickers_all_exchanges(const std::string &symbol, InstrumentType type = InstrumentType::SPOT);
            bool subscribe_funding_rates_all_exchanges(const std::string &symbol);

            // Specific exchange subscriptions
            bool subscribe_orderbook(ExchangeType exchange, const std::string &symbol, InstrumentType type = InstrumentType::SPOT);
            bool subscribe_trades(ExchangeType exchange, const std::string &symbol, InstrumentType type = InstrumentType::SPOT);
            bool subscribe_tickers(ExchangeType exchange, const std::string &symbol, InstrumentType type = InstrumentType::SPOT);
            bool subscribe_funding_rates(ExchangeType exchange, const std::string &symbol);
            bool subscribe_mark_price(ExchangeType exchange, const std::string &symbol);

            // Unified callbacks
            void set_unified_orderbook_callback(OrderBookCallback callback);
            void set_unified_trade_callback(TradeCallback callback);
            void set_unified_ticker_callback(TickerCallback callback);
            void set_unified_funding_rate_callback(FundingRateCallback callback);
            void set_unified_mark_price_callback(MarkPriceCallback callback);
            void set_unified_error_callback(ErrorCallback callback);

            // Data access
            std::map<ExchangeType, OrderBookSnapshot> get_unified_orderbook(const std::string &symbol, InstrumentType type = InstrumentType::SPOT) const;
            std::map<ExchangeType, TickerData> get_unified_ticker(const std::string &symbol, InstrumentType type = InstrumentType::SPOT) const;
            std::map<ExchangeType, FundingRateData> get_unified_funding_rates(const std::string &symbol) const;

            // Status monitoring
            std::map<ExchangeType, FeedStatus> get_all_exchange_status() const;
            bool are_all_exchanges_connected() const;

            // Configuration
            static ExchangeConfig get_default_config(ExchangeType exchange);
        };

        // Utility functions
        std::string exchange_type_to_string(ExchangeType type);
        ExchangeType string_to_exchange_type(const std::string &str);
        std::string data_type_to_string(DataType type);
        DataType string_to_data_type(const std::string &str);
        std::string instrument_type_to_string(InstrumentType type);
        InstrumentType string_to_instrument_type(const std::string &str);

    } // namespace exchange_ws
} // namespace spe
