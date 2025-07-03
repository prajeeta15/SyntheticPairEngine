#pragma once

#include "market_data.hpp"
#include "pricing_models.hpp"
#include "mispricing_detector.hpp"
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <queue>
#include <set>

namespace spe
{
    namespace arbitrage
    {

        using namespace market_data;
        using namespace pricing;
        using namespace mispricing;

        enum class ArbitrageType
        {
            PURE_ARBITRAGE,
            STATISTICAL_ARBITRAGE,
            TRIANGULAR_ARBITRAGE,
            CALENDAR_SPREAD,
            INTER_MARKET_SPREAD,
            SPOT_FUNDING_SYNTHETIC_PERPETUAL,
            CROSS_EXCHANGE_SYNTHETIC_REPLICATION,
            MULTI_INSTRUMENT_SYNTHETIC_COMBINATION
        };

        enum class ArbitrageStatus
        {
            IDENTIFIED,
            VALIDATED,
            EXECUTING,
            COMPLETED,
            FAILED,
            EXPIRED
        };

        struct ArbitrageLeg // Represents one leg of an arbitrage opportunity
        {
            InstrumentId instrument_id;
            Side side; // BID or ASK
            Volume size;
            Price entry_price;
            Price exit_price;
            double weight;
            Timestamp entry_time;
            Timestamp exit_time;

            ArbitrageLeg() : size(0.0), entry_price(0.0), exit_price(0.0), weight(0.0) {}
            ArbitrageLeg(const InstrumentId &id, Side s, Volume sz, Price price, double w)
                : instrument_id(id), side(s), size(sz), entry_price(price), exit_price(0.0), weight(w) {}
        };

        struct ArbitrageOpportunity // detailed structure for an arbitrage opportunity including legs, mispricing source, and financial metrics
        {
            std::string opportunity_id;
            ArbitrageType type;
            ArbitrageStatus status;

            std::vector<ArbitrageLeg> legs;
            MispricingOpportunity mispricing_source;

            // Financial metrics
            double expected_profit;
            double max_loss;
            double profit_probability;
            double break_even_price;
            double total_cost;
            double net_exposure;

            // Risk metrics
            double value_at_risk;
            double expected_shortfall;
            double sharpe_ratio;
            double max_drawdown;
            double correlation_risk;

            // Timing
            Timestamp identification_time;
            Timestamp validation_time;
            Timestamp expiry_time;
            std::chrono::milliseconds estimated_duration;

            // Execution details
            double slippage_estimate;
            double transaction_costs;
            Volume total_volume;
            double market_impact;

            ArbitrageOpportunity() : expected_profit(0.0), max_loss(0.0), profit_probability(0.0),
                                     break_even_price(0.0), total_cost(0.0), net_exposure(0.0),
                                     value_at_risk(0.0), expected_shortfall(0.0), sharpe_ratio(0.0),
                                     max_drawdown(0.0), correlation_risk(0.0), slippage_estimate(0.0),
                                     transaction_costs(0.0), total_volume(0.0), market_impact(0.0),
                                     identification_time(std::chrono::high_resolution_clock::now()) {}
        };

        struct ArbitrageParameters // configurable parameters for arbitrage engine
        {
            double min_profit_threshold = 0.001; // 0.1%
            double max_risk_per_trade = 0.02;    // 2%
            double max_correlation_risk = 0.3;
            double max_market_impact = 0.005; // 0.5%
            double max_slippage = 0.001;      // 0.1%
            Volume max_position_size = 1000000.0;
            std::chrono::minutes max_holding_period = std::chrono::minutes(60);
            double min_liquidity_requirement = 100000.0;
            double confidence_threshold = 0.8;
        };

        using ArbitrageCallback = std::function<void(const ArbitrageOpportunity &)>;
        using ArbitrageUpdateCallback = std::function<void(const ArbitrageOpportunity &)>;

        class IArbitrageEngine // base interface for all arbitrage engines, defining common methods
        {
        public:
            virtual ~IArbitrageEngine() = default;

