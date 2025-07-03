#include "pricing_models.hpp"
#include <cmath>
#include <algorithm>

namespace spe
{
    namespace pricing
    {

        // VolatilitySurface implementation for options pricing
        double VolatilitySurface::interpolate_volatility(double strike, double time_to_expiry) const
        {
            // Simple linear interpolation for demo
            auto key = std::make_pair(strike, time_to_expiry);
            auto it = vol_surface.find(key);
            if (it != vol_surface.end())
            {
                return it->second;
            }
            return 0.2; // Default 20% volatility
        }

        void VolatilitySurface::update_point(double strike, double time_to_expiry, double volatility)
        {
            vol_surface[std::make_pair(strike, time_to_expiry)] = volatility;
        }

        double VolatilitySurface::get_atm_volatility(double spot_price, double time_to_expiry) const
        {
            return interpolate_volatility(spot_price, time_to_expiry);
        }

        // PerpetualSwapPricingModel implementation for perpetual swaps
        PerpetualSwapPricingModel::PerpetualSwapPricingModel(const PricingParameters &params)
            : params_(params) {}
// methods for calculating synthetic price, weights, and correlation
        SyntheticPrice PerpetualSwapPricingModel::calculate_synthetic_price(
            const InstrumentId &target_instrument,
            const std::vector<InstrumentId> &component_instruments,
            const MarketSnapshot &market_data)
        {

            SyntheticPrice result;
            result.theoretical_price = 100.0; // Simplified for demo
            result.bid_price = result.theoretical_price - 0.05;
            result.ask_price = result.theoretical_price + 0.05;
            result.confidence_score = 0.85;
            result.component_instruments = component_instruments;
            result.weights = std::vector<double>(component_instruments.size(), 1.0);

            return result;
        }

        std::vector<double> PerpetualSwapPricingModel::calculate_weights(
            const std::vector<InstrumentId> &instruments,
            const MarketSnapshot &market_data)
        {

            return std::vector<double>(instruments.size(), 1.0 / instruments.size());
        }
        
        double PerpetualSwapPricingModel::calculate_correlation(
            const InstrumentId &instrument1,
            const InstrumentId &instrument2,
            const std::vector<Quote> &historical_data)
        {

            return 0.75; // Simplified correlation for demo
        }

        void PerpetualSwapPricingModel::update_parameters(const PricingParameters &params)
        {
            params_ = params;
        }

        void PerpetualSwapPricingModel::update_funding_rate(const InstrumentId &instrument, const FundingRate &rate)
        {
            funding_rates_[instrument] = rate;
        }
// handles funding rate updates and retrieval
        double PerpetualSwapPricingModel::get_current_funding_rate(const InstrumentId &instrument) const
        {
            auto it = funding_rates_.find(instrument);
            return (it != funding_rates_.end()) ? it->second.rate : 0.0001;
        }

        double PerpetualSwapPricingModel::calculate_funding_payment(const InstrumentId &instrument, Volume position_size) const
        {
            double rate = get_current_funding_rate(instrument);
            return rate * position_size;
        }

        double PerpetualSwapPricingModel::calculate_funding_component(const InstrumentId &instrument,
                                                                      const MarketSnapshot &market_data,
                                                                      const FundingRate &funding_rate)
        {
            return funding_rate.rate * 0.1; // Simplified calculation
        }

        double PerpetualSwapPricingModel::calculate_basis_for_perpetuals(const Quote &spot_quote,
                                                                         const Quote &perpetual_quote,
                                                                         double funding_rate)
        {
            return (perpetual_quote.bid_price + perpetual_quote.ask_price) / 2.0 -
                   (spot_quote.bid_price + spot_quote.ask_price) / 2.0;
        }

        double PerpetualSwapPricingModel::calculate_perpetual_fair_value(const Quote &spot_quote,
                                                                         const FundingRate &funding_rate)
        {
            double mid_price = (spot_quote.bid_price + spot_quote.ask_price) / 2.0;
            return mid_price * (1.0 + funding_rate.rate);
        }

        // FuturesPricingModel implementation for futures contracts
        FuturesPricingModel::FuturesPricingModel(const PricingParameters &params)
            : params_(params) {}

        SyntheticPrice FuturesPricingModel::calculate_synthetic_price(
            const InstrumentId &target_instrument,
            const std::vector<InstrumentId> &component_instruments,
            const MarketSnapshot &market_data)
        {

            SyntheticPrice result;
            result.theoretical_price = 101.0; // Simplified for demo
            result.bid_price = result.theoretical_price - 0.05;
            result.ask_price = result.theoretical_price + 0.05;
            result.confidence_score = 0.80;
            result.component_instruments = component_instruments;
            result.weights = std::vector<double>(component_instruments.size(), 1.0);

            return result;
        }

        std::vector<double> FuturesPricingModel::calculate_weights(
            const std::vector<InstrumentId> &instruments,
            const MarketSnapshot &market_data)
        {

            return std::vector<double>(instruments.size(), 1.0 / instruments.size());
        }

        double FuturesPricingModel::calculate_correlation(
            const InstrumentId &instrument1,
            const InstrumentId &instrument2,
            const std::vector<Quote> &historical_data)
        {

            return 0.80; // Simplified correlation for demo
        }
// handles interet rates and dividend yields
        void FuturesPricingModel::update_parameters(const PricingParameters &params)
        {
            params_ = params;
        }

        void FuturesPricingModel::set_interest_rate(const InstrumentId &instrument, double rate)
        {
            interest_rates_[instrument] = rate;
        }

        void FuturesPricingModel::set_dividend_yield(const InstrumentId &instrument, double yield)
        {
            dividend_yields_[instrument] = yield;
        }

        double FuturesPricingModel::calculate_basis(const InstrumentId &futures_instrument, const Quote &spot_quote) const
        {
            return 1.0; // Simplified basis calculation
        }

        double FuturesPricingModel::calculate_cost_of_carry(const InstrumentId &instrument,
                                                            const MarketSnapshot &market_data,
                                                            double interest_rate,
                                                            double dividend_yield)
        {
            return interest_rate - dividend_yield;
        }

        // cost of carry, forward prices and basis for futures
        double FuturesPricingModel::calculate_forward_price(const Quote &spot_quote,
                                                            double cost_of_carry,
                                                            double time_to_maturity)
        {
            double spot_mid = (spot_quote.bid_price + spot_quote.ask_price) / 2.0;
            return spot_mid * std::exp(cost_of_carry * time_to_maturity);
        }

        double FuturesPricingModel::get_time_to_maturity(const InstrumentId &instrument) const
        {
            return 0.25; // 3 months for demo
        }

    } // namespace pricing
} // namespace spe
