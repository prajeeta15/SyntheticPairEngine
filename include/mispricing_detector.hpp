#pragma once

#include "market_data.hpp"
#include "pricing_models.hpp"
#include <vector>
#include <queue>
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>
#include <chrono>
#include <set>

namespace spe {
namespace mispricing {

using namespace market_data;
using namespace pricing;

enum class MispricingType {
    STATISTICAL_ARBITRAGE,
    CROSS_CURRENCY_TRIANGULAR,
    MEAN_REVERSION,
    VOLATILITY_ARBITRAGE,
    SPREAD_ANOMALY,
    SPOT_VS_SYNTHETIC_DERIVATIVE,
    CROSS_EXCHANGE_ARBITRAGE,
    REAL_TIME_PRICE_DISCREPANCY
};

enum class MispricingSeverity {
    LOW,
    MEDIUM,
    HIGH,
    CRITICAL
};

struct MispricingOpportunity {
    InstrumentId target_instrument;
    std::vector<InstrumentId> component_instruments;
    MispricingType type;
    MispricingSeverity severity;
    
    Price market_price;
    Price theoretical_price;
    double deviation_percentage;
    double z_score;
    double confidence_level;
    double expected_profit;
    double max_loss;
    
    std::vector<double> weights;
    Timestamp detection_time;
    Timestamp expiry_time;
    
    // Risk metrics
    double value_at_risk;
    double expected_shortfall;
    double sharpe_ratio;
    
    MispricingOpportunity() : market_price(0.0), theoretical_price(0.0), 
                             deviation_percentage(0.0), z_score(0.0), confidence_level(0.0),
                             expected_profit(0.0), max_loss(0.0), value_at_risk(0.0),
                             expected_shortfall(0.0), sharpe_ratio(0.0),
                             detection_time(std::chrono::high_resolution_clock::now()) {}
};

struct DetectionParameters {
    double min_deviation_threshold = 0.005;  // 0.5%
    double min_z_score = 2.0;
    double min_confidence_level = 0.8;
    double max_spread_ratio = 0.02;  // 2%
    size_t min_observation_window = 50;
    double volatility_threshold = 0.15;
    double liquidity_threshold = 1000.0;
    std::chrono::minutes max_opportunity_duration = std::chrono::minutes(30);
};

using MispricingCallback = std::function<void(const MispricingOpportunity&)>;
using MispricingExpiredCallback = std::function<void(const MispricingOpportunity&)>;

class IMispricingDetector {
public:
    virtual ~IMispricingDetector() = default;
    
    virtual void update_market_data(const MarketSnapshot& snapshot) = 0;
    virtual std::vector<MispricingOpportunity> detect_opportunities() = 0;
    virtual void set_detection_callback(MispricingCallback callback) = 0;
    virtual void set_expiry_callback(MispricingExpiredCallback callback) = 0;
    virtual void update_parameters(const DetectionParameters& params) = 0;
};

class StatisticalMispricingDetector : public IMispricingDetector {
private:
    DetectionParameters params_;
    std::unique_ptr<IPricingModel> pricing_model_;
    
    // Historical data storage
    std::map<InstrumentId, std::queue<Quote>> price_history_;
    std::map<InstrumentId, std::queue<double>> deviation_history_;
    
    // Active opportunities tracking
    std::vector<MispricingOpportunity> active_opportunities_;
    mutable std::mutex opportunities_mutex_;
    
    // Callbacks
    MispricingCallback detection_callback_;
    MispricingExpiredCallback expiry_callback_;
    
    // Detection methods
    bool is_significant_deviation(double deviation, double z_score, double confidence);
    double calculate_z_score(const std::queue<double>& history, double current_value);
    double calculate_confidence_level(const std::queue<Quote>& history, double theoretical_price);
    MispricingSeverity assess_severity(const PriceDeviation& deviation);
    