            virtual void update_market_data(const MarketSnapshot &snapshot) = 0;
            virtual void process_mispricing(const MispricingOpportunity &mispricing) = 0;
            virtual std::vector<ArbitrageOpportunity> identify_opportunities() = 0;
            virtual bool validate_opportunity(ArbitrageOpportunity &opportunity) = 0;

            virtual void set_opportunity_callback(ArbitrageCallback callback) = 0;
            virtual void set_update_callback(ArbitrageUpdateCallback callback) = 0;
            virtual void update_parameters(const ArbitrageParameters &params) = 0;

            virtual std::vector<ArbitrageOpportunity> get_active_opportunities() const = 0;
            virtual void clear_opportunities() = 0;
        };

        class ArbitrageEngine : public IArbitrageEngine
        {
        private:
            ArbitrageParameters params_;
            std::vector<ArbitrageOpportunity> active_opportunities_;
            std::queue<MispricingOpportunity> pending_mispricings_;

            // Market data
            MarketSnapshot latest_snapshot_;

            // Callbacks
            ArbitrageCallback opportunity_callback_;
            ArbitrageUpdateCallback update_callback_;

            // Internal methods
            ArbitrageOpportunity create_arbitrage_from_mispricing(const MispricingOpportunity &mispricing);
            std::vector<ArbitrageLeg> optimize_legs(const MispricingOpportunity &mispricing);

            // Risk calculations
            double calculate_value_at_risk(const ArbitrageOpportunity &opportunity);
            double calculate_expected_shortfall(const ArbitrageOpportunity &opportunity);
            double calculate_correlation_risk(const std::vector<ArbitrageLeg> &legs);
            double calculate_market_impact(const ArbitrageOpportunity &opportunity);

            // Validation methods
            bool validate_liquidity(const ArbitrageOpportunity &opportunity);
            bool validate_risk_limits(const ArbitrageOpportunity &opportunity);
            bool validate_timing(const ArbitrageOpportunity &opportunity);
            bool validate_execution_feasibility(const ArbitrageOpportunity &opportunity);

            // Opportunity management
            void cleanup_expired_opportunities();
            void update_opportunity_status(ArbitrageOpportunity &opportunity);
            std::string generate_opportunity_id();

        public:
            ArbitrageEngine(const ArbitrageParameters &params = ArbitrageParameters{});
            ~ArbitrageEngine();

            void update_market_data(const MarketSnapshot &snapshot) override;
            void process_mispricing(const MispricingOpportunity &mispricing) override;
            std::vector<ArbitrageOpportunity> identify_opportunities() override;
            bool validate_opportunity(ArbitrageOpportunity &opportunity) override;

            void set_opportunity_callback(ArbitrageCallback callback) override;
            void set_update_callback(ArbitrageUpdateCallback callback) override;
            void update_parameters(const ArbitrageParameters &params) override;

            std::vector<ArbitrageOpportunity> get_active_opportunities() const override;
            void clear_opportunities() override;

            // Additional methods
            void update_opportunity_status(const std::string &opportunity_id, ArbitrageStatus status);
            ArbitrageOpportunity get_opportunity_by_id(const std::string &opportunity_id) const;
        };

        class TriangularArbitrageEngine : public IArbitrageEngine // specialized for triangular arbitrage in currency markets
        {
        private:
            ArbitrageParameters params_;
            std::map<std::string, std::vector<InstrumentId>> currency_triangles_;
            std::vector<ArbitrageOpportunity> active_opportunities_;

            ArbitrageCallback opportunity_callback_;
            ArbitrageUpdateCallback update_callback_;

            std::vector<ArbitrageOpportunity> identify_triangular_opportunities(const MarketSnapshot &snapshot);
            ArbitrageOpportunity create_triangular_opportunity(const std::vector<InstrumentId> &triangle,
                                                               const MarketSnapshot &snapshot);
            double calculate_triangular_profit(const std::vector<Quote> &quotes);

        public:
            TriangularArbitrageEngine(const ArbitrageParameters &params = ArbitrageParameters{});

