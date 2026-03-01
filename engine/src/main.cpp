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

    uint32_t rng = 43124; // fixed seed for reproducibility

    // 1️⃣ Create MCCFR object (shared across all iterations)
    MCCFR mccfr;

    // 2️⃣ Run MCCFR iterations — each iteration samples new hole cards for both players
    const int iterations = 10000;
    for (int i = 0; i < iterations; ++i) {
        DecisionState s;
        s.hand_id      = 1;
        s.street       = Street::PREFLOP;
        s.position     = Position::OUT_OF_POSITION;
        s.pot_size     = 2;     
        s.stack_self   = 10;
        s.stack_opp    = 10;
        s.to_call      = 0;
        s.street_actions = 0;

        // Draw hole cards for BOTH players
        s.hole_cards     = draw_random_cards(rng, 0, 2, 7);
        s.opp_hole_cards = draw_random_cards(rng, s.hole_cards, 2, 7);
        s.board_cards    = 0;

        // Preflop with no bet to call: check or raise (no fold)
        s.action_mask = action::CALL_MASK | action::RAISE_MASK;

        mccfr.traverse(s, 1.f, 1.f, rng);

        if ((i + 1) % 2000 == 0) {
            std::cout << "Completed " << (i + 1) << " / " << iterations << " iterations\n";
        }
    }

    // 3️⃣ Print learned AVERAGE strategies (converged Nash approx)
    std::cout << "\nLearned average strategies at infosets:\n";
    for (auto& [key, I] : mccfr.get_infosets()) {
        float strat[3];
        I.get_average_strategy(strat);
        std::cout << "Infoset key: " << infoset_key_to_string(key) << "\n";
        std::cout << "  FOLD: "  << strat[0] 
                  << ", CALL: " << strat[1] 
                  << ", RAISE: "<< strat[2] << "\n\n";
    }

    

    return 0;
}