    void cleanup_expired_opportunities();
    void update_price_history(const InstrumentId& instrument, const Quote& quote);
    
public:
    StatisticalMispricingDetector(std::unique_ptr<IPricingModel> model, 
                                 const DetectionParameters& params = DetectionParameters{});
    ~StatisticalMispricingDetector();
    
    void update_market_data(const MarketSnapshot& snapshot) override;
    std::vector<MispricingOpportunity> detect_opportunities() override;
    void set_detection_callback(MispricingCallback callback) override;
    void set_expiry_callback(MispricingExpiredCallback callback) override;
    void update_parameters(const DetectionParameters& params) override;
    
    // Additional methods
    std::vector<MispricingOpportunity> get_active_opportunities() const;
    void clear_opportunities();
};

class TriangularArbitrageDetector : public IMispricingDetector {
private:
    DetectionParameters params_;
    std::map<std::string, std::vector<InstrumentId>> currency_triangles_;
    
    MispricingCallback detection_callback_;
    MispricingExpiredCallback expiry_callback_;
    
    std::vector<MispricingOpportunity> detect_triangular_opportunities(const MarketSnapshot& snapshot);
    double calculate_triangular_profit(const Quote& pair1, const Quote& pair2, const Quote& pair3);
    bool is_profitable_triangle(double profit_percentage);
    
public:
    TriangularArbitrageDetector(const DetectionParameters& params = DetectionParameters{});
    
    void update_market_data(const MarketSnapshot& snapshot) override;
    std::vector<MispricingOpportunity> detect_opportunities() override;
    void set_detection_callback(MispricingCallback callback) override;
    void set_expiry_callback(MispricingExpiredCallback callback) override;
    void update_parameters(const DetectionParameters& params) override;
    
    void add_currency_triangle(const std::string& name, const std::vector<InstrumentId>& instruments);
    void remove_currency_triangle(const std::string& name);
};

class VolatilityArbitrageDetector : public IMispricingDetector {
private:
    DetectionParameters params_;
    std::map<InstrumentId, std::queue<Price>> volatility_history_;
    
    MispricingCallback detection_callback_;
    MispricingExpiredCallback expiry_callback_;
    
    double calculate_realized_volatility(const std::queue<Price>& prices);
    double calculate_implied_volatility_proxy(const Quote& quote);
    std::vector<MispricingOpportunity> detect_volatility_opportunities(const MarketSnapshot& snapshot);
    
public:
    VolatilityArbitrageDetector(const DetectionParameters& params = DetectionParameters{});
    
    void update_market_data(const MarketSnapshot& snapshot) override;
    std::vector<MispricingOpportunity> detect_opportunities() override;
    void set_detection_callback(MispricingCallback callback) override;
    void set_expiry_callback(MispricingExpiredCallback callback) override;
    void update_parameters(const DetectionParameters& params) override;
};

class CompositeMispricingDetector : public IMispricingDetector {
private:
    std::vector<std::unique_ptr<IMispricingDetector>> detectors_;
    DetectionParameters params_;
    
    MispricingCallback detection_callback_;
    MispricingExpiredCallback expiry_callback_;
    
    void consolidate_opportunities(std::vector<MispricingOpportunity>& opportunities);
    
public:
    CompositeMispricingDetector(const DetectionParameters& params = DetectionParameters{});
    
    void add_detector(std::unique_ptr<IMispricingDetector> detector);
    void remove_detector(size_t index);
    
    void update_market_data(const MarketSnapshot& snapshot) override;
    std::vector<MispricingOpportunity> detect_opportunities() override;
    void set_detection_callback(MispricingCallback callback) override;
    void set_expiry_callback(MispricingExpiredCallback callback) override;
    void update_parameters(const DetectionParameters& params) override;
};

// Enhanced structures for real-time detection and profit calculation
struct PriceDiscrepancy {
    InstrumentId instrument_id;
    std::string exchange_id;
    Price spot_price;
    Price synthetic_price;
    double price_difference;
    double percentage_discrepancy;
    double expected_profit_percentage;
    double required_capital;
    double estimated_transaction_cost;
    double net_profit_after_costs;
    Timestamp detection_time;
    std::chrono::milliseconds latency;
    