            void update_market_data(const MarketSnapshot &snapshot) override;
            void process_mispricing(const MispricingOpportunity &mispricing) override;
            std::vector<ArbitrageOpportunity> identify_opportunities() override;
            bool validate_opportunity(ArbitrageOpportunity &opportunity) override;

            void set_opportunity_callback(ArbitrageCallback callback) override;
            void set_update_callback(ArbitrageUpdateCallback callback) override;
            void update_parameters(const ArbitrageParameters &params) override;

            std::vector<ArbitrageOpportunity> get_active_opportunities() const override;
            void clear_opportunities() override;

            void add_currency_triangle(const std::string &name, const std::vector<InstrumentId> &instruments);
            void remove_currency_triangle(const std::string &name);
        };

        class StatisticalArbitrageEngine : public IArbitrageEngine // focuses on stat arbitrage opportunities
        {
        private:
            ArbitrageParameters params_;
            std::unique_ptr<IPricingModel> pricing_model_;
            std::vector<ArbitrageOpportunity> active_opportunities_;

            // Historical data for mean reversion
            std::map<InstrumentId, std::queue<Price>> price_history_;
            std::map<std::pair<InstrumentId, InstrumentId>, double> correlation_matrix_;

            ArbitrageCallback opportunity_callback_;
            ArbitrageUpdateCallback update_callback_;

            std::vector<ArbitrageOpportunity> identify_statistical_opportunities(const MarketSnapshot &snapshot);
            ArbitrageOpportunity create_pairs_trade(const InstrumentId &instrument1,
                                                    const InstrumentId &instrument2,
                                                    const MarketSnapshot &snapshot);
            double calculate_hedge_ratio(const InstrumentId &instrument1, const InstrumentId &instrument2);

        public:
            StatisticalArbitrageEngine(std::unique_ptr<IPricingModel> model,
                                       const ArbitrageParameters &params = ArbitrageParameters{});

            void update_market_data(const MarketSnapshot &snapshot) override;
            void process_mispricing(const MispricingOpportunity &mispricing) override;
            std::vector<ArbitrageOpportunity> identify_opportunities() override;
            bool validate_opportunity(ArbitrageOpportunity &opportunity) override;

            void set_opportunity_callback(ArbitrageCallback callback) override;
            void set_update_callback(ArbitrageUpdateCallback callback) override;
            void update_parameters(const ArbitrageParameters &params) override;

            std::vector<ArbitrageOpportunity> get_active_opportunities() const override;
            void clear_opportunities() override;

            void update_correlation_matrix(const MarketSnapshot &snapshot);
        };

        // Spot + Funding Rate Synthetic Perpetual Arbitrage Engine
        class SpotFundingSyntheticPerpetualEngine : public IArbitrageEngine // handles arbitrage between spot and perpetual future markets
        {
        private:
            ArbitrageParameters params_;
            std::unique_ptr<PerpetualSwapPricingModel> perpetual_pricing_model_;
            std::vector<ArbitrageOpportunity> active_opportunities_;

            // Funding rate tracking
            std::map<InstrumentId, FundingRate> current_funding_rates_;
            std::map<InstrumentId, std::queue<FundingRate>> funding_rate_history_;

            ArbitrageCallback opportunity_callback_;
            ArbitrageUpdateCallback update_callback_;

            // Spot + funding synthetic methods
            std::vector<ArbitrageOpportunity> identify_spot_funding_opportunities(const MarketSnapshot &snapshot);
            ArbitrageOpportunity create_synthetic_perpetual_opportunity(
                const InstrumentId &spot_instrument,
                const InstrumentId &perpetual_instrument,
                const MarketSnapshot &snapshot);
            double calculate_synthetic_perpetual_fair_value(
                const Quote &spot_quote,
                const FundingRate &funding_rate);
            double calculate_funding_arbitrage_profit(
                const Quote &spot_quote,
                const Quote &perpetual_quote,
                const FundingRate &funding_rate);
            bool is_profitable_funding_arbitrage(
                double profit_percentage,
                double funding_cost,
                double transaction_cost);
            std::vector<ArbitrageLeg> construct_spot_funding_legs(
                const InstrumentId &spot_instrument,
                const InstrumentId &perpetual_instrument,
                const MarketSnapshot &snapshot);

