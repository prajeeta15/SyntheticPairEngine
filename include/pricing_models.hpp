#pragma once

#include "market_data.hpp"
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <optional>

namespace spe {
namespace pricing {

using namespace market_data;

struct PricingParameters {
    double correlation_threshold = 0.8;
    double volatility_adjustment = 0.05;
    double liquidity_penalty = 0.001;
    double transaction_cost = 0.0001;
    size_t lookback_period = 100;
    double confidence_interval = 0.95;
};

struct SyntheticPrice {
    Price theoretical_price;
    Price bid_price;
    Price ask_price;
    double confidence_score;
    std::vector<InstrumentId> component_instruments;
    std::vector<double> weights;
    Timestamp calculation_time;
    
    SyntheticPrice() : theoretical_price(0.0), bid_price(0.0), ask_price(0.0), 
                      confidence_score(0.0), calculation_time(std::chrono::high_resolution_clock::now()) {}
};

struct PriceDeviation {
    InstrumentId instrument_id;
    Price market_price;
    Price theoretical_price;
    double deviation_percentage;
    double z_score;
    double confidence_level;
    Timestamp timestamp;
    
    PriceDeviation() : market_price(0.0), theoretical_price(0.0), 
                      deviation_percentage(0.0), z_score(0.0), confidence_level(0.0),
                      timestamp(std::chrono::high_resolution_clock::now()) {}
};

class IPricingModel {
public:
    virtual ~IPricingModel() = default;
    
    virtual SyntheticPrice calculate_synthetic_price(
        const InstrumentId& target_instrument,
        const std::vector<InstrumentId>& component_instruments,
        const MarketSnapshot& market_data) = 0;
        
    virtual std::vector<double> calculate_weights(
        const std::vector<InstrumentId>& instruments,
        const MarketSnapshot& market_data) = 0;
        
    virtual double calculate_correlation(
        const InstrumentId& instrument1,
        const InstrumentId& instrument2,
        const std::vector<Quote>& historical_data) = 0;
        
    virtual void update_parameters(const PricingParameters& params) = 0;
};

// Volatility Surface for Options Pricing
struct VolatilitySurface {
    std::map<std::pair<double, double>, double> vol_surface;  // (strike, time_to_expiry) -> volatility
    
    double interpolate_volatility(double strike, double time_to_expiry) const;
    void update_point(double strike, double time_to_expiry, double volatility);
    double get_atm_volatility(double spot_price, double time_to_expiry) const;
};

// Funding Rate Structure
struct FundingRate {
    InstrumentId instrument_id;
    double rate;
    Timestamp timestamp;
    std::chrono::hours frequency;  // funding frequency
    
    FundingRate() : rate(0.0), timestamp(std::chrono::high_resolution_clock::now()),
                   frequency(std::chrono::hours(8)) {}
};

// Perpetual Swap Pricing Model
class PerpetualSwapPricingModel : public IPricingModel {
private:
    PricingParameters params_;
    std::map<InstrumentId, FundingRate> funding_rates_;
    
    double calculate_funding_component(const InstrumentId& instrument,
                                       const MarketSnapshot& market_data,
                                       const FundingRate& funding_rate);
    
    double calculate_basis_for_perpetuals(const Quote& spot_quote,
                                          const Quote& perpetual_quote,
                                          double funding_rate);
    
    double calculate_perpetual_fair_value(const Quote& spot_quote,
                                          const FundingRate& funding_rate);
    
public:
    PerpetualSwapPricingModel(const PricingParameters& params = PricingParameters{});
    
    SyntheticPrice calculate_synthetic_price(
        const InstrumentId& target_instrument,
        const std::vector<InstrumentId>& component_instruments,
        const MarketSnapshot& market_data) override;
        
    std::vector<double> calculate_weights(
        const std::vector<InstrumentId>& instruments,
        const MarketSnapshot& market_data) override;
        
    double calculate_correlation(
        const InstrumentId& instrument1,
        const InstrumentId& instrument2,
        const std::vector<Quote>& historical_data) override;
        
