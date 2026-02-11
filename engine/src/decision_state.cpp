#include "decision_state.h"
#include <algorithm>

namespace state {

DecisionState apply_action(const DecisionState& state, action::ActionType action) {
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
        if (next.street != Street::RIVER) {
            next.street = static_cast<Street>(static_cast<uint8_t>(next.street) + 1);
        } else {
            // Move to showdown
        }
    }

    if (!is_terminal_node(next)) next.action_mask = action::ALL;
    else next.action_mask = action::NONE;

    return next;
}

bool is_chance_node(const DecisionState& state) noexcept {
    if (state.street == Street::FLOP &&
        cards::cardsInMask(state.board_cards) == 0)
        return true;

    if (state.street == Street::TURN &&
        cards::cardsInMask(state.board_cards) == 3)
        return true;

    if (state.street == Street::RIVER &&
        cards::cardsInMask(state.board_cards) == 4)
        return true;

    return false;
}

bool is_terminal_node(const DecisionState& state) noexcept {
    if (state.stack_self == 0 || state.stack_opp == 0) return true; // all-in 
    if (get_cards_left(state) == 0 && state.to_call == 0) return true; // showdown
    return false;
}

DecisionState deal_cards(DecisionState state, uint32_t& rng) {
    cards::CardsMask used = state.hole_cards | state.board_cards;

    if (state.street == Street::FLOP)
        state.board_cards |= cards::draw_random_cards(rng, used, 3);
    else if (state.street == Street::TURN)
        state.board_cards |= cards::draw_random_cards(rng, used, 1);
    else if (state.street == Street::RIVER)
        state.board_cards |= cards::draw_random_cards(rng, used, 1);

    return state;
}

int get_cards_left(const DecisionState& state, const int tableSize) noexcept {
    return tableSize - cards::cardsInMask(state.board_cards);
}

} // namespace state