        public:
            SpotFundingSyntheticPerpetualEngine(
                std::unique_ptr<PerpetualSwapPricingModel> model,
                const ArbitrageParameters &params = ArbitrageParameters{});

            void update_market_data(const MarketSnapshot &snapshot) override;
            void process_mispricing(const MispricingOpportunity &mispricing) override;
            std::vector<ArbitrageOpportunity> identify_opportunities() override;
            bool validate_opportunity(ArbitrageOpportunity &opportunity) override;

            void set_opportunity_callback(ArbitrageCallback callback) override;
            void set_update_callback(ArbitrageUpdateCallback callback) override;
            void update_parameters(const ArbitrageParameters &params) override;

            std::vector<ArbitrageOpportunity> get_active_opportunities() const override;
            void clear_opportunities() override;

            // Specific methods for spot+funding arbitrage
            void update_funding_rate(const InstrumentId &instrument, const FundingRate &rate);
            FundingRate get_current_funding_rate(const InstrumentId &instrument) const;
            double calculate_expected_funding_pnl(
                const InstrumentId &instrument,
                Volume position_size,
                std::chrono::hours holding_period) const;
            void add_spot_perpetual_pair(const InstrumentId &spot, const InstrumentId &perpetual);
        };

        // Cross-Exchange Synthetic Replication Engine
        class CrossExchangeSyntheticReplicationEngine : public IArbitrageEngine
        {
        private:
            ArbitrageParameters params_;
            std::unique_ptr<IPricingModel> pricing_model_;
            std::vector<ArbitrageOpportunity> active_opportunities_;

            // Multi-exchange data
            std::map<std::string, MarketSnapshot> exchange_snapshots_;
            std::map<std::string, double> exchange_transaction_costs_;
            std::map<std::string, double> exchange_latencies_;

            // Synthetic replication tracking
            std::map<InstrumentId, std::vector<std::string>> instrument_exchange_mapping_;
            std::map<std::pair<InstrumentId, std::string>, SyntheticPrice> synthetic_price_cache_;

            ArbitrageCallback opportunity_callback_;
            ArbitrageUpdateCallback update_callback_;

            // Cross-exchange synthetic methods
            std::vector<ArbitrageOpportunity> identify_cross_exchange_synthetic_opportunities();
            ArbitrageOpportunity create_cross_exchange_replication_opportunity(
                const InstrumentId &target_instrument,
                const std::string &target_exchange,
                const std::string &replication_exchange,
                const MarketSnapshot &target_snapshot,
                const MarketSnapshot &replication_snapshot);
            SyntheticPrice calculate_cross_exchange_synthetic_price(
                const InstrumentId &instrument,
                const std::string &exchange_id,
                const MarketSnapshot &snapshot);
            double calculate_cross_exchange_arbitrage_profit(
                const SyntheticPrice &target_price,
                const SyntheticPrice &synthetic_price,
                const std::string &target_exchange,
                const std::string &synthetic_exchange);
            std::vector<ArbitrageLeg> construct_cross_exchange_legs(
                const InstrumentId &target_instrument,
                const std::string &target_exchange,
                const std::vector<InstrumentId> &synthetic_components,
                const std::string &synthetic_exchange,
                const std::vector<double> &weights);
            bool validate_cross_exchange_execution(
                const ArbitrageOpportunity &opportunity);
            double estimate_cross_exchange_latency_risk(
                const std::string &exchange1,
                const std::string &exchange2);

        public:
            CrossExchangeSyntheticReplicationEngine( //manages cross-exchange synthetic replication opportunities
                std::unique_ptr<IPricingModel> model,
                const ArbitrageParameters &params = ArbitrageParameters{});

            void update_market_data(const MarketSnapshot &snapshot) override;
            void process_mispricing(const MispricingOpportunity &mispricing) override;
            std::vector<ArbitrageOpportunity> identify_opportunities() override;
            bool validate_opportunity(ArbitrageOpportunity &opportunity) override;

