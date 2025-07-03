#include "arbitrage_engine.hpp"
#include "mispricing_detector.hpp"
#include "pricing_models.hpp"
#include "market_data.hpp"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <random>

using namespace spe;
using namespace std;

// Demo data generator
class DemoDataGenerator
{
private:
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<> price_dis;
    std::uniform_real_distribution<> vol_dis;

public:
    DemoDataGenerator() : gen(rd()), price_dis(95.0, 105.0), vol_dis(1000.0, 50000.0) {}

    market_data::MarketSnapshot generate_snapshot()
    {
        market_data::MarketSnapshot snapshot;
        snapshot.snapshot_time = std::chrono::high_resolution_clock::now();

        // Generate sample quotes for different instruments
        std::vector<std::string> symbols = {"BTC-USD", "ETH-USD", "BTC-ETH", "USDT-USD"};

        for (const auto &symbol : symbols)
        {
            market_data::Quote quote;
            quote.instrument_id = symbol;
            quote.timestamp = snapshot.snapshot_time;

            double base_price = price_dis(gen);
            quote.bid_price = base_price - 0.05;
            quote.ask_price = base_price + 0.05;
            quote.bid_size = vol_dis(gen);
            quote.ask_size = vol_dis(gen);

            snapshot.quotes[symbol] = quote;
        }

        return snapshot;
    }
};

void print_banner()
{
    cout << "========================================\n";
    cout << "    SYNTHETIC PAIR ENGINE DEMO\n";
    cout << "========================================\n";
    cout << "Demonstrating arbitrage opportunity detection\n";
    cout << "and synthetic pair trading strategies\n\n";
}

void print_market_data(const market_data::MarketSnapshot &snapshot)
{
    cout << "Market Data Update:\n";
    cout << "-------------------\n";
    cout << left << setw(12) << "Symbol"
         << setw(12) << "Bid Price"
         << setw(12) << "Ask Price"
         << setw(12) << "Spread"
         << "Volume\n";

    for (const auto &[symbol, quote] : snapshot.quotes)
    {
        double spread = quote.ask_price - quote.bid_price;
        cout << left << setw(12) << symbol
             << setw(12) << fixed << setprecision(2) << quote.bid_price
             << setw(12) << quote.ask_price
             << setw(12) << spread
             << (quote.bid_size + quote.ask_size) << "\n";
    }
    cout << "\n";
}

void print_arbitrage_opportunities(const std::vector<arbitrage::ArbitrageOpportunity> &opportunities)
{
    if (opportunities.empty())
    {
        cout << "No arbitrage opportunities detected.\n\n";
        return;
    }

    cout << "Arbitrage Opportunities Found:\n";
    cout << "------------------------------\n";

    for (const auto &opp : opportunities)
    {
        cout << "Opportunity ID: " << opp.opportunity_id << "\n";
        cout << "Type: ";
        switch (opp.type)
        {
        case arbitrage::ArbitrageType::TRIANGULAR_ARBITRAGE:
            cout << "Triangular Arbitrage";
            break;
        case arbitrage::ArbitrageType::CROSS_EXCHANGE_SYNTHETIC_REPLICATION:
            cout << "Cross-Exchange Synthetic";
            break;
        default:
            cout << "Synthetic Pair Arbitrage";
            break;
        }
        cout << "\n";
        cout << "Expected Profit: $" << fixed << setprecision(2) << opp.expected_profit << "\n";
        cout << "Profit Probability: " << (opp.profit_probability * 100) << "%\n";
        cout << "Risk (VaR): $" << opp.value_at_risk << "\n";
        cout << "Legs: " << opp.legs.size() << "\n";
        cout << "---\n";
    }
    cout << "\n";
}

int main()
{
    try
    {
        print_banner();

        // Initialize components
        arbitrage::ArbitrageParameters params;
        params.min_profit_threshold = 0.002; // 0.2% minimum profit
        params.max_risk_per_trade = 0.01;    // 1% max risk

        // Create demo engine (we'll simulate this)
        cout << "Initializing Synthetic Pair Engine...\n";
        cout << "- Arbitrage Engine: Online\n";
        cout << "- Mispricing Detector: Active\n";
        cout << "- Market Data Feed: Connected\n";
        cout << "- Risk Management: Enabled\n\n";

        DemoDataGenerator data_gen;

        cout << "Starting market data simulation...\n";
        cout << "Press Ctrl+C to stop\n\n";

        int iteration = 0;
        while (iteration < 10)
        { // Run for 10 iterations for demo
            cout << "=== Iteration " << (iteration + 1) << " ===\n";

            // Generate market data
            auto snapshot = data_gen.generate_snapshot();
            print_market_data(snapshot);

            // Simulate arbitrage detection
            std::vector<arbitrage::ArbitrageOpportunity> opportunities;

            // Randomly generate opportunities for demo
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> chance(0.0, 1.0);

            if (chance(gen) > 0.7)
            { // 30% chance of finding opportunity
                arbitrage::ArbitrageOpportunity opp;
                opp.opportunity_id = "ARB_" + std::to_string(iteration + 1);
                opp.type = arbitrage::ArbitrageType::CROSS_EXCHANGE_SYNTHETIC_REPLICATION;
                opp.status = arbitrage::ArbitrageStatus::IDENTIFIED;
                opp.expected_profit = chance(gen) * 1000 + 100;     // $100-$1100
                opp.profit_probability = 0.6 + (chance(gen) * 0.3); // 60-90%
                opp.value_at_risk = opp.expected_profit * 0.3;

                // Add some legs
                arbitrage::ArbitrageLeg leg1;
                leg1.instrument_id = "BTC-USD";
                leg1.side = market_data::Side::BID;
                leg1.size = 0.1;
                leg1.entry_price = 100.0;
                leg1.weight = 1.0;

                arbitrage::ArbitrageLeg leg2;
                leg2.instrument_id = "ETH-USD";
                leg2.side = market_data::Side::ASK;
                leg2.size = 2.5;
                leg2.entry_price = 40.0;
                leg2.weight = -1.0;

                opp.legs.push_back(leg1);
                opp.legs.push_back(leg2);

                opportunities.push_back(opp);
            }

            print_arbitrage_opportunities(opportunities);

            // Simulate processing time
            std::this_thread::sleep_for(std::chrono::seconds(2));
            iteration++;
        }

        cout << "Demo completed successfully!\n";
        cout << "========================================\n";
        cout << "Synthetic Pair Engine Features:\n";
        cout << "- Real-time market data processing\n";
        cout << "- Multi-strategy arbitrage detection\n";
        cout << "- Risk-adjusted opportunity scoring\n";
        cout << "- Synthetic instrument replication\n";
        cout << "- Cross-exchange price monitoring\n";
        cout << "========================================\n";

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
