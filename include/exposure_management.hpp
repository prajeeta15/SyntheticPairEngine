#pragma once

#include "market_data.hpp"
#include "pricing_models.hpp"
#include "arbitrage_engine.hpp"
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>
#include <chrono>

namespace spe
{
    namespace exposure
    {

        using namespace market_data;
        using namespace pricing;
        using namespace arbitrage;

        enum class DerivativeType
        {
            FORWARD,
            FUTURES,
            OPTION_CALL,
            OPTION_PUT,
            SWAP,
            SYNTHETIC_FORWARD,
            SYNTHETIC_OPTION,
            SYNTHETIC_SWAP
        };

        enum class PositionSide
        {
            LONG,
            SHORT,
            NEUTRAL
        };

        enum class RiskLevel
        {
            LOW,
            MEDIUM,
            HIGH,
            EXTREME
        };

        struct SyntheticDerivative // synthetic derivative instrument with its properties and risk metrics
        {
            std::string derivative_id;
            DerivativeType type;
            InstrumentId underlying_instrument;
            std::vector<InstrumentId> component_instruments;
            std::vector<double> component_weights;
            std::vector<Volume> component_sizes;

            Price strike_price;
            Timestamp expiry_time;
            double implied_volatility;
            double time_to_expiry;

            // Greeks
            double delta;
            double gamma;
            double theta;
            double vega;
            double rho;

            // Construction metadata
            Price theoretical_price;
            Price market_price;
            double construction_cost;
            double maintenance_cost;
            double profit_potential;
            Timestamp creation_time;

            SyntheticDerivative() : strike_price(0.0), implied_volatility(0.0), time_to_expiry(0.0),
                                    delta(0.0), gamma(0.0), theta(0.0), vega(0.0), rho(0.0),
                                    theoretical_price(0.0), market_price(0.0), construction_cost(0.0),
                                    maintenance_cost(0.0), profit_potential(0.0),
                                    creation_time(std::chrono::high_resolution_clock::now()) {}
        };

        struct Position // trading position with associated risk metrics
        {
            std::string position_id;
            InstrumentId instrument_id;
            PositionSide side;
            Volume size;
            Price entry_price;
            Price current_price;
            Price unrealized_pnl;
            Price realized_pnl;

            // Risk metrics
            double value_at_risk;
            double expected_shortfall;
            double maximum_drawdown;
            double exposure_amount;
            double margin_requirement;

            // Timing
            Timestamp entry_time;
            Timestamp last_update;

            Position() : size(0.0), entry_price(0.0), current_price(0.0),
                         unrealized_pnl(0.0), realized_pnl(0.0), value_at_risk(0.0),
                         expected_shortfall(0.0), maximum_drawdown(0.0), exposure_amount(0.0),
                         margin_requirement(0.0), entry_time(std::chrono::high_resolution_clock::now()),
                         last_update(std::chrono::high_resolution_clock::now()) {}
        };

        struct Portfolio // a collection of positions and synthetic derivatives with aggregate risk metrics
        {
            std::string portfolio_id;
            std::vector<Position> positions;
            std::vector<SyntheticDerivative> synthetic_derivatives;

            // Aggregate metrics
            double total_exposure;
            double net_exposure;
            double gross_exposure;
            double total_pnl;
            double total_var;
            double portfolio_beta;
            double sharpe_ratio;
            double correlation_risk;

            // Risk limits
            double max_position_size;
            double max_portfolio_var;
            double max_correlation_exposure;
            double max_sector_concentration;

            Portfolio() : total_exposure(0.0), net_exposure(0.0), gross_exposure(0.0),
                          total_pnl(0.0), total_var(0.0), portfolio_beta(0.0),
                          sharpe_ratio(0.0), correlation_risk(0.0), max_position_size(1000000.0),
                          max_portfolio_var(100000.0), max_correlation_exposure(0.3),
                          max_sector_concentration(0.25) {}
        };

