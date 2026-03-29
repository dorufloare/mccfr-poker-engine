#include "example_lookups.h"

#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>

#include "action.h"
#include "mccfr.h"

namespace {

constexpr std::array<const char*, action::ACTIONS> kActionNames = {
    "FOLD", "CALL", "BET_SMALL", "BET_BIG"
};

const char* display_action_name(int action_index, uint8_t to_call) {
    if (action_index == static_cast<int>(action::ActionType::CALL) && to_call == 0) {
        return "CHECK";
    }
    return kActionNames[action_index];
}

void print_strategy(const TestConfig& test, mccfr::MCCFR& solver) {
    const mccfr::InfoSetKey key = test.to_key();

    auto& infosets = solver.get_infosets();
    auto it = infosets.find(key);

    std::cout << "------------------------------------------------------------\n";
    std::cout << "Description: " << test.description << "\n";
    std::cout << "State      : " << test << "\n";

    if (it == infosets.end()) {
        std::cout << "Visits     : 0\n";
        std::cout << "AvgStr     : (not visited during training)\n\n";
        return;
    }

    std::cout << "Visits     : " << it->second.visit_count << "\n";

    float strategy[action::ACTIONS] = {};
    it->second.get_average_strategy(strategy);

    std::ostringstream strategy_line;
    strategy_line << std::fixed << std::setprecision(2);
    for (int i = 0; i < action::ACTIONS; ++i) {
        if (i > 0) {
            strategy_line << " | ";
        }
        strategy_line << display_action_name(i, test.to_call) << ": " << (strategy[i] * 100.0f) << "%";
    }

    std::cout << "AvgStr     : " << strategy_line.str() << "\n\n";
}

} // namespace

mccfr::InfoSetKey TestConfig::to_key() const {
    mccfr::InfoSetKey key{};
    key.hole = hole;
    key.board = board;
    key.street = static_cast<uint8_t>(street);
    key.position = static_cast<uint8_t>(position);
    key.street_actions = street_actions;
    key.to_call = to_call;
    return key;
}

std::ostream& operator<<(std::ostream& os, const TestConfig& config) {
    os << "Hole: " << cards::mask_to_string(config.hole)
       << ", Board: " << cards::mask_to_string(config.board)
       << ", ToCall: " << static_cast<int>(config.to_call)
       << ", StackSelf: " << static_cast<int>(config.stack_self)
       << ", StackOpp: " << static_cast<int>(config.stack_opp);
    return os;
}