    PriceDiscrepancy() : spot_price(0.0), synthetic_price(0.0), price_difference(0.0),
                        percentage_discrepancy(0.0), expected_profit_percentage(0.0),
                        required_capital(0.0), estimated_transaction_cost(0.0),
                        net_profit_after_costs(0.0), detection_time(std::chrono::high_resolution_clock::now()) {}
};

struct CrossExchangeOpportunity {
    InstrumentId instrument_id;
    std::string exchange_1;
    std::string exchange_2;
    Price price_1;
    Price price_2;
    double price_spread;
    double percentage_spread;
    double expected_profit;
    double required_capital;
    double capital_efficiency_ratio;
    Volume available_volume;
    double execution_probability;
    Timestamp detection_time;
    std::chrono::milliseconds window_duration;
    
    CrossExchangeOpportunity() : price_1(0.0), price_2(0.0), price_spread(0.0),
                               percentage_spread(0.0), expected_profit(0.0), required_capital(0.0),
                               capital_efficiency_ratio(0.0), available_volume(0.0),
                               execution_probability(0.0), detection_time(std::chrono::high_resolution_clock::now()) {}
};

struct DerivativePricingDiscrepancy {
    InstrumentId spot_instrument;
    InstrumentId derivative_instrument;
    Price spot_price;
    Price derivative_market_price;
    Price derivative_theoretical_price;
    double fair_value_deviation;
    double implied_volatility;
    double time_to_expiry;
    double delta;
    double gamma;
    double theta;
    double expected_profit;
    double required_margin;
    double profit_to_margin_ratio;
    Timestamp detection_time;
    
    DerivativePricingDiscrepancy() : spot_price(0.0), derivative_market_price(0.0),
                                   derivative_theoretical_price(0.0), fair_value_deviation(0.0),
                                   implied_volatility(0.0), time_to_expiry(0.0), delta(0.0),
                                   gamma(0.0), theta(0.0), expected_profit(0.0), required_margin(0.0),
                                   profit_to_margin_ratio(0.0), detection_time(std::chrono::high_resolution_clock::now()) {}
};

// Real-time price discrepancy detector
class RealTimePriceDiscrepancyDetector : public IMispricingDetector {
private:
    DetectionParameters params_;
    std::map<std::string, MarketSnapshot> exchange_snapshots_;
    std::vector<PriceDiscrepancy> active_discrepancies_;
    mutable std::mutex discrepancies_mutex_;
    
    MispricingCallback detection_callback_;
    MispricingExpiredCallback expiry_callback_;
    
    // Real-time detection methods
    std::vector<PriceDiscrepancy> detect_price_discrepancies(const MarketSnapshot& snapshot);
    double calculate_expected_profit(const PriceDiscrepancy& discrepancy);
    double calculate_required_capital(const PriceDiscrepancy& discrepancy);
    bool is_actionable_discrepancy(const PriceDiscrepancy& discrepancy);
    
public:
    RealTimePriceDiscrepancyDetector(const DetectionParameters& params = DetectionParameters{});
    
    void update_market_data(const MarketSnapshot& snapshot) override;
    std::vector<MispricingOpportunity> detect_opportunities() override;
    void set_detection_callback(MispricingCallback callback) override;
    void set_expiry_callback(MispricingExpiredCallback callback) override;
    void update_parameters(const DetectionParameters& params) override;
    