        struct RiskParameters // defines various risk limits and thresholds for risk management
        {
            double max_position_size_percentage = 0.05; // 5% of portfolio
            double max_portfolio_var = 0.02;            // 2% VaR
            double max_individual_var = 0.01;           // 1% individual position VaR
            double max_correlation_risk = 0.3;          // 30% correlation exposure
            double max_leverage = 3.0;                  // 3:1 leverage
            double margin_requirement_multiplier = 1.2; // 20% margin buffer
            double stop_loss_percentage = 0.05;         // 5% stop loss
            double take_profit_percentage = 0.15;       // 15% take profit
            double max_drawdown_threshold = 0.1;        // 10% max drawdown
            double liquidity_requirement = 0.8;         // 80% liquidity requirement
        };

        class ISyntheticDerivativeConstructor // constructs synthetic derivative instruments based on market data
        {
        public:
            virtual ~ISyntheticDerivativeConstructor() = default;

            virtual SyntheticDerivative construct_synthetic_forward(
                const InstrumentId &underlying,
                Price strike,
                Timestamp expiry,
                const MarketSnapshot &market_data) = 0;

            virtual SyntheticDerivative construct_synthetic_option(
                const InstrumentId &underlying,
                DerivativeType option_type,
                Price strike,
                Timestamp expiry,
                const MarketSnapshot &market_data) = 0;

            virtual SyntheticDerivative construct_synthetic_swap(
                const InstrumentId &pay_leg,
                const InstrumentId &receive_leg,
                Timestamp expiry,
                const MarketSnapshot &market_data) = 0;

            virtual double calculate_construction_cost(const SyntheticDerivative &derivative) = 0;
            virtual void update_greeks(SyntheticDerivative &derivative, const MarketSnapshot &market_data) = 0;
        };

        class IPositionSizer // sizing trading positions based on market data and risk parameters
        {
        public:
            virtual ~IPositionSizer() = default;

            virtual Volume calculate_optimal_position_size(
                const ArbitrageOpportunity &opportunity,
                const Portfolio &portfolio,
                const RiskParameters &risk_params) = 0;

            virtual Volume calculate_kelly_size(
                double expected_return,
                double volatility,
                double win_probability,
                double portfolio_value) = 0;

            virtual Volume calculate_var_based_size(
                double value_at_risk,
                double max_var_limit,
                double portfolio_value) = 0;

            virtual Volume calculate_volatility_adjusted_size(
                double volatility,
                double target_volatility,
                double base_size) = 0;
        };

        class IRiskCalculator // calculates risk metrics for trading positions and portfolios
        {
        public:
            virtual ~IRiskCalculator() = default;

            virtual double calculate_value_at_risk(
                const Position &position,
                double confidence_level = 0.95,
                size_t time_horizon_days = 1) = 0;

            virtual double calculate_expected_shortfall(
                const Position &position,
                double confidence_level = 0.95) = 0;

            virtual double calculate_portfolio_var(
                const Portfolio &portfolio,
                double confidence_level = 0.95) = 0;

            virtual double calculate_correlation_risk(
                const std::vector<Position> &positions,
                const MarketSnapshot &market_data) = 0;

            virtual double calculate_maximum_drawdown(
                const std::vector<double> &pnl_history) = 0;

            virtual RiskLevel assess_risk_level(
                const Position &position,
                const RiskParameters &params) = 0;
            double calculate_funding_rate_impact(
                const Portfolio\&portfolio,
                const MarketSnapshot\&market_data,
                const std::map<InstrumentId, double> &funding_rates);

            double evaluate_liquidity_across_legs(
                const std::vector<ArbitrageLeg> &legs,
                const MarketSnapshot &market_data,
                double liquidity_threshold);

            std::pair<double, double> assess_correlation_and_basis_risk(
                const std::vector<Position> &positions,
                const MarketSnapshot &market_data);
        };

        class SyntheticDerivativeConstructor : public ISyntheticDerivativeConstructor       // constructor for synthetic derivative instruments
        {
        private:
            std::unique_ptr<IPricingModel> pricing_model_;
            RiskParameters risk_params_;

