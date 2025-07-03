#include "arbitrage_engine.hpp"
#include <algorithm>
#include <random>
#include <sstream>

namespace spe {
namespace arbitrage {

// ArbitrageEngine implementation
ArbitrageEngine::ArbitrageEngine(const ArbitrageParameters& params)
    : params_(params) {}

ArbitrageEngine::~ArbitrageEngine() = default;

void ArbitrageEngine::update_market_data(const MarketSnapshot& snapshot) {
    latest_snapshot_ = snapshot;
    cleanup_expired_opportunities();
}

void ArbitrageEngine::process_mispricing(const MispricingOpportunity& mispricing) {
    pending_mispricings_.push(mispricing);
    
    // Process pending mispricings
    while (!pending_mispricings_.empty()) {
        auto mispricing_opp = pending_mispricings_.front();
        pending_mispricings_.pop();
        
        auto arbitrage_opp = create_arbitrage_from_mispricing(mispricing_opp);
        
        if (validate_opportunity(arbitrage_opp)) {
            active_opportunities_.push_back(arbitrage_opp);
            
            if (opportunity_callback_) {
                opportunity_callback_(arbitrage_opp);
            }
        }
    }
}

std::vector<ArbitrageOpportunity> ArbitrageEngine::identify_opportunities() {
    std::vector<ArbitrageOpportunity> opportunities;
    
    // Simplified opportunity identification for demo
    if (latest_snapshot_.quotes.size() >= 2) {
        ArbitrageOpportunity opp;
        opp.opportunity_id = generate_opportunity_id();
        opp.type = ArbitrageType::CROSS_EXCHANGE_SYNTHETIC_REPLICATION;
        opp.status = ArbitrageStatus::IDENTIFIED;
        
        // Create synthetic arbitrage legs
        auto it = latest_snapshot_.quotes.begin();
        for (int i = 0; i < 2 && it != latest_snapshot_.quotes.end(); ++i, ++it) {
            ArbitrageLeg leg;
            leg.instrument_id = it->first;
            leg.side = (i == 0) ? market_data::Side::BID : market_data::Side::ASK;
            leg.size = 100.0;
            leg.entry_price = (it->second.bid_price + it->second.ask_price) / 2.0;
            leg.weight = (i == 0) ? 1.0 : -1.0;
            leg.entry_time = std::chrono::high_resolution_clock::now();
            
            opp.legs.push_back(leg);
        }
        
        // Calculate financial metrics
        opp.expected_profit = 250.0;
        opp.max_loss = 125.0;
        opp.profit_probability = 0.75;
        opp.value_at_risk = calculate_value_at_risk(opp);
        opp.expected_shortfall = calculate_expected_shortfall(opp);
        opp.correlation_risk = calculate_correlation_risk(opp.legs);
        opp.market_impact = calculate_market_impact(opp);
        
        opp.identification_time = std::chrono::high_resolution_clock::now();
        opp.expiry_time = opp.identification_time + std::chrono::minutes(30);
        
        opportunities.push_back(opp);
    }
    
    return opportunities;
}

bool ArbitrageEngine::validate_opportunity(ArbitrageOpportunity& opportunity) {
    // Comprehensive validation
    bool is_valid = true;
    
    is_valid &= validate_liquidity(opportunity);
    is_valid &= validate_risk_limits(opportunity);
    is_valid &= validate_timing(opportunity);
    is_valid &= validate_execution_feasibility(opportunity);
    
    if (is_valid) {
        opportunity.status = ArbitrageStatus::VALIDATED;
        opportunity.validation_time = std::chrono::high_resolution_clock::now();
    }
    
    return is_valid;
}

void ArbitrageEngine::set_opportunity_callback(ArbitrageCallback callback) {
    opportunity_callback_ = callback;
}

void ArbitrageEngine::set_update_callback(ArbitrageUpdateCallback callback) {
    update_callback_ = callback;
}

void ArbitrageEngine::update_parameters(const ArbitrageParameters& params) {
    params_ = params;
}

std::vector<ArbitrageOpportunity> ArbitrageEngine::get_active_opportunities() const {
    return active_opportunities_;
}

void ArbitrageEngine::clear_opportunities() {
    active_opportunities_.clear();
}

void ArbitrageEngine::update_opportunity_status(const std::string& opportunity_id, ArbitrageStatus status) {
    auto it = std::find_if(active_opportunities_.begin(), active_opportunities_.end(),
        [&opportunity_id](const ArbitrageOpportunity& opp) {
            return opp.opportunity_id == opportunity_id;
        });
    
    if (it != active_opportunities_.end()) {
        it->status = status;
        update_opportunity_status(*it);
        
        if (update_callback_) {
            update_callback_(*it);
        }
    }
}

ArbitrageOpportunity ArbitrageEngine::get_opportunity_by_id(const std::string& opportunity_id) const {
    auto it = std::find_if(active_opportunities_.begin(), active_opportunities_.end(),
        [&opportunity_id](const ArbitrageOpportunity& opp) {
            return opp.opportunity_id == opportunity_id;
        });
    
    if (it != active_opportunities_.end()) {
        return *it;
    }
    
    return ArbitrageOpportunity{}; // Return empty opportunity if not found
}

// Private methods
ArbitrageOpportunity ArbitrageEngine::create_arbitrage_from_mispricing(const MispricingOpportunity& mispricing) {
    ArbitrageOpportunity arbitrage_opp;
    
    arbitrage_opp.opportunity_id = generate_opportunity_id();
    arbitrage_opp.type = ArbitrageType::STATISTICAL_ARBITRAGE;
    arbitrage_opp.status = ArbitrageStatus::IDENTIFIED;
    arbitrage_opp.mispricing_source = mispricing;
    
    // Convert mispricing to arbitrage legs
    arbitrage_opp.legs = optimize_legs(mispricing);
    
    // Copy financial metrics from mispricing
    arbitrage_opp.expected_profit = mispricing.expected_profit;
    arbitrage_opp.max_loss = mispricing.max_loss;
    arbitrage_opp.value_at_risk = mispricing.value_at_risk;
    arbitrage_opp.expected_shortfall = mispricing.expected_shortfall;
    arbitrage_opp.sharpe_ratio = mispricing.sharpe_ratio;
    
    arbitrage_opp.identification_time = std::chrono::high_resolution_clock::now();
    arbitrage_opp.expiry_time = mispricing.expiry_time;
    
    return arbitrage_opp;
}

std::vector<ArbitrageLeg> ArbitrageEngine::optimize_legs(const MispricingOpportunity& mispricing) {
    std::vector<ArbitrageLeg> legs;
    
    // Create primary leg for target instrument
    ArbitrageLeg primary_leg;
    primary_leg.instrument_id = mispricing.target_instrument;
    primary_leg.side = (mispricing.market_price < mispricing.theoretical_price) ? 
                       market_data::Side::BID : market_data::Side::ASK;
    primary_leg.size = 100.0; // Simplified size
    primary_leg.entry_price = mispricing.market_price;
    primary_leg.weight = 1.0;
    primary_leg.entry_time = std::chrono::high_resolution_clock::now();
    
    legs.push_back(primary_leg);
    
    // Create hedging legs for component instruments
    for (size_t i = 0; i < mispricing.component_instruments.size() && i < mispricing.weights.size(); ++i) {
        ArbitrageLeg hedge_leg;
        hedge_leg.instrument_id = mispricing.component_instruments[i];
        hedge_leg.side = (mispricing.weights[i] > 0) ? market_data::Side::ASK : market_data::Side::BID;
        hedge_leg.size = std::abs(mispricing.weights[i]) * 100.0;
        hedge_leg.entry_price = 100.0; // Simplified price
        hedge_leg.weight = -mispricing.weights[i];
        hedge_leg.entry_time = std::chrono::high_resolution_clock::now();
        
        legs.push_back(hedge_leg);
    }
    
    return legs;
}

double ArbitrageEngine::calculate_value_at_risk(const ArbitrageOpportunity& opportunity) {
    // Simplified VaR calculation for demo
    double total_exposure = 0.0;
    for (const auto& leg : opportunity.legs) {
        total_exposure += std::abs(leg.size * leg.entry_price * leg.weight);
    }
    
    return total_exposure * 0.05; // 5% VaR assumption
}

double ArbitrageEngine::calculate_expected_shortfall(const ArbitrageOpportunity& opportunity) {
    // ES is typically 1.3x VaR for normal distribution
    return calculate_value_at_risk(opportunity) * 1.3;
}

double ArbitrageEngine::calculate_correlation_risk(const std::vector<ArbitrageLeg>& legs) {
    // Simplified correlation risk calculation
    if (legs.size() < 2) return 0.0;
    
    double max_correlation = 0.0;
    for (size_t i = 0; i < legs.size(); ++i) {
        for (size_t j = i + 1; j < legs.size(); ++j) {
            // Assume moderate correlation between instruments
            double correlation = 0.6;
            max_correlation = std::max(max_correlation, correlation);
        }
    }
    
    return max_correlation;
}

double ArbitrageEngine::calculate_market_impact(const ArbitrageOpportunity& opportunity) {
    // Simplified market impact calculation
    double total_volume = 0.0;
    for (const auto& leg : opportunity.legs) {
        total_volume += leg.size;
    }
    
    // Assume linear market impact: 0.1 basis points per 1000 units
    return (total_volume / 1000.0) * 0.001;
}

bool ArbitrageEngine::validate_liquidity(const ArbitrageOpportunity& opportunity) {
    for (const auto& leg : opportunity.legs) {
        auto quote_it = latest_snapshot_.quotes.find(leg.instrument_id);
        if (quote_it != latest_snapshot_.quotes.end()) {
            double available_liquidity = (leg.side == market_data::Side::BID) ?
                                       quote_it->second.ask_size : quote_it->second.bid_size;
            
            if (available_liquidity < leg.size) {
                return false; // Insufficient liquidity
            }
        }
    }
    
    return true;
}

bool ArbitrageEngine::validate_risk_limits(const ArbitrageOpportunity& opportunity) {
    // Check profit threshold
    if (opportunity.expected_profit < params_.min_profit_threshold * opportunity.total_cost) {
        return false;
    }
    
    // Check VaR limit
    if (opportunity.value_at_risk > params_.max_risk_per_trade * opportunity.total_cost) {
        return false;
    }
    
    // Check correlation risk
    if (opportunity.correlation_risk > params_.max_correlation_risk) {
        return false;
    }
    
    // Check market impact
    if (opportunity.market_impact > params_.max_market_impact) {
        return false;
    }
    
    return true;
}

bool ArbitrageEngine::validate_timing(const ArbitrageOpportunity& opportunity) {
    auto now = std::chrono::high_resolution_clock::now();
    
    // Check if opportunity hasn't expired
    if (now >= opportunity.expiry_time) {
        return false;
    }
    
    // Check if we have enough time to execute
    auto time_remaining = std::chrono::duration_cast<std::chrono::minutes>(
        opportunity.expiry_time - now);
    
    if (time_remaining < std::chrono::minutes(5)) { // Need at least 5 minutes
        return false;
    }
    
    return true;
}

bool ArbitrageEngine::validate_execution_feasibility(const ArbitrageOpportunity& opportunity) {
    // Check if total position size is within limits
    double total_position_value = 0.0;
    for (const auto& leg : opportunity.legs) {
        total_position_value += leg.size * leg.entry_price;
    }
    
    if (total_position_value > params_.max_position_size) {
        return false;
    }
    
    // Check estimated slippage
    if (opportunity.slippage_estimate > params_.max_slippage) {
        return false;
    }
    
    return true;
}

void ArbitrageEngine::cleanup_expired_opportunities() {
    auto now = std::chrono::high_resolution_clock::now();
    
    active_opportunities_.erase(
        std::remove_if(active_opportunities_.begin(), active_opportunities_.end(),
            [now](const ArbitrageOpportunity& opp) {
                return now >= opp.expiry_time;
            }),
        active_opportunities_.end());
}

void ArbitrageEngine::update_opportunity_status(ArbitrageOpportunity& opportunity) {
    auto now = std::chrono::high_resolution_clock::now();
    
    // Update status based on timing and other factors
    if (now >= opportunity.expiry_time) {
        opportunity.status = ArbitrageStatus::EXPIRED;
    }
}

std::string ArbitrageEngine::generate_opportunity_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1000, 9999);
    