    // Enhanced methods
    void add_exchange_feed(const std::string& exchange_id, const MarketSnapshot& snapshot);
    std::vector<PriceDiscrepancy> get_active_discrepancies() const;
    void clear_discrepancies();
};

// Spot vs Synthetic Derivative Detector
class SpotVsSyntheticDerivativeDetector : public IMispricingDetector {
private:
    DetectionParameters params_;
    std::unique_ptr<IPricingModel> derivative_pricing_model_;
    std::vector<DerivativePricingDiscrepancy> active_discrepancies_;
    mutable std::mutex discrepancies_mutex_;
    
    MispricingCallback detection_callback_;
    MispricingExpiredCallback expiry_callback_;
    
    // Derivative pricing methods
    std::vector<DerivativePricingDiscrepancy> detect_derivative_mispricings(const MarketSnapshot& snapshot);
    double calculate_theoretical_derivative_price(const InstrumentId& derivative, const Quote& spot_quote);
    double calculate_implied_volatility(const Quote& derivative_quote, const Quote& spot_quote);
    double calculate_greeks(const InstrumentId& derivative, const Quote& spot_quote);
    double calculate_margin_requirement(const DerivativePricingDiscrepancy& discrepancy);
    
public:
    SpotVsSyntheticDerivativeDetector(std::unique_ptr<IPricingModel> model,
                                     const DetectionParameters& params = DetectionParameters{});
    
    void update_market_data(const MarketSnapshot& snapshot) override;
    std::vector<MispricingOpportunity> detect_opportunities() override;
    void set_detection_callback(MispricingCallback callback) override;
    void set_expiry_callback(MispricingExpiredCallback callback) override;
    void update_parameters(const DetectionParameters& params) override;
    
    // Enhanced methods
    std::vector<DerivativePricingDiscrepancy> get_active_derivative_discrepancies() const;
    void add_derivative_instrument(const InstrumentId& derivative_id, const InstrumentId& underlying_id);
};

// Cross-Exchange Arbitrage Detector
class CrossExchangeArbitrageDetector : public IMispricingDetector {
private:
    DetectionParameters params_;
    std::map<std::string, MarketSnapshot> exchange_feeds_;
    std::vector<CrossExchangeOpportunity> active_opportunities_;
    mutable std::mutex opportunities_mutex_;
    
    MispricingCallback detection_callback_;
    MispricingExpiredCallback expiry_callback_;
    
    // Cross-exchange detection methods
    std::vector<CrossExchangeOpportunity> detect_cross_exchange_opportunities();
    double calculate_execution_probability(const CrossExchangeOpportunity& opportunity);
    double calculate_capital_efficiency(const CrossExchangeOpportunity& opportunity);
    bool validate_liquidity_constraints(const CrossExchangeOpportunity& opportunity);
    double estimate_execution_costs(const CrossExchangeOpportunity& opportunity);
    
public:
    CrossExchangeArbitrageDetector(const DetectionParameters& params = DetectionParameters{});
    
    void update_market_data(const MarketSnapshot& snapshot) override;
    std::vector<MispricingOpportunity> detect_opportunities() override;
    void set_detection_callback(MispricingCallback callback) override;
    void set_expiry_callback(MispricingExpiredCallback callback) override;
    void update_parameters(const DetectionParameters& params) override;
    
    // Enhanced methods
    void register_exchange(const std::string& exchange_id);
    void update_exchange_data(const std::string& exchange_id, const MarketSnapshot& snapshot);
    std::vector<CrossExchangeOpportunity> get_active_cross_exchange_opportunities() const;
    void set_exchange_transaction_costs(const std::string& exchange_id, double cost_percentage);
};

// Enhanced Composite Detector with Profit/Capital Analysis
class EnhancedCompositeMispricingDetector : public IMispricingDetector {
private:
    std::vector<std::unique_ptr<IMispricingDetector>> detectors_;
    DetectionParameters params_;
    
    // Enhanced tracking
    std::vector<PriceDiscrepancy> price_discrepancies_;
    std::vector<CrossExchangeOpportunity> cross_exchange_opportunities_;
    std::vector<DerivativePricingDiscrepancy> derivative_discrepancies_;
    
