#include <iostream>
#include <cstdint>
#include "decision_state.h"
#include "cards.h"
#include "mccfr.h"
#include "action.h"
#include "info_set.h"
#include "utils.h" // for random_utils::fast_rand
#include "evaluation.h" // for eval::evaluate_showdown
#include "mccfr.h"

void test_showdowns() {
    using namespace cards;
    using namespace eval;

    // Example 1: simple pair vs high card
    CardsMask my_hole   = make_hole_cards(Rank::ACE, Suit::HEARTS, Rank::KING, Suit::CLUBS);
    CardsMask opp_hole  = make_hole_cards(Rank::QUEEN, Suit::SPADES, Rank::TEN, Suit::DIAMONDS);
    CardsMask board     = make_board_cards({{Rank::ACE, Suit::DIAMONDS}, {Rank::TWO, Suit::CLUBS}, {Rank::FIVE, Suit::HEARTS}});

    float result1 = evaluate_showdown(my_hole, opp_hole, board);
    std::cout << "Example 1: My pair vs opponent high card = " << result1 << "\n";

    // Example 2: both have pairs, mine higher
    my_hole   = make_hole_cards(Rank::KING, Suit::HEARTS, Rank::TEN, Suit::CLUBS);
    opp_hole  = make_hole_cards(Rank::QUEEN, Suit::SPADES, Rank::TEN, Suit::DIAMONDS);
    board     = make_board_cards({{Rank::TEN, Suit::HEARTS}, {Rank::TWO, Suit::CLUBS}, {Rank::FIVE, Suit::SPADES}});

    float result2 = evaluate_showdown(my_hole, opp_hole, board);
    std::cout << "Example 2: My higher pair vs opponent = " << result2 << "\n";

    // Example 3: tie (both high card only)
    my_hole   = make_hole_cards(Rank::ACE, Suit::HEARTS, Rank::KING, Suit::CLUBS);
    opp_hole  = make_hole_cards(Rank::ACE, Suit::DIAMONDS, Rank::KING, Suit::SPADES);
    board     = make_board_cards({{Rank::TWO, Suit::CLUBS}, {Rank::FIVE, Suit::SPADES}, {Rank::NINE, Suit::HEARTS}});

    float result3 = evaluate_showdown(my_hole, opp_hole, board);
    std::cout << "Example 3: Tie high cards = " << result3 << "\n";

    // Example 4: opponent wins with higher pair
    my_hole   = make_hole_cards(Rank::TEN, Suit::HEARTS, Rank::NINE, Suit::CLUBS);
    opp_hole  = make_hole_cards(Rank::JACK, Suit::DIAMONDS, Rank::JACK, Suit::SPADES);
    board     = make_board_cards({{Rank::TWO, Suit::CLUBS}, {Rank::FIVE, Suit::SPADES}, {Rank::NINE, Suit::HEARTS}});

    float result4 = evaluate_showdown(my_hole, opp_hole, board);
    std::cout << "Example 4: Opponent higher pair = " << result4 << "\n";
}

int main() {
    using namespace state;
    using namespace cards;
    using namespace action;
    using namespace eval;
    using namespace mccfr;

    uint32_t rng = 12345; // fixed seed for reproducibility

    // 1️⃣ Create a simple test hand
    DecisionState s;
    s.hand_id      = 1;
    s.street       = Street::PREFLOP;
    s.position     = Position::OUT_OF_POSITION;
    s.pot_size     = 2;
    s.stack_self   = 10;
    s.stack_opp    = 10;
    s.to_call      = 1;

    // Draw hole cards (reduced deck for testing)
    s.hole_cards = draw_random_card(rng, 0) | draw_random_card(rng, 0);
    s.board_cards = 0;

    std::cout << "Initial hand:\n";
    std::cout << "Hole cards: " << cards::mask_to_string(s.hole_cards) << "\n";
    // All actions allowed initially
    s.action_mask = static_cast<ActionsMask>(action::ALL); // FOLD/CALL/RAISE

    // 2️⃣ Create MCCFR object
    MCCFR mccfr;

    // 3️⃣ Run a few iterations to let MCCFR update regrets
    const int iterations = 5000;
    for (int i = 0; i < iterations; ++i) {
        float util = mccfr.traverse(s, 1.f, 1.f, rng);
        if (i % 1000 == 0) std::cout << "Iteration " << i << ", utility: " << util << "\n";
    }

    // 4️⃣ Print learned strategies
    std::cout << "\nLearned strategies at infosets:\n";
    for (auto& [key, I] : mccfr.get_infosets()) {
        float strat[3];
        I.get_strategy(strat, 1.f);
        std::cout << "Infoset key: " << infoset_key_to_string(key) << "\n";
        std::cout << "  FOLD: "  << strat[0] 
                  << ", CALL: " << strat[1] 
                  << ", RAISE: "<< strat[2] << "\n\n";
    }

    // 5️⃣ Test showdown evaluation
    std::cout << "Testing showdown evaluations:\n";
    test_showdowns();

    return 0;
}