    auto now = std::chrono::high_resolution_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    std::stringstream ss;
    ss << "ARB_" << timestamp << "_" << dis(gen);
    return ss.str();
}

// TriangularArbitrageEngine implementation
TriangularArbitrageEngine::TriangularArbitrageEngine(const ArbitrageParameters& params)
    : params_(params) {
    // Initialize some default currency triangles
    currency_triangles_["BTC-ETH-USD"] = {"BTC-USD", "ETH-USD", "BTC-ETH"};
    currency_triangles_["BTC-USDT-USD"] = {"BTC-USD", "USDT-USD", "BTC-USDT"};
}

void TriangularArbitrageEngine::update_market_data(const MarketSnapshot& snapshot) {
    // Store latest snapshot and detect opportunities
    auto opportunities = identify_triangular_opportunities(snapshot);
    
    for (auto& opp : opportunities) {
        if (validate_opportunity(opp)) {
            active_opportunities_.push_back(opp);
            
            if (opportunity_callback_) {
                opportunity_callback_(opp);
            }
        }
    }
}

void TriangularArbitrageEngine::process_mispricing(const MispricingOpportunity& mispricing) {
    // Convert mispricing to triangular arbitrage if applicable
    if (mispricing.type == mispricing::MispricingType::CROSS_CURRENCY_TRIANGULAR) {
        ArbitrageOpportunity triangular_opp;
        triangular_opp.opportunity_id = "TRIANG_" + std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count());
        triangular_opp.type = ArbitrageType::TRIANGULAR_ARBITRAGE;
        triangular_opp.status = ArbitrageStatus::IDENTIFIED;
        triangular_opp.expected_profit = mispricing.expected_profit;
        triangular_opp.identification_time = std::chrono::high_resolution_clock::now();
        
        active_opportunities_.push_back(triangular_opp);
    }
}