    MispricingCallback detection_callback_;
    MispricingExpiredCallback expiry_callback_;
    
    // Analysis methods
    void analyze_profit_potential(std::vector<MispricingOpportunity>& opportunities);
    void calculate_capital_requirements(std::vector<MispricingOpportunity>& opportunities);
    void rank_opportunities_by_efficiency(std::vector<MispricingOpportunity>& opportunities);
    
public:
    EnhancedCompositeMispricingDetector(const DetectionParameters& params = DetectionParameters{});
    
    void add_detector(std::unique_ptr<IMispricingDetector> detector);
    void remove_detector(size_t index);
    
    void update_market_data(const MarketSnapshot& snapshot) override;
    std::vector<MispricingOpportunity> detect_opportunities() override;
    void set_detection_callback(MispricingCallback callback) override;
    void set_expiry_callback(MispricingExpiredCallback callback) override;
    void update_parameters(const DetectionParameters& params) override;
    
    // Enhanced reporting
    std::vector<PriceDiscrepancy> get_real_time_discrepancies() const;
    std::vector<CrossExchangeOpportunity> get_cross_exchange_opportunities() const;
    std::vector<DerivativePricingDiscrepancy> get_derivative_discrepancies() const;
    double get_total_profit_potential() const;
    double get_total_capital_required() const;
    double get_portfolio_efficiency_ratio() const;
};

// Real-time Basis Calculation Structure
struct BasisCalculation {
    InstrumentId spot_instrument;
    InstrumentId derivative_instrument;
    Price spot_price;
    Price derivative_price;
    double basis_value;
    double basis_percentage;
    double theoretical_basis;
    double basis_deviation;
    double z_score;
    Timestamp calculation_time;
    std::chrono::milliseconds update_frequency;
    
    BasisCalculation() : spot_price(0.0), derivative_price(0.0), basis_value(0.0),
                        basis_percentage(0.0), theoretical_basis(0.0), basis_deviation(0.0),
                        z_score(0.0), calculation_time(std::chrono::high_resolution_clock::now()) {}
};

// Statistical Arbitrage Signal Structure
struct StatArbitrageSignal {
    InstrumentId instrument_1;
    InstrumentId instrument_2;
    double price_ratio;
    double mean_ratio;
    double ratio_std_dev;
    double z_score;
    double correlation;
    double half_life;
    double signal_strength;
    std::string signal_type;  // "LONG_SPREAD", "SHORT_SPREAD", "NEUTRAL"
    double entry_threshold;
    double exit_threshold;
    double confidence_level;
    Timestamp signal_time;
    
    StatArbitrageSignal() : price_ratio(0.0), mean_ratio(0.0), ratio_std_dev(0.0),
                           z_score(0.0), correlation(0.0), half_life(0.0),
                           signal_strength(0.0), entry_threshold(2.0), exit_threshold(0.5),
                           confidence_level(0.0), signal_time(std::chrono::high_resolution_clock::now()) {}
};

// Real-time Basis Calculator
class RealTimeBasisCalculator : public IMispricingDetector {
private:
    DetectionParameters params_;
    std::map<std::pair<InstrumentId, InstrumentId>, std::queue<BasisCalculation>> basis_history_;
    std::vector<BasisCalculation> active_basis_opportunities_;
    mutable std::mutex basis_mutex_;
    
    MispricingCallback detection_callback_;
    MispricingExpiredCallback expiry_callback_;
    
    // Basis calculation methods
    std::vector<BasisCalculation> calculate_real_time_basis(const MarketSnapshot& snapshot);
    double calculate_theoretical_basis(const InstrumentId& spot, const InstrumentId& derivative,
                                     const MarketSnapshot& snapshot);
    double calculate_basis_z_score(const BasisCalculation& current_basis,
                                  const std::queue<BasisCalculation>& history);
    bool is_significant_basis_deviation(const BasisCalculation& basis);
    void update_basis_history(const BasisCalculation& basis);
    
public:
    RealTimeBasisCalculator(const DetectionParameters& params = DetectionParameters{});
    