    void update_parameters(const PricingParameters& params) override;
    
    // Specific methods for perpetual swaps
    void update_funding_rate(const InstrumentId& instrument, const FundingRate& rate);
    double get_current_funding_rate(const InstrumentId& instrument) const;
    double calculate_funding_payment(const InstrumentId& instrument, Volume position_size) const;
};

// Futures Pricing Model with Cost of Carry
class FuturesPricingModel : public IPricingModel {
private:
    PricingParameters params_;
    std::map<InstrumentId, double> interest_rates_;
    std::map<InstrumentId, double> dividend_yields_;
    
    double calculate_cost_of_carry(const InstrumentId& instrument,
                                   const MarketSnapshot& market_data,
                                   double interest_rate,
                                   double dividend_yield);
    
    double calculate_forward_price(const Quote& spot_quote,
                                   double cost_of_carry,
                                   double time_to_maturity);
    
    double get_time_to_maturity(const InstrumentId& instrument) const;
    
public:
    FuturesPricingModel(const PricingParameters& params = PricingParameters{});
    
    SyntheticPrice calculate_synthetic_price(
        const InstrumentId& target_instrument,
        const std::vector<InstrumentId>& component_instruments,
        const MarketSnapshot& market_data) override;
        
    std::vector<double> calculate_weights(
        const std::vector<InstrumentId>& instruments,
        const MarketSnapshot& market_data) override;
        
    double calculate_correlation(
        const InstrumentId& instrument1,
        const InstrumentId& instrument2,
        const std::vector<Quote>& historical_data) override;
        
    void update_parameters(const PricingParameters& params) override;
    
    // Specific methods for futures
    void set_interest_rate(const InstrumentId& instrument, double rate);
    void set_dividend_yield(const InstrumentId& instrument, double yield);
    double calculate_basis(const InstrumentId& futures_instrument, const Quote& spot_quote) const;
};

// Options Pricing Model with Volatility Surface
class OptionsPricingModel : public IPricingModel {
private:
    PricingParameters params_;
    std::map<InstrumentId, VolatilitySurface> volatility_surfaces_;
    std::map<InstrumentId, double> risk_free_rates_;
    
    double interpolate_volatility(const InstrumentId& instrument,
                                  const MarketSnapshot& market_data,
                                  double strike,
                                  double time_to_maturity);
    
    double calculate_black_scholes_price(const Quote& spot_quote,
                                         double strike,
                                         double volatility,
                                         double time_to_maturity,
                                         double risk_free_rate,
                                         bool is_call);
    
    double calculate_delta(const Quote& spot_quote, double strike, double volatility,
                          double time_to_maturity, double risk_free_rate, bool is_call);
    
    double calculate_gamma(const Quote& spot_quote, double strike, double volatility,
                          double time_to_maturity, double risk_free_rate);
    
    double calculate_theta(const Quote& spot_quote, double strike, double volatility,
                          double time_to_maturity, double risk_free_rate, bool is_call);
    
    double calculate_vega(const Quote& spot_quote, double strike, double volatility,
                         double time_to_maturity, double risk_free_rate);
    
public:
    OptionsPricingModel(const PricingParameters& params = PricingParameters{});
    
    SyntheticPrice calculate_synthetic_price(
        const InstrumentId& target_instrument,
        const std::vector<InstrumentId>& component_instruments,
        const MarketSnapshot& market_data) override;
        
    std::vector<double> calculate_weights(
        const std::vector<InstrumentId>& instruments,
        const MarketSnapshot& market_data) override;
        
    double calculate_correlation(
        const InstrumentId& instrument1,
        const InstrumentId& instrument2,
        const std::vector<Quote>& historical_data) override;
        
    void update_parameters(const PricingParameters& params) override;
    