            // Helper methods
            std::vector<double> optimize_component_weights(
                const std::vector<InstrumentId> &components,
                const MarketSnapshot &market_data,
                DerivativeType target_type);

            double calculate_implied_volatility_from_components(
                const std::vector<InstrumentId> &components,
                const std::vector<double> &weights,
                const MarketSnapshot &market_data);

            void calculate_all_greeks(
                SyntheticDerivative &derivative,
                const MarketSnapshot &market_data);

        public:
            SyntheticDerivativeConstructor(std::unique_ptr<IPricingModel> model,
                                           const RiskParameters &params = RiskParameters{});

            SyntheticDerivative construct_synthetic_forward(
                const InstrumentId &underlying,
                Price strike,
                Timestamp expiry,
                const MarketSnapshot &market_data) override;

            SyntheticDerivative construct_synthetic_option(
                const InstrumentId &underlying,
                DerivativeType option_type,
                Price strike,
                Timestamp expiry,
                const MarketSnapshot &market_data) override;

            SyntheticDerivative construct_synthetic_swap(
                const InstrumentId &pay_leg,
                const InstrumentId &receive_leg,
                Timestamp expiry,
                const MarketSnapshot &market_data) override;

            double calculate_construction_cost(const SyntheticDerivative &derivative) override;
            void update_greeks(SyntheticDerivative &derivative, const MarketSnapshot &market_data) override;

            // Additional methods
            std::vector<SyntheticDerivative> construct_hedge_portfolio(
                const Position &position,
                const MarketSnapshot &market_data);

            SyntheticDerivative optimize_synthetic_construction(
                DerivativeType type,
                const InstrumentId &underlying,
                const MarketSnapshot &market_data);
        };

        class OptimalPositionSizer : public IPositionSizer 
        {
        private:
            RiskParameters risk_params_;

            // Optimization methods
            Volume calculate_risk_parity_size(
                const ArbitrageOpportunity &opportunity,
                const Portfolio &portfolio);

            Volume calculate_sharpe_optimal_size(
                const ArbitrageOpportunity &opportunity,
                const Portfolio &portfolio);

            Volume apply_risk_constraints(
                Volume proposed_size,
                const ArbitrageOpportunity &opportunity,
                const Portfolio &portfolio);

        public:
            OptimalPositionSizer(const RiskParameters &params = RiskParameters{});

            Volume calculate_optimal_position_size(
                const ArbitrageOpportunity &opportunity,
                const Portfolio &portfolio,
                const RiskParameters &risk_params) override;

            Volume calculate_kelly_size(
                double expected_return,
                double volatility,
                double win_probability,
                double portfolio_value) override;

            Volume calculate_var_based_size(
                double value_at_risk,
                double max_var_limit,
                double portfolio_value) override;

            Volume calculate_volatility_adjusted_size(
                double volatility,
                double target_volatility,
                double base_size) override;

            // Additional methods
            Volume calculate_leverage_adjusted_size(
                Volume base_size,
                double current_leverage,
                double max_leverage);

            Volume calculate_correlation_adjusted_size(
                Volume base_size,
                double correlation_exposure,
                double max_correlation);
        };

        class AdvancedRiskCalculator : public IRiskCalculator // implements advanced risk calculations
        {
        private:
            std::map<InstrumentId, std::vector<Price>> price_history_;
            std::map<std::pair<InstrumentId, InstrumentId>, double> correlation_matrix_;

            // Monte Carlo simulation methods
            std::vector<double> monte_carlo_simulation(
                const Position &position,
                size_t num_simulations = 10000);

            double calculate_parametric_var(
                const Position &position,
                double confidence_level);

            double calculate_historical_var(
                const Position &position,
                double confidence_level);

            void update_correlation_matrix(const MarketSnapshot &market_data);

        public:
            AdvancedRiskCalculator();

            double calculate_value_at_risk(
                const Position &position,
                double confidence_level = 0.95,
                size_t time_horizon_days = 1) override;

            double calculate_expected_shortfall(
                const Position &position,
                double confidence_level = 0.95) override;