    void update_market_data(const MarketSnapshot& snapshot) override;
    std::vector<MispricingOpportunity> detect_opportunities() override;
    void set_detection_callback(MispricingCallback callback) override;
    void set_expiry_callback(MispricingExpiredCallback callback) override;
    void update_parameters(const DetectionParameters& params) override;
    
    // Specific basis calculation methods
    std::vector<BasisCalculation> get_active_basis_opportunities() const;
    void add_instrument_pair(const InstrumentId& spot, const InstrumentId& derivative);
    double get_current_basis(const InstrumentId& spot, const InstrumentId& derivative) const;
    std::vector<BasisCalculation> get_basis_history(const InstrumentId& spot, 
                                                   const InstrumentId& derivative) const;
};

// Statistical Arbitrage Signal Generator
class StatisticalArbitrageSignalGenerator : public IMispricingDetector {
private:
    DetectionParameters params_;
    std::map<std::pair<InstrumentId, InstrumentId>, std::queue<double>> price_ratio_history_;
    std::map<std::pair<InstrumentId, InstrumentId>, double> correlation_cache_;
    std::vector<StatArbitrageSignal> active_signals_;
    mutable std::mutex signals_mutex_;
    
    MispricingCallback detection_callback_;
    MispricingExpiredCallback expiry_callback_;
    
    // Statistical arbitrage methods
    std::vector<StatArbitrageSignal> generate_stat_arb_signals(const MarketSnapshot& snapshot);
    double calculate_price_ratio(const Quote& quote1, const Quote& quote2);
    double calculate_mean_ratio(const std::queue<double>& ratio_history);
    double calculate_ratio_volatility(const std::queue<double>& ratio_history, double mean);
    double calculate_z_score(double current_ratio, double mean_ratio, double std_dev);
    double calculate_correlation(const InstrumentId& instrument1, const InstrumentId& instrument2,
                               const MarketSnapshot& snapshot);
    double calculate_half_life(const std::queue<double>& ratio_history);
    std::string determine_signal_type(double z_score, double entry_threshold);
    double calculate_signal_strength(double z_score, double correlation, double half_life);
    bool is_valid_signal(const StatArbitrageSignal& signal);
    void update_ratio_history(const InstrumentId& instrument1, const InstrumentId& instrument2,
                             double ratio);
    
public:
    StatisticalArbitrageSignalGenerator(const DetectionParameters& params = DetectionParameters{});
    
    void update_market_data(const MarketSnapshot& snapshot) override;
    std::vector<MispricingOpportunity> detect_opportunities() override;
    void set_detection_callback(MispricingCallback callback) override;
    void set_expiry_callback(MispricingExpiredCallback callback) override;
    void update_parameters(const DetectionParameters& params) override;
    
    // Specific statistical arbitrage methods
    std::vector<StatArbitrageSignal> get_active_signals() const;
    void add_instrument_pair(const InstrumentId& instrument1, const InstrumentId& instrument2);
    void set_signal_thresholds(double entry_threshold, double exit_threshold);
    double get_current_z_score(const InstrumentId& instrument1, const InstrumentId& instrument2) const;
    std::map<std::string, double> get_pair_statistics(const InstrumentId& instrument1,
                                                     const InstrumentId& instrument2) const;
};

// Enhanced Cross-Exchange Synthetic Price Comparator
class CrossExchangeSyntheticPriceComparator : public IMispricingDetector {
private:
    DetectionParameters params_;
    std::unique_ptr<IPricingModel> pricing_model_;
    std::map<std::string, MarketSnapshot> exchange_snapshots_;
    std::vector<CrossExchangeOpportunity> active_opportunities_;
    mutable std::mutex opportunities_mutex_;
    
