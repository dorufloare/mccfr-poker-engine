#include "decision_state.h"
#include <algorithm>

namespace state {

DecisionState apply_action(const DecisionState& state, action::ActionType action, uint32_t& rng) {
    DecisionState next = state;

    if (action == action::ActionType::FOLD) {
        // Opponent wins the pot
        next.stack_self = 0;    
        next.stack_opp += next.pot_size;
        next.pot_size = 0;
        next.action_mask = 0;
        next.to_call = 0;
        return next;
    }

    if (action == action::ActionType::CALL) {
        Chips call_amount = std::min(state.to_call, state.stack_self);

        next.stack_self -= call_amount;
        next.pot_size += call_amount;

        next.to_call = 0; 
    }

    // TODO: raise logic
    if (action == action::ActionType::RAISE) {
        Chips raise_amount = 1;

        raise_amount = std::min(raise_amount, next.stack_self);
        next.stack_self -= raise_amount;
        next.pot_size += raise_amount;
        
        next.to_call = raise_amount;
    }

    if (next.to_call == 0) {
        cards::CardsMask used = next.hole_cards | next.board_cards;

        if (next.street == Street::PREFLOP) {
            next.street = Street::FLOP;
            next.board_cards |= cards::draw_random_cards(rng, used, 3);
        }
        else if (next.street == Street::FLOP) {
            next.street = Street::TURN;
            next.board_cards |= cards::draw_random_cards(rng, used, 1);
        }
        else if (next.street == Street::TURN) {
            next.street = Street::RIVER;
            next.board_cards |= cards::draw_random_cards(rng, used, 1);
        }
    }

    if (!is_terminal(next)) next.action_mask = action::ALL;
    else next.action_mask = action::NONE;

    return next;
}

bool is_terminal(const DecisionState& state) noexcept {
    if (state.stack_self == 0 || state.stack_opp == 0) return true; // all-in 
    if (get_cards_left(state) == 0 && state.to_call == 0) return true; // showdown
    return false;
}

int get_cards_left(const DecisionState& state, const int tableSize) noexcept {
    return tableSize - cards::cardsInMask(state.board_cards);
}

} // namespace state