std::vector<ArbitrageOpportunity> TriangularArbitrageEngine::identify_opportunities() {
    return identify_triangular_opportunities(MarketSnapshot{});
}

bool TriangularArbitrageEngine::validate_opportunity(ArbitrageOpportunity& opportunity) {
    // Basic validation for triangular arbitrage
    opportunity.status = ArbitrageStatus::VALIDATED;
    opportunity.validation_time = std::chrono::high_resolution_clock::now();
    return true;
}

void TriangularArbitrageEngine::set_opportunity_callback(ArbitrageCallback callback) {
    opportunity_callback_ = callback;
}

void TriangularArbitrageEngine::set_update_callback(ArbitrageUpdateCallback callback) {
    update_callback_ = callback;
}

void TriangularArbitrageEngine::update_parameters(const ArbitrageParameters& params) {
    params_ = params;
}

std::vector<ArbitrageOpportunity> TriangularArbitrageEngine::get_active_opportunities() const {
    return active_opportunities_;
}

void TriangularArbitrageEngine::clear_opportunities() {
    active_opportunities_.clear();
}

std::vector<ArbitrageOpportunity> TriangularArbitrageEngine::identify_triangular_opportunities(const MarketSnapshot& snapshot) {
    std::vector<ArbitrageOpportunity> opportunities;
    
    for (const auto& [name, triangle] : currency_triangles_) {
        if (triangle.size() >= 3) {
            ArbitrageOpportunity opp = create_triangular_opportunity(triangle, snapshot);
            
            double profit = calculate_triangular_profit(std::vector<market_data::Quote>{}); // Simplified
            if (profit > params_.min_profit_threshold) {
                opp.expected_profit = profit * 1000; // Scale for demo
                opportunities.push_back(opp);
            }
        }
    }
    
    return opportunities;
}