    // Specific methods for options
    void update_volatility_surface(const InstrumentId& instrument, const VolatilitySurface& surface);
    void set_risk_free_rate(const InstrumentId& instrument, double rate);
    double get_implied_volatility(const InstrumentId& option, const Quote& market_quote, const Quote& spot_quote) const;
    std::map<std::string, double> calculate_greeks(const InstrumentId& option, const Quote& spot_quote) const;
};

// Cross-Currency Synthetic Pricing (e.g., EUR/JPY from EUR/USD and USD/JPY)
class CrossCurrencyPricingModel : public IPricingModel {
private:
    PricingParameters params_;
    std::map<std::pair<InstrumentId, InstrumentId>, double> correlation_cache_;
    
    double calculate_cross_rate(const Quote& base_quote, const Quote& quote_quote, bool invert_quote = false);
    double calculate_spread_adjustment(const Quote& base_quote, const Quote& quote_quote);
    
public:
    CrossCurrencyPricingModel(const PricingParameters& params = PricingParameters{});
    
    SyntheticPrice calculate_synthetic_price(
        const InstrumentId& target_instrument,
        const std::vector<InstrumentId>& component_instruments,
        const MarketSnapshot& market_data) override;
        
    std::vector<double> calculate_weights(
        const std::vector<InstrumentId>& instruments,
        const MarketSnapshot& market_data) override;
        
    double calculate_correlation(
        const InstrumentId& instrument1,
        const InstrumentId& instrument2,
        const std::vector<Quote>& historical_data) override;
        
    void update_parameters(const PricingParameters& params) override;
};

// Statistical Arbitrage Pricing Model
class StatisticalArbitragePricingModel : public IPricingModel {
private:
    PricingParameters params_;
    std::map<InstrumentId, std::vector<Price>> price_history_;
    
    double calculate_mean_reversion_price(const InstrumentId& instrument, const std::vector<Price>& prices);
    double calculate_volatility(const std::vector<Price>& prices);
    std::pair<double, double> calculate_bollinger_bands(const std::vector<Price>& prices, double std_dev = 2.0);
    
public:
    StatisticalArbitragePricingModel(const PricingParameters& params = PricingParameters{});
    
    SyntheticPrice calculate_synthetic_price(
        const InstrumentId& target_instrument,
        const std::vector<InstrumentId>& component_instruments,
        const MarketSnapshot& market_data) override;
        
    std::vector<double> calculate_weights(
        const std::vector<InstrumentId>& instruments,
        const MarketSnapshot& market_data) override;
        
    double calculate_correlation(
        const InstrumentId& instrument1,
        const InstrumentId& instrument2,
        const std::vector<Quote>& historical_data) override;
        
    void update_parameters(const PricingParameters& params) override;
    
    void update_price_history(const InstrumentId& instrument, const Quote& quote);
};

// Basket Pricing Model (for multi-instrument synthetics)
class BasketPricingModel : public IPricingModel {
private:
    PricingParameters params_;
    std::map<InstrumentId, double> instrument_weights_;
    
    double calculate_basket_price(const std::vector<InstrumentId>& instruments,
                                 const std::vector<double>& weights,
                                 const MarketSnapshot& market_data);
    double calculate_portfolio_volatility(const std::vector<InstrumentId>& instruments,
                                        const std::vector<double>& weights,
                                        const MarketSnapshot& market_data);
    
public:
    BasketPricingModel(const PricingParameters& params = PricingParameters{});
    
    SyntheticPrice calculate_synthetic_price(
        const InstrumentId& target_instrument,
        const std::vector<InstrumentId>& component_instruments,
        const MarketSnapshot& market_data) override;
        
    std::vector<double> calculate_weights(
        const std::vector<InstrumentId>& instruments,
        const MarketSnapshot& market_data) override;
        
    double calculate_correlation(
        const InstrumentId& instrument1,
        const InstrumentId& instrument2,
        const std::vector<Quote>& historical_data) override;
        
    void update_parameters(const PricingParameters& params) override;
    
    void set_instrument_weights(const std::map<InstrumentId, double>& weights);
};

} // namespace pricing
} // namespace spe