    MispricingCallback detection_callback_;
    MispricingExpiredCallback expiry_callback_;
    
    // Enhanced comparison methods
    std::vector<CrossExchangeOpportunity> compare_synthetic_prices_across_exchanges();
    SyntheticPrice calculate_synthetic_price_for_exchange(const InstrumentId& instrument,
                                                         const std::string& exchange_id);
    double calculate_cross_exchange_spread(const SyntheticPrice& price1, const SyntheticPrice& price2);
    bool validate_synthetic_construction_quality(const SyntheticPrice& synthetic_price);
    double estimate_cross_exchange_execution_cost(const CrossExchangeOpportunity& opportunity);
    double calculate_synthetic_price_confidence(const SyntheticPrice& synthetic_price);
    
public:
    CrossExchangeSyntheticPriceComparator(std::unique_ptr<IPricingModel> model,
                                         const DetectionParameters& params = DetectionParameters{});
    
    void update_market_data(const MarketSnapshot& snapshot) override;
    std::vector<MispricingOpportunity> detect_opportunities() override;
    void set_detection_callback(MispricingCallback callback) override;
    void set_expiry_callback(MispricingExpiredCallback callback) override;
    void update_parameters(const DetectionParameters& params) override;
    
    // Enhanced cross-exchange methods
    void register_exchange_feed(const std::string& exchange_id);
    void update_exchange_snapshot(const std::string& exchange_id, const MarketSnapshot& snapshot);
    std::vector<CrossExchangeOpportunity> get_synthetic_price_opportunities() const;
    SyntheticPrice get_best_synthetic_price(const InstrumentId& instrument) const;
    std::map<std::string, SyntheticPrice> get_all_exchange_synthetic_prices(const InstrumentId& instrument) const;
};

// Comprehensive Enhanced Mispricing Detector
class ComprehensiveEnhancedMispricingDetector : public IMispricingDetector {
private:
    std::unique_ptr<RealTimeBasisCalculator> basis_calculator_;
    std::unique_ptr<StatisticalArbitrageSignalGenerator> stat_arb_generator_;
    std::unique_ptr<CrossExchangeSyntheticPriceComparator> cross_exchange_comparator_;
    std::unique_ptr<EnhancedCompositeMispricingDetector> composite_detector_;
    
    DetectionParameters params_;
    MispricingCallback detection_callback_;
    MispricingExpiredCallback expiry_callback_;
    
    // Consolidation methods
    std::vector<MispricingOpportunity> consolidate_all_opportunities();
    void rank_opportunities_by_priority(std::vector<MispricingOpportunity>& opportunities);
    void filter_duplicate_opportunities(std::vector<MispricingOpportunity>& opportunities);
    
public:
    ComprehensiveEnhancedMispricingDetector(
        std::unique_ptr<IPricingModel> pricing_model,
        const DetectionParameters& params = DetectionParameters{});
    
    void update_market_data(const MarketSnapshot& snapshot) override;
    std::vector<MispricingOpportunity> detect_opportunities() override;
    void set_detection_callback(MispricingCallback callback) override;
    void set_expiry_callback(MispricingExpiredCallback callback) override;
    void update_parameters(const DetectionParameters& params) override;
    
    // Comprehensive reporting
    std::vector<BasisCalculation> get_all_basis_opportunities() const;
    std::vector<StatArbitrageSignal> get_all_stat_arb_signals() const;
    std::vector<CrossExchangeOpportunity> get_all_cross_exchange_opportunities() const;
    
    // Configuration methods
    void add_instrument_pair_for_stat_arb(const InstrumentId& instrument1, const InstrumentId& instrument2);
    void add_derivative_pair_for_basis(const InstrumentId& spot, const InstrumentId& derivative);
    void register_exchange_for_comparison(const std::string& exchange_id);
};

} // namespace mispricing
} // namespace spe
