#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include "decision_state.h"
#include "cards.h"
#include "mccfr.h"
#include "action.h"
#include "utils.h"
#include "optimal_ehs_iterations.h"
#include "example_lookups.h"
#include "training_config.h"

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    uint32_t rng = random_utils::init_rng(static_cast<uint32_t>(rand()));

    //experiments::run_ehs_iterations_experiment();

    mccfr::MCCFR solver;

    std::cout << "Training MCCFR (" << training_config::kIterations << " iterations)...\n";
    for (int i = 0; i < training_config::kIterations; ++i) {
        state::DecisionState s{};
        s.hand_id        = training_config::kInitialHandId;
        s.street         = training_config::kInitialStreet;
        s.position       = training_config::kInitialPosition;
        s.pot_size       = training_config::kInitialPotSize;
        s.stack_self     = training_config::kInitialStackSelf;
        s.stack_opp      = training_config::kInitialStackOpp;
        s.to_call        = training_config::kInitialToCall;
        s.street_actions = training_config::kInitialStreetActions;

        s.hole_cards     = cards::draw_random_cards(rng, training_config::kEmptyCardsMask, training_config::kHeroHoleCardCount);
        s.opp_hole_cards = cards::draw_random_cards(rng, s.hole_cards, training_config::kOppHoleCardCount);
        s.board_cards    = training_config::kEmptyCardsMask;

        s.action_mask = training_config::kInitialActionMask;

        solver.traverse(s, training_config::kTraverseReachSelf, training_config::kTraverseReachOpp, rng);

        if ((i + 1) % training_config::kProgressPrintEvery == 0)
            std::cout << "  " << (i + 1) << " / " << training_config::kIterations << "\n";
    }

    std::cout << "Training done. Infosets: " << solver.get_infosets().size() << "\n\n";

    // Example: add your own custom lookup datapoint.
    // auto custom_tests = make_default_test_configs();
    // custom_tests.push_back({
    //     "My custom flop spot",
    //     cards::make_hole_cards({cards::Rank::ACE, cards::Suit::SPADES},
    //                            {cards::Rank::QUEEN, cards::Suit::SPADES}),
    //     cards::make_board_cards({
    //         {cards::Rank::KING, cards::Suit::SPADES},
    //         {cards::Rank::TEN, cards::Suit::CLUBS},
    //         {cards::Rank::TWO, cards::Suit::DIAMONDS}
    //     }),
    //     42, 42,
    //     state::Street::FLOP,
    //     state::Position::IN_POSITION,
    //     1,
    //     6
    // });
    // print_example_lookups(solver, custom_tests);

    print_example_lookups(solver);

    return 0;
}