            void set_opportunity_callback(ArbitrageCallback callback) override;
            void set_update_callback(ArbitrageUpdateCallback callback) override;
            void update_parameters(const ArbitrageParameters &params) override;

            std::vector<ArbitrageOpportunity> get_active_opportunities() const override;
            void clear_opportunities() override;

            // Specific methods for cross-exchange replication
            void register_exchange(const std::string &exchange_id, double transaction_cost, double latency_ms);
            void update_exchange_snapshot(const std::string &exchange_id, const MarketSnapshot &snapshot);
            void add_instrument_to_exchange(const InstrumentId &instrument, const std::string &exchange_id);
            std::vector<std::string> get_available_exchanges_for_instrument(const InstrumentId &instrument) const;
            SyntheticPrice get_best_synthetic_replication(
                const InstrumentId &instrument,
                const std::string &exclude_exchange = "") const;
        };

        // Multi-Instrument Synthetic Combinations Engine
        class MultiInstrumentSyntheticCombinationsEngine : public IArbitrageEngine
        { // deals with comples multi-instrument synthetic combinations 
        private:
            ArbitrageParameters params_;
            std::unique_ptr<BasketPricingModel> basket_pricing_model_;
            std::vector<ArbitrageOpportunity> active_opportunities_;

            // Multi-instrument combinations
            std::map<std::string, std::vector<InstrumentId>> predefined_combinations_;
            std::map<std::string, std::vector<double>> combination_weights_;
            std::map<InstrumentId, std::vector<std::string>> instrument_combination_mapping_;

            // Dynamic combination generation
            std::map<InstrumentId, std::vector<InstrumentId>> correlation_clusters_;
            std::map<std::pair<InstrumentId, InstrumentId>, double> correlation_matrix_;

            ArbitrageCallback opportunity_callback_;
            ArbitrageUpdateCallback update_callback_;

            // Multi-instrument synthetic methods
            std::vector<ArbitrageOpportunity> identify_multi_instrument_opportunities(const MarketSnapshot &snapshot);
            ArbitrageOpportunity create_multi_instrument_synthetic_opportunity(
                const std::string &combination_name,
                const InstrumentId &target_instrument,
                const MarketSnapshot &snapshot);
            std::vector<ArbitrageOpportunity> generate_dynamic_combinations(
                const InstrumentId &target_instrument,
                const MarketSnapshot &snapshot);
            double calculate_multi_instrument_synthetic_price(
                const std::vector<InstrumentId> &instruments,
                const std::vector<double> &weights,
                const MarketSnapshot &snapshot);
            std::vector<double> optimize_combination_weights(
                const std::vector<InstrumentId> &instruments,
                const InstrumentId &target_instrument,
                const MarketSnapshot &snapshot);
            std::vector<ArbitrageLeg> construct_multi_instrument_legs(
                const std::vector<InstrumentId> &instruments,
                const std::vector<double> &weights,
                const InstrumentId &target_instrument,
                const MarketSnapshot &snapshot);
            bool validate_combination_quality(
                const std::vector<InstrumentId> &instruments,
                const std::vector<double> &weights,
                const InstrumentId &target_instrument);
            double calculate_combination_tracking_error(
                const std::vector<InstrumentId> &instruments,
                const std::vector<double> &weights,
                const InstrumentId &target_instrument);
            std::vector<InstrumentId> find_optimal_instrument_set(
                const InstrumentId &target_instrument,
                size_t max_instruments = 5);

        public:
            MultiInstrumentSyntheticCombinationsEngine(
                std::unique_ptr<BasketPricingModel> model,
                const ArbitrageParameters &params = ArbitrageParameters{});

            void update_market_data(const MarketSnapshot &snapshot) override;
            void process_mispricing(const MispricingOpportunity &mispricing) override;
            std::vector<ArbitrageOpportunity> identify_opportunities() override;
            bool validate_opportunity(ArbitrageOpportunity &opportunity) override;

