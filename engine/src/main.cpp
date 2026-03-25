#include <iostream>
#include <cstdint>
#include "decision_state.h"
#include "cards.h"
#include "mccfr.h"
#include "action.h"
#include "info_set.h"
#include "utils.h"
#include "evaluation.h"

using namespace state;
using namespace cards;
using namespace action;
using namespace mccfr;

void print_strategy(const char* label, MCCFR& solver, const InfoSetKey& key) {
    auto& infosets = solver.get_infosets();
    auto it = infosets.find(key);
    std::cout << label << "  [" << infoset_key_to_string(key) << "]\n";
    if (it == infosets.end()) {
        std::cout << "  (not visited during training)\n\n";
        return;
    }
    float strat[action::ACTIONS];
    it->second.get_average_strategy(strat);
    std::cout << " ";
    for (int i = 0; i < action::ACTIONS; ++i) {
        if (i > 0) std::cout << ",";
        const char* names[] = {"FOLD", "CALL", "BET_SMALL", "BET_BIG"};
        std::cout << " " << names[i] << ": " << strat[i] << '\n';
    }
    std::cout << "\n\n";
}

InfoSetKey make_test_key(CardsMask hole, CardsMask board,
                         Street street, Position pos, uint8_t actions,
                         uint8_t to_call = 0) {
    InfoSetKey k;
    k.hole = hole;
    k.board = board;
    k.street = static_cast<uint8_t>(street);
    k.position = static_cast<uint8_t>(pos);
    k.street_actions = actions;
    k.to_call = to_call;
    return k;
}