            double calculate_portfolio_var(
                const Portfolio &portfolio,
                double confidence_level = 0.95) override;

            double calculate_correlation_risk(
                const std::vector<Position> &positions,
                const MarketSnapshot &market_data) override;

            double calculate_maximum_drawdown(
                const std::vector<double> &pnl_history) override;

            RiskLevel assess_risk_level(
                const Position &position,
                const RiskParameters &params) override;

            // Additional risk metrics
            double calculate_conditional_var(
                const Position &position,
                double confidence_level = 0.95);

            double calculate_tail_ratio(const Position &position);
            double calculate_beta(const InstrumentId &instrument, const InstrumentId &benchmark);
            double calculate_tracking_error(const Portfolio &portfolio, const InstrumentId &benchmark);

            void update_price_history(const MarketSnapshot &market_data);
        };

        class ArbitrageLegOptimizer // optimizes arbitrage legs for various objectives
        {
        private:
            std::unique_ptr<IPositionSizer> position_sizer_;
            std::unique_ptr<IRiskCalculator> risk_calculator_;
            RiskParameters risk_params_;

            // Optimization methods
            std::vector<ArbitrageLeg> optimize_leg_weights(
                const std::vector<ArbitrageLeg> &initial_legs,
                const MarketSnapshot &market_data);

            std::vector<ArbitrageLeg> optimize_execution_timing(
                const std::vector<ArbitrageLeg> &legs,
                const MarketSnapshot &market_data);

            double calculate_leg_efficiency(
                const ArbitrageLeg &leg,
                const MarketSnapshot &market_data);

            std::vector<ArbitrageLeg> balance_risk_across_legs(
                const std::vector<ArbitrageLeg> &legs,
                const Portfolio &portfolio);

        public:
            ArbitrageLegOptimizer(std::unique_ptr<IPositionSizer> sizer,
                                  std::unique_ptr<IRiskCalculator> calculator,
                                  const RiskParameters &params = RiskParameters{});

            std::vector<ArbitrageLeg> optimize_arbitrage_legs(
                const ArbitrageOpportunity &opportunity,
                const Portfolio &portfolio,
                const MarketSnapshot &market_data);

            double calculate_optimal_hedge_ratio(
                const InstrumentId &instrument1,
                const InstrumentId &instrument2,
                const MarketSnapshot &market_data);

            std::vector<ArbitrageLeg> create_delta_neutral_legs(
                const ArbitrageOpportunity &opportunity,
                const MarketSnapshot &market_data);

            std::vector<ArbitrageLeg> minimize_transaction_costs(
                const std::vector<ArbitrageLeg> &legs,
                const MarketSnapshot &market_data);

            double calculate_legs_correlation_risk(
                const std::vector<ArbitrageLeg> &legs,
                const MarketSnapshot &market_data);

            // Enhanced optimization methods
            std::vector<ArbitrageLeg> construct_multi_leg_position(
                const ArbitrageOpportunity &opportunity,
                const std::vector<InstrumentId> &instruments,
                const MarketSnapshot &market_data);

            std::vector<ArbitrageLeg> optimize_capital_efficiency(
                const std::vector<ArbitrageLeg> &initial_legs,
                const MarketSnapshot &market_data);

            std::vector<ArbitrageLeg> maximize_risk_adjusted_return(
                const std::vector<ArbitrageLeg> &legs,
                const MarketSnapshot &market_data);

            double calculate_portfolio_sharpe_ratio(
                const std::vector<ArbitrageLeg> &legs,
                const MarketSnapshot &market_data);

            double calculate_capital_efficiency_ratio(
                const std::vector<ArbitrageLeg> &legs,
                const MarketSnapshot &market_data);

            std::vector<ArbitrageLeg> optimize_risk_return_profile(
                const std::vector<ArbitrageLeg> &legs,
                double target_return,
                double max_risk,
                const MarketSnapshot &market_data);

            double calculate_information_ratio(
                const std::vector<ArbitrageLeg> &legs,
                const MarketSnapshot &market_data);