            void set_opportunity_callback(ArbitrageCallback callback) override;
            void set_update_callback(ArbitrageUpdateCallback callback) override;
            void update_parameters(const ArbitrageParameters &params) override;

            std::vector<ArbitrageOpportunity> get_active_opportunities() const override;
            void clear_opportunities() override;

            // Specific methods for multi-instrument combinations
            void add_predefined_combination(
                const std::string &name,
                const std::vector<InstrumentId> &instruments,
                const std::vector<double> &weights);
            void remove_predefined_combination(const std::string &name);
            std::vector<std::string> get_available_combinations_for_instrument(const InstrumentId &instrument) const;
            void update_correlation_matrix(const MarketSnapshot &snapshot);
            std::vector<InstrumentId> get_highly_correlated_instruments(
                const InstrumentId &target_instrument,
                double min_correlation = 0.7) const;
            double calculate_combination_efficiency(
                const std::vector<InstrumentId> &instruments,
                const std::vector<double> &weights,
                const InstrumentId &target_instrument) const;
        };

        // Comprehensive Enhanced Arbitrage Engine
        class ComprehensiveEnhancedArbitrageEngine : public IArbitrageEngine
        { // engine that combines and coordinates all other engiens
        private:
            std::unique_ptr<ArbitrageEngine> general_engine_;
            std::unique_ptr<TriangularArbitrageEngine> triangular_engine_;
            std::unique_ptr<StatisticalArbitrageEngine> statistical_engine_;
            std::unique_ptr<SpotFundingSyntheticPerpetualEngine> spot_funding_engine_;
            std::unique_ptr<CrossExchangeSyntheticReplicationEngine> cross_exchange_engine_;
            std::unique_ptr<MultiInstrumentSyntheticCombinationsEngine> multi_instrument_engine_;

            ArbitrageParameters params_;
            ArbitrageCallback opportunity_callback_;
            ArbitrageUpdateCallback update_callback_;

            // Consolidation and optimization
            std::vector<ArbitrageOpportunity> consolidate_all_opportunities();
            void rank_opportunities_by_profitability(std::vector<ArbitrageOpportunity> &opportunities);
            void filter_conflicting_opportunities(std::vector<ArbitrageOpportunity> &opportunities);
            void optimize_opportunity_portfolio(std::vector<ArbitrageOpportunity> &opportunities);

        public:
            ComprehensiveEnhancedArbitrageEngine(
                std::unique_ptr<IPricingModel> general_model,
                std::unique_ptr<PerpetualSwapPricingModel> perpetual_model,
                std::unique_ptr<BasketPricingModel> basket_model,
                const ArbitrageParameters &params = ArbitrageParameters{});

            void update_market_data(const MarketSnapshot &snapshot) override;
            void process_mispricing(const MispricingOpportunity &mispricing) override;
            std::vector<ArbitrageOpportunity> identify_opportunities() override;
            bool validate_opportunity(ArbitrageOpportunity &opportunity) override;

            void set_opportunity_callback(ArbitrageCallback callback) override;
            void set_update_callback(ArbitrageUpdateCallback callback) override;
            void update_parameters(const ArbitrageParameters &params) override;

            std::vector<ArbitrageOpportunity> get_active_opportunities() const override;
            void clear_opportunities() override;

            // Comprehensive reporting
            std::vector<ArbitrageOpportunity> get_opportunities_by_type(ArbitrageType type) const;
            std::map<ArbitrageType, size_t> get_opportunity_count_by_type() const;
            double get_total_expected_profit() const;
            double get_total_capital_required() const;
            std::vector<ArbitrageOpportunity> get_top_opportunities(size_t count = 10) const;

            // Configuration
            void enable_engine_type(ArbitrageType type, bool enabled);
            void configure_spot_funding_pairs(const std::vector<std::pair<InstrumentId, InstrumentId>> &pairs);
            void configure_cross_exchange_instruments(const std::map<InstrumentId, std::vector<std::string>> &mapping);
            void configure_multi_instrument_combinations(
                const std::map<std::string, std::vector<InstrumentId>> &combinations);
        };

    } // namespace arbitrage
} // namespace spe