ArbitrageOpportunity TriangularArbitrageEngine::create_triangular_opportunity(
    const std::vector<InstrumentId>& triangle, const MarketSnapshot& snapshot) {
    
    ArbitrageOpportunity opp;
    opp.opportunity_id = "TRIANG_" + std::to_string(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count());
    opp.type = ArbitrageType::TRIANGULAR_ARBITRAGE;
    opp.status = ArbitrageStatus::IDENTIFIED;
    opp.identification_time = std::chrono::high_resolution_clock::now();
    opp.expiry_time = opp.identification_time + std::chrono::minutes(15);
    
    // Create legs for the triangle
    for (size_t i = 0; i < triangle.size(); ++i) {
        ArbitrageLeg leg;
        leg.instrument_id = triangle[i];
        leg.side = (i % 2 == 0) ? market_data::Side::BID : market_data::Side::ASK;
        leg.size = 100.0;
        leg.entry_price = 100.0 + i; // Simplified pricing
        leg.weight = (i % 2 == 0) ? 1.0 : -1.0;
        leg.entry_time = std::chrono::high_resolution_clock::now();
        
        opp.legs.push_back(leg);
    }
    
    return opp;
}

double TriangularArbitrageEngine::calculate_triangular_profit(const std::vector<market_data::Quote>& quotes) {
    // Simplified triangular arbitrage profit calculation
    return 0.015; // 1.5% profit for demo
}

} // namespace arbitrage
} // namespace spe