std::vector<TestConfig> make_default_test_configs() {
    const auto c = [](cards::Rank rank, cards::Suit suit) {
        return std::pair{rank, suit};
    };

    std::vector<TestConfig> tests;
    tests.reserve(14);

    // Preflop baselines
    tests.push_back({
        "AA preflop opener",
        cards::make_hole_cards(c(cards::Rank::ACE, cards::Suit::HEARTS), c(cards::Rank::ACE, cards::Suit::SPADES)),
        cards::CardsMask{0},
        50,
        50,
        state::Street::PREFLOP,
        state::Position::OUT_OF_POSITION,
        0,
        3
    });
    tests.push_back({
        "72o preflop opener",
        cards::make_hole_cards(c(cards::Rank::SEVEN, cards::Suit::HEARTS), c(cards::Rank::TWO, cards::Suit::SPADES)),
        cards::CardsMask{0},
        50,
        50,
        state::Street::PREFLOP,
        state::Position::OUT_OF_POSITION,
        0,
        3
    });
    tests.push_back({
        "72o preflop facing raise",
        cards::make_hole_cards(c(cards::Rank::SEVEN, cards::Suit::HEARTS), c(cards::Rank::TWO, cards::Suit::SPADES)),
        cards::CardsMask{0},
        50,
        50,
        state::Street::PREFLOP,
        state::Position::OUT_OF_POSITION,
        1,
        7
    });

    // Flop: one value hand + two draws
    tests.push_back({
        "Top pair top kicker (KQs on K-8-3)",
        cards::make_hole_cards(c(cards::Rank::KING, cards::Suit::HEARTS), c(cards::Rank::QUEEN, cards::Suit::HEARTS)),
        cards::make_board_cards({
            c(cards::Rank::KING, cards::Suit::DIAMONDS),
            c(cards::Rank::EIGHT, cards::Suit::CLUBS),
            c(cards::Rank::THREE, cards::Suit::SPADES)
        }),
        50,
        50,
        state::Street::FLOP,
        state::Position::OUT_OF_POSITION,
        0,
        0
    });
    tests.push_back({
        "Nut flush draw (AhKh on Qh-7h-2c)",
        cards::make_hole_cards(c(cards::Rank::ACE, cards::Suit::HEARTS), c(cards::Rank::KING, cards::Suit::HEARTS)),
        cards::make_board_cards({
            c(cards::Rank::QUEEN, cards::Suit::HEARTS),
            c(cards::Rank::SEVEN, cards::Suit::HEARTS),
            c(cards::Rank::TWO, cards::Suit::CLUBS)
        }),
        50,
        50,
        state::Street::FLOP,
        state::Position::IN_POSITION,
        1,
        7
    });
    tests.push_back({
        "Open-ended straight draw (98o on 7-6-2)",
        cards::make_hole_cards(c(cards::Rank::NINE, cards::Suit::CLUBS), c(cards::Rank::EIGHT, cards::Suit::DIAMONDS)),
        cards::make_board_cards({
            c(cards::Rank::SEVEN, cards::Suit::SPADES),
            c(cards::Rank::SIX, cards::Suit::HEARTS),
            c(cards::Rank::TWO, cards::Suit::CLUBS)
        }),
        50,
        50,
        state::Street::FLOP,
        state::Position::IN_POSITION,
        1,
        7
    });

    // Strong showdown textures on river
    tests.push_back({
        "Royal flush (AhKh on Qh-Jh-Th-2c-3d)",
        cards::make_hole_cards(c(cards::Rank::ACE, cards::Suit::HEARTS), c(cards::Rank::KING, cards::Suit::HEARTS)),
        cards::make_board_cards({
            c(cards::Rank::QUEEN, cards::Suit::HEARTS),
            c(cards::Rank::JACK, cards::Suit::HEARTS),
            c(cards::Rank::TEN, cards::Suit::HEARTS),
            c(cards::Rank::TWO, cards::Suit::CLUBS),
            c(cards::Rank::THREE, cards::Suit::DIAMONDS)
        }),
        50,
        50,
        state::Street::RIVER,
        state::Position::OUT_OF_POSITION,
        0,
        0
    });
    tests.push_back({
        "Quads aces (AcAd on As-Ah-7c-2d-9s)",
        cards::make_hole_cards(c(cards::Rank::ACE, cards::Suit::CLUBS), c(cards::Rank::ACE, cards::Suit::DIAMONDS)),
        cards::make_board_cards({
            c(cards::Rank::ACE, cards::Suit::SPADES),
            c(cards::Rank::ACE, cards::Suit::HEARTS),
            c(cards::Rank::SEVEN, cards::Suit::CLUBS),
            c(cards::Rank::TWO, cards::Suit::DIAMONDS),
            c(cards::Rank::NINE, cards::Suit::SPADES)
        }),
        50,
        50,
        state::Street::RIVER,
        state::Position::OUT_OF_POSITION,
        0,
        0
    });
    tests.push_back({
        "Full house (KcKs on Kh-7d-7s-2c-9h)",
        cards::make_hole_cards(c(cards::Rank::KING, cards::Suit::CLUBS), c(cards::Rank::KING, cards::Suit::SPADES)),
        cards::make_board_cards({
            c(cards::Rank::KING, cards::Suit::HEARTS),
            c(cards::Rank::SEVEN, cards::Suit::DIAMONDS),
            c(cards::Rank::SEVEN, cards::Suit::SPADES),
            c(cards::Rank::TWO, cards::Suit::CLUBS),
            c(cards::Rank::NINE, cards::Suit::HEARTS)
        }),
        50,
        50,
        state::Street::RIVER,
        state::Position::OUT_OF_POSITION,
        0,
        0
    });

    // One weak-hand river pressure example
    tests.push_back({
        "Air facing river bet (23o on K-Q-J-9-7)",
        cards::make_hole_cards(c(cards::Rank::TWO, cards::Suit::CLUBS), c(cards::Rank::THREE, cards::Suit::DIAMONDS)),
        cards::make_board_cards({
            c(cards::Rank::KING, cards::Suit::HEARTS),
            c(cards::Rank::QUEEN, cards::Suit::CLUBS),
            c(cards::Rank::JACK, cards::Suit::SPADES),
            c(cards::Rank::NINE, cards::Suit::DIAMONDS),
            c(cards::Rank::SEVEN, cards::Suit::HEARTS)
        }),
        50,
        50,
        state::Street::RIVER,
        state::Position::IN_POSITION,
        1,
        7
    });

    return tests;
}

void print_example_lookups(mccfr::MCCFR& solver) {
    print_example_lookups(solver, make_default_test_configs());
}

void print_example_lookups(mccfr::MCCFR& solver, const std::vector<TestConfig>& tests) {
    std::cout << "\n========== EXAMPLE LOOKUPS ==========\n\n";
    for (const TestConfig& test : tests) {
        print_strategy(test, solver);
    }
}
