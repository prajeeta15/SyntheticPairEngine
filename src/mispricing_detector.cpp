#include "mispricing_detector.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace spe
{
    namespace mispricing
    {

        // StatisticalMispricingDetector implementation
        StatisticalMispricingDetector::StatisticalMispricingDetector(
            std::unique_ptr<IPricingModel> model,
            const DetectionParameters &params)
            : params_(params), pricing_model_(std::move(model)) {}

        StatisticalMispricingDetector::~StatisticalMispricingDetector() = default;
        
        //methods for updating market data and detecting opportunities and managing active differences
        void StatisticalMispricingDetector::update_market_data(const MarketSnapshot &snapshot)
        {
            for (const auto &[instrument_id, quote] : snapshot.quotes)
            {
                update_price_history(instrument_id, quote);
            }
            cleanup_expired_opportunities();
        }
        
        // included calc for z-scores, confidence levels and severity assessment.
        std::vector<MispricingOpportunity> StatisticalMispricingDetector::detect_opportunities()
        {
            std::vector<MispricingOpportunity> opportunities;

            // Simplified detection for demo
            for (const auto &[instrument_id, price_queue] : price_history_)
            {
                if (price_queue.size() >= params_.min_observation_window)
                {
                    MispricingOpportunity opp;
                    opp.target_instrument = instrument_id;
                    opp.type = MispricingType::STATISTICAL_ARBITRAGE;
                    opp.severity = MispricingSeverity::MEDIUM;
                    opp.market_price = price_queue.back().bid_price;
                    opp.theoretical_price = opp.market_price * 1.02; // 2% difference for demo
                    opp.deviation_percentage = 0.02;
                    opp.z_score = 2.5;
                    opp.confidence_level = 0.85;
                    opp.expected_profit = 100.0;
                    opp.max_loss = 50.0;
                    opp.value_at_risk = 30.0;

                    opportunities.push_back(opp);
                }
            }

            return opportunities;
        }

        void StatisticalMispricingDetector::set_detection_callback(MispricingCallback callback)
        {
            detection_callback_ = callback;
        }

        void StatisticalMispricingDetector::set_expiry_callback(MispricingExpiredCallback callback)
        {
            expiry_callback_ = callback;
        }

        void StatisticalMispricingDetector::update_parameters(const DetectionParameters &params)
        {
            params_ = params;
        }

        std::vector<MispricingOpportunity> StatisticalMispricingDetector::get_active_opportunities() const
        {
            std::lock_guard<std::mutex> lock(opportunities_mutex_);
            return active_opportunities_;
        }

        void StatisticalMispricingDetector::clear_opportunities()
        {
            std::lock_guard<std::mutex> lock(opportunities_mutex_);
            active_opportunities_.clear();
        }

        bool StatisticalMispricingDetector::is_significant_deviation(double deviation, double z_score, double confidence)
        {
            return std::abs(deviation) > params_.min_deviation_threshold &&
                   std::abs(z_score) > params_.min_z_score &&
                   confidence > params_.min_confidence_level;
        }

        double StatisticalMispricingDetector::calculate_z_score(const std::queue<double> &history, double current_value)
        {
            if (history.size() < 2)
                return 0.0;

            // Convert queue to vector for calculations
            std::vector<double> values;
            std::queue<double> temp_queue = history;
            while (!temp_queue.empty())
            {
                values.push_back(temp_queue.front());
                temp_queue.pop();
            }

            double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
            double variance = 0.0;
            for (double val : values)
            {
                variance += (val - mean) * (val - mean);
            }
            variance /= values.size();
            double std_dev = std::sqrt(variance);

            return std_dev > 0 ? (current_value - mean) / std_dev : 0.0;
        }

        double StatisticalMispricingDetector::calculate_confidence_level(const std::queue<Quote> &history, double theoretical_price)
        {
            return 0.85; // Simplified confidence calculation for demo
        }

        MispricingSeverity StatisticalMispricingDetector::assess_severity(const PriceDeviation &deviation)
        {
            if (std::abs(deviation.deviation_percentage) > 0.05)
                return MispricingSeverity::CRITICAL;
            if (std::abs(deviation.deviation_percentage) > 0.02)
                return MispricingSeverity::HIGH;
            if (std::abs(deviation.deviation_percentage) > 0.01)
                return MispricingSeverity::MEDIUM;
            return MispricingSeverity::LOW;
        }

        void StatisticalMispricingDetector::cleanup_expired_opportunities()
        {
            std::lock_guard<std::mutex> lock(opportunities_mutex_);
            auto now = std::chrono::high_resolution_clock::now();
            active_opportunities_.erase(
                std::remove_if(active_opportunities_.begin(), active_opportunities_.end(),
                               [now](const MispricingOpportunity &opp)
                               {
                                   return now > opp.expiry_time;
                               }),
                active_opportunities_.end());
        }

        void StatisticalMispricingDetector::update_price_history(const InstrumentId &instrument, const Quote &quote)
        {
            auto &history = price_history_[instrument];
            history.push(quote);

            // Keep only recent history
            if (history.size() > params_.min_observation_window * 2)
            {
                history.pop();
            }
        }

        // TriangularArbitrageDetector implementation
        TriangularArbitrageDetector::TriangularArbitrageDetector(const DetectionParameters &params)
            : params_(params)
        {
            // Add some default currency triangles for demo
            add_currency_triangle("BTC-ETH-USD", {"BTC-USD", "ETH-USD", "BTC-ETH"});
            add_currency_triangle("BTC-USDT-USD", {"BTC-USD", "USDT-USD", "BTC-USDT"});
        }
        
        //methods for adding/removing currency triangles and detecting arbitrage opportunities
        void TriangularArbitrageDetector::update_market_data(const MarketSnapshot &snapshot)
        {
            // Process market data for triangular arbitrage detection
        }

        std::vector<MispricingOpportunity> TriangularArbitrageDetector::detect_opportunities()
        {
            std::vector<MispricingOpportunity> opportunities;

            // Simplified triangular arbitrage detection for demo
            for (const auto &[name, instruments] : currency_triangles_)
            {
                if (instruments.size() >= 3)
                {
                    MispricingOpportunity opp;
                    opp.target_instrument = instruments[0];
                    opp.component_instruments = instruments;
                    opp.type = MispricingType::CROSS_CURRENCY_TRIANGULAR;
                    opp.severity = MispricingSeverity::MEDIUM;
                    opp.market_price = 100.0;
                    opp.theoretical_price = 101.5;
                    opp.deviation_percentage = 0.015;
                    opp.z_score = 2.2;
                    opp.confidence_level = 0.88;
                    opp.expected_profit = 150.0;
                    opp.max_loss = 75.0;

                    opportunities.push_back(opp);
                }
            }

            return opportunities;
        }

        void TriangularArbitrageDetector::set_detection_callback(MispricingCallback callback)
        {
            detection_callback_ = callback;
        }

        void TriangularArbitrageDetector::set_expiry_callback(MispricingExpiredCallback callback)
        {
            expiry_callback_ = callback;
        }

        void TriangularArbitrageDetector::update_parameters(const DetectionParameters &params)
        {
            params_ = params;
        }

        void TriangularArbitrageDetector::add_currency_triangle(const std::string &name, const std::vector<InstrumentId> &instruments)
        {
            currency_triangles_[name] = instruments;
        }

        void TriangularArbitrageDetector::remove_currency_triangle(const std::string &name)
        {
            currency_triangles_.erase(name);
        }

        std::vector<MispricingOpportunity> TriangularArbitrageDetector::detect_triangular_opportunities(const MarketSnapshot &snapshot)
        {
            return detect_opportunities(); // Simplified for demo
        }

        double TriangularArbitrageDetector::calculate_triangular_profit(const Quote &pair1, const Quote &pair2, const Quote &pair3)
        {
            // Simplified triangular profit calculation
            return 0.015; // 1.5% profit for demo
        }

        bool TriangularArbitrageDetector::is_profitable_triangle(double profit_percentage)
        {
            return profit_percentage > params_.min_deviation_threshold;
        }

        // VolatilityArbitrageDetector implementation
        VolatilityArbitrageDetector::VolatilityArbitrageDetector(const DetectionParameters &params)
            : params_(params) {}
        // calculates realized volatility based on historical prices
        void VolatilityArbitrageDetector::update_market_data(const MarketSnapshot &snapshot)
        {
            for (const auto &[instrument_id, quote] : snapshot.quotes)
            {
                auto &history = volatility_history_[instrument_id];
                double mid_price = (quote.bid_price + quote.ask_price) / 2.0;
                history.push(mid_price);

                // Keep only recent history
                if (history.size() > 100)
                {
                    history.pop();
                }
            }
        }
        // calculates realized volatility
        std::vector<MispricingOpportunity> VolatilityArbitrageDetector::detect_opportunities()
        {
            std::vector<MispricingOpportunity> opportunities;

            for (const auto &[instrument_id, price_history] : volatility_history_)
            {
                if (price_history.size() >= 20)
                {
                    double realized_vol = calculate_realized_volatility(price_history);

                    MispricingOpportunity opp;
                    opp.target_instrument = instrument_id;
                    opp.type = MispricingType::VOLATILITY_ARBITRAGE;
                    opp.severity = MispricingSeverity::LOW;
                    opp.market_price = 100.0;
                    opp.theoretical_price = 100.8;
                    opp.deviation_percentage = 0.008;
                    opp.z_score = 1.8;
                    opp.confidence_level = 0.75;
                    opp.expected_profit = 80.0;
                    opp.max_loss = 40.0;

                    opportunities.push_back(opp);
                }
            }

            return opportunities;
        }

        void VolatilityArbitrageDetector::set_detection_callback(MispricingCallback callback)
        {
            detection_callback_ = callback;
        }

        void VolatilityArbitrageDetector::set_expiry_callback(MispricingExpiredCallback callback)
        {
            expiry_callback_ = callback;
        }

        void VolatilityArbitrageDetector::update_parameters(const DetectionParameters &params)
        {
            params_ = params;
        }

        double VolatilityArbitrageDetector::calculate_realized_volatility(const std::queue<Price> &prices)
        {
            if (prices.size() < 2)
                return 0.0;

            std::vector<double> returns;
            std::queue<Price> temp_queue = prices;
            double prev_price = temp_queue.front();
            temp_queue.pop();

            while (!temp_queue.empty())
            {
                double current_price = temp_queue.front();
                temp_queue.pop();
                if (prev_price > 0)
                {
                    returns.push_back(std::log(current_price / prev_price));
                }
                prev_price = current_price;
            }

            if (returns.empty())
                return 0.0;

            double mean_return = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
            double variance = 0.0;
            for (double ret : returns)
            {
                variance += (ret - mean_return) * (ret - mean_return);
            }
            variance /= returns.size();

            return std::sqrt(variance * 252); // Annualized volatility
        }

        double VolatilityArbitrageDetector::calculate_implied_volatility_proxy(const Quote &quote)
        {
            double mid_price = (quote.bid_price + quote.ask_price) / 2.0;
            double spread = quote.ask_price - quote.bid_price;
            return spread / mid_price; // Simple proxy using relative spread
        }

        std::vector<MispricingOpportunity> VolatilityArbitrageDetector::detect_volatility_opportunities(const MarketSnapshot &snapshot)
        {
            return detect_opportunities(); // Simplified for demo
        }

        // CompositeMispricingDetector implementation
        CompositeMispricingDetector::CompositeMispricingDetector(const DetectionParameters &params)
            : params_(params) {}
          // manages a collection of deifferent detector types
        void CompositeMispricingDetector::add_detector(std::unique_ptr<IMispricingDetector> detector)
        {
            detectors_.push_back(std::move(detector));
        }

        void CompositeMispricingDetector::remove_detector(size_t index)
        {
            if (index < detectors_.size())
            {
                detectors_.erase(detectors_.begin() + index);
            }
        }

        void CompositeMispricingDetector::update_market_data(const MarketSnapshot &snapshot)
        {
            for (auto &detector : detectors_)
            {
                detector->update_market_data(snapshot);
            }
        }

        std::vector<MispricingOpportunity> CompositeMispricingDetector::detect_opportunities()
        {
            std::vector<MispricingOpportunity> all_opportunities;

            for (auto &detector : detectors_)
            {
                auto opportunities = detector->detect_opportunities();
                all_opportunities.insert(all_opportunities.end(), opportunities.begin(), opportunities.end());
            }

            consolidate_opportunities(all_opportunities);
            return all_opportunities;
        }

        void CompositeMispricingDetector::set_detection_callback(MispricingCallback callback)
        {
            detection_callback_ = callback;
            for (auto &detector : detectors_)
            {
                detector->set_detection_callback(callback);
            }
        }

        void CompositeMispricingDetector::set_expiry_callback(MispricingExpiredCallback callback)
        {
            expiry_callback_ = callback;
            for (auto &detector : detectors_)
            {
                detector->set_expiry_callback(callback);
            }
        }

        void CompositeMispricingDetector::update_parameters(const DetectionParameters &params)
        {
            params_ = params;
            for (auto &detector : detectors_)
            {
                detector->update_parameters(params);
            }
        }

        void CompositeMispricingDetector::consolidate_opportunities(std::vector<MispricingOpportunity> &opportunities)
        {
            // Remove duplicates and merge similar opportunities
            // Simplified implementation for demo
            std::sort(opportunities.begin(), opportunities.end(),
                      [](const MispricingOpportunity &a, const MispricingOpportunity &b)
                      {
                          return a.expected_profit > b.expected_profit;
                      });
        }

    } // namespace mispricing
} // namespace spe