int main() {
    srand(time(0));
    uint32_t rng = random_utils::init_rng(static_cast<uint32_t>(rand()));

    // ── Train ───────────────────────────────────────────────────────────
    MCCFR solver;
    const int iterations = 10000;

    std::cout << "Training MCCFR (" << iterations << " iterations)...\n";
    for (int i = 0; i < iterations; ++i) {
        DecisionState s;
        s.hand_id        = 1;
        s.street         = Street::PREFLOP;
        s.position       = Position::OUT_OF_POSITION;
        s.pot_size       = 9;
        s.stack_self     = 50;
        s.stack_opp      = 50;
        s.to_call        = 3;
        s.street_actions = 0;

        s.hole_cards     = draw_random_cards(rng, 0, 2);
        s.opp_hole_cards = draw_random_cards(rng, s.hole_cards, 2);
        s.board_cards    = 0;

        s.action_mask = FOLD_MASK | CALL_MASK | BET_SMALL_MASK | BET_BIG_MASK;

        solver.traverse(s, 1.f, 1.f, rng);

        if ((i + 1) % 100 == 0)
            std::cout << "  " << (i + 1) << " / " << iterations << "\n";
    }

    std::cout << "Training done. Infosets: " << solver.get_infosets().size() << "\n\n";

    // ── Test hands ──────────────────────────────────────────────────────
    std::cout << "========== EXAMPLE LOOKUPS ==========\n\n";

    // Note: to_call=3 is bucket 1 (1-3), to_call=7 is bucket 2 (4-10).
    // Preflop training starts with to_call=3.

    // ── Preflop ─────────────────────────────────────────────────────────
    CardsMask aces = card_to_mask(Rank::ACE, Suit::HEARTS)
                   | card_to_mask(Rank::ACE, Suit::SPADES);
    print_strategy("AA preflop, first action (to_call=3)",
        solver, make_test_key(aces, 0, Street::PREFLOP, Position::OUT_OF_POSITION, 0, 3));
    print_strategy("AA preflop, facing re-raise (to_call=7)",
        solver, make_test_key(aces, 0, Street::PREFLOP, Position::OUT_OF_POSITION, 1, 7));

    CardsMask kings = card_to_mask(Rank::KING, Suit::HEARTS)
                    | card_to_mask(Rank::KING, Suit::SPADES);
    print_strategy("KK preflop, first action (to_call=3)",
        solver, make_test_key(kings, 0, Street::PREFLOP, Position::OUT_OF_POSITION, 0, 3));

    CardsMask seven_two = card_to_mask(Rank::SEVEN, Suit::HEARTS)
                        | card_to_mask(Rank::TWO, Suit::SPADES);
    print_strategy("72o preflop, first action (to_call=3)",
        solver, make_test_key(seven_two, 0, Street::PREFLOP, Position::OUT_OF_POSITION, 0, 3));
    print_strategy("72o preflop, facing raise (to_call=7)",
        solver, make_test_key(seven_two, 0, Street::PREFLOP, Position::OUT_OF_POSITION, 1, 7));

    // ── Flop ─────────────────────────────────────────────────────────────
    // KQs: top pair top kicker on K-8-3
    CardsMask kqs     = card_to_mask(Rank::KING,  Suit::HEARTS)
                      | card_to_mask(Rank::QUEEN, Suit::HEARTS);
    CardsMask k83     = card_to_mask(Rank::KING,   Suit::DIAMONDS)
                      | card_to_mask(Rank::EIGHT,  Suit::CLUBS)
                      | card_to_mask(Rank::THREE,  Suit::SPADES);
    print_strategy("KQs on K-8-3, first to act (to_call=0)",
        solver, make_test_key(kqs, k83, Street::FLOP, Position::OUT_OF_POSITION, 0, 0));
    print_strategy("KQs on K-8-3, facing small bet (to_call=7)",
        solver, make_test_key(kqs, k83, Street::FLOP, Position::IN_POSITION, 1, 7));

    // JJ: overpair on A-9-4 (scary board)
    CardsMask jacks   = card_to_mask(Rank::JACK, Suit::CLUBS)
                      | card_to_mask(Rank::JACK, Suit::DIAMONDS);
    CardsMask a94     = card_to_mask(Rank::ACE,  Suit::CLUBS)
                      | card_to_mask(Rank::NINE, Suit::HEARTS)
                      | card_to_mask(Rank::FOUR, Suit::SPADES);
    print_strategy("JJ on A-9-4, first to act (to_call=0)",
        solver, make_test_key(jacks, a94, Street::FLOP, Position::OUT_OF_POSITION, 0, 0));
    print_strategy("JJ on A-9-4, facing bet (to_call=7)",
        solver, make_test_key(jacks, a94, Street::FLOP, Position::IN_POSITION, 1, 7));
    print_strategy("JJ on A-9-4, facing big bet (to_call=11)",
        solver, make_test_key(jacks, a94, Street::FLOP, Position::IN_POSITION, 1, 11));

    // 44: bottom set on 4-8-K
    CardsMask fours   = card_to_mask(Rank::FOUR, Suit::CLUBS)
                      | card_to_mask(Rank::FOUR, Suit::DIAMONDS);
    CardsMask four_board = card_to_mask(Rank::FOUR,  Suit::HEARTS)
                         | card_to_mask(Rank::EIGHT, Suit::SPADES)
                         | card_to_mask(Rank::KING,  Suit::CLUBS);
    print_strategy("44 flopped bottom set (4-8-K), first to act (to_call=0)",
        solver, make_test_key(fours, four_board, Street::FLOP, Position::OUT_OF_POSITION, 0, 0));

    // 96o: gutshot + backdoor on 7-8-2
    CardsMask nine_six = card_to_mask(Rank::NINE, Suit::HEARTS)
                       | card_to_mask(Rank::SIX,  Suit::SPADES);
    CardsMask draw_board = card_to_mask(Rank::SEVEN, Suit::CLUBS)
                         | card_to_mask(Rank::EIGHT, Suit::DIAMONDS)
                         | card_to_mask(Rank::TWO,   Suit::HEARTS);
    print_strategy("96o gutshot on 7-8-2, first to act (to_call=0)",
        solver, make_test_key(nine_six, draw_board, Street::FLOP, Position::OUT_OF_POSITION, 0, 0));
    print_strategy("96o gutshot on 7-8-2, facing bet (to_call=7)",
        solver, make_test_key(nine_six, draw_board, Street::FLOP, Position::IN_POSITION, 1, 7));

    // ── River ────────────────────────────────────────────────────────────
    // AA: aces full on A-K-Q-4-2
    CardsMask river_aces  = card_to_mask(Rank::ACE, Suit::HEARTS)
                          | card_to_mask(Rank::ACE, Suit::SPADES);
    CardsMask river_board = card_to_mask(Rank::ACE,   Suit::CLUBS)
                          | card_to_mask(Rank::KING,  Suit::DIAMONDS)
                          | card_to_mask(Rank::QUEEN, Suit::HEARTS)
                          | card_to_mask(Rank::FOUR,  Suit::SPADES)
                          | card_to_mask(Rank::TWO,   Suit::CLUBS);
    print_strategy("AA (set of aces) on A-K-Q-4-2 river, first to act (to_call=0)",
        solver, make_test_key(river_aces, river_board, Street::RIVER, Position::OUT_OF_POSITION, 0, 0));
    print_strategy("AA (set of aces) on A-K-Q-4-2 river, facing bet (to_call=7)",
        solver, make_test_key(river_aces, river_board, Street::RIVER, Position::IN_POSITION, 1, 7));

    // 23o: complete air on K-Q-J-9-7 river facing a big bet
    CardsMask air       = card_to_mask(Rank::TWO,   Suit::CLUBS)
                        | card_to_mask(Rank::THREE, Suit::DIAMONDS);
    CardsMask scary_run = card_to_mask(Rank::KING,  Suit::HEARTS)
                        | card_to_mask(Rank::QUEEN, Suit::CLUBS)
                        | card_to_mask(Rank::JACK,  Suit::SPADES)
                        | card_to_mask(Rank::NINE,  Suit::DIAMONDS)
                        | card_to_mask(Rank::SEVEN, Suit::HEARTS);
    print_strategy("23o (air) on K-Q-J-9-7 river, facing big bet (to_call=7)",
        solver, make_test_key(air, scary_run, Street::RIVER, Position::IN_POSITION, 1, 7));

    return 0;
}