            std::vector<ArbitrageLeg> apply_black_litterman_optimization(
                const std::vector<ArbitrageLeg> &legs,
                const std::vector<double> &expected_returns,
                const MarketSnapshot &market_data);

        public:
            // Enhanced public methods
            std::vector<ArbitrageLeg> construct_optimal_multi_leg_strategy(
                const ArbitrageOpportunity &opportunity,
                const Portfolio &portfolio,
                const MarketSnapshot &market_data,
                const std::string &optimization_objective = "risk_adjusted_return");

            std::vector<ArbitrageLeg> optimize_for_capital_efficiency(
                const ArbitrageOpportunity &opportunity,
                double available_capital,
                const MarketSnapshot &market_data);

            std::vector<ArbitrageLeg> maximize_sharpe_ratio(
                const ArbitrageOpportunity &opportunity,
                const Portfolio &portfolio,
                const MarketSnapshot &market_data);

            double calculate_strategy_efficiency_metrics(
                const std::vector<ArbitrageLeg> &legs,
                const MarketSnapshot &market_data);

            std::map<std::string, double> get_optimization_metrics(
                const std::vector<ArbitrageLeg> &legs,
                const MarketSnapshot &market_data);

            std::vector<ArbitrageLeg> rebalance_for_optimal_allocation(
                const std::vector<ArbitrageLeg> &current_legs,
                const MarketSnapshot &market_data,
                double rebalancing_threshold = 0.05);

            std::vector<ArbitrageLeg> apply_kelly_criterion_sizing(
                const std::vector<ArbitrageLeg> &legs,
                const std::vector<double> &win_probabilities,
                const std::vector<double> &expected_returns,
                const MarketSnapshot &market_data);
        };

        class SyntheticExposureManager // Main class that manages synthetic exposures integrating all other components
        {
        private:
            std::unique_ptr<ISyntheticDerivativeConstructor> derivative_constructor_;
            std::unique_ptr<IPositionSizer> position_sizer_;
            std::unique_ptr<IRiskCalculator> risk_calculator_;
            std::unique_ptr<ArbitrageLegOptimizer> leg_optimizer_;

            Portfolio portfolio_;
            RiskParameters risk_params_;
            mutable std::mutex portfolio_mutex_;

            // Internal methods
            void update_portfolio_metrics();
            void check_risk_limits();
            void rebalance_if_needed(const MarketSnapshot &market_data);
            std::string generate_position_id();
            std::string generate_derivative_id();

        public:
            SyntheticExposureManager(
                std::unique_ptr<ISyntheticDerivativeConstructor> constructor,
                std::unique_ptr<IPositionSizer> sizer,
                std::unique_ptr<IRiskCalculator> calculator,
                std::unique_ptr<ArbitrageLegOptimizer> optimizer,
                const RiskParameters &params = RiskParameters{});

            // Core functionality
            std::string add_synthetic_derivative(
                DerivativeType type,
                const InstrumentId &underlying,
                const MarketSnapshot &market_data);

            std::string execute_arbitrage_opportunity(
                const ArbitrageOpportunity &opportunity,
                const MarketSnapshot &market_data);

            void update_market_data(const MarketSnapshot &market_data);
            void close_position(const std::string &position_id);
            void hedge_position(const std::string &position_id, const MarketSnapshot &market_data);

            // Portfolio management
            Portfolio get_portfolio() const;
            void set_risk_parameters(const RiskParameters &params);
            std::vector<Position> get_positions_by_risk_level(RiskLevel level) const;

            // Risk management
            bool validate_new_position(
                const ArbitrageOpportunity &opportunity,
                Volume proposed_size) const;

            std::vector<std::string> get_risk_violations() const;
            void emergency_risk_reduction();

            // Reporting
            double get_total_pnl() const;
            double get_portfolio_var() const;
            double get_portfolio_exposure() const;
            std::map<std::string, double> get_risk_metrics() const;
        };

    } // namespace exposure
} // namespace spe
