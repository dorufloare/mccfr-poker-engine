#include "decision_state.h"
#include <algorithm>

namespace state {

DecisionState apply_action(const DecisionState& state, action::ActionType action) {
    DecisionState next = state;

    if (action == action::ActionType::FOLD) {
        if (state.to_call == 0) {
            return next; 
        }

        next.stack_opp += next.pot_size;
        next.pot_size = 0;
        next.action_mask = action::NONE;
        next.to_call = 0;
        return next;
    }

    if (action == action::ActionType::CALL) {
        Chips call_amount = std::min(state.to_call, state.stack_self);

        next.stack_self -= call_amount;
        next.pot_size += call_amount;

        next.to_call = 0; 
    }

    if (action == action::ActionType::RAISE) {
        Chips raise_amount = std::max(static_cast<Chips>(1), static_cast<Chips>(state.pot_size / 2));
        Chips total_bet = state.to_call + raise_amount;
        total_bet = std::min(total_bet, next.stack_self);
        
        next.stack_self -= total_bet;
        next.pot_size += total_bet;
        
        next.to_call = raise_amount;
    }

    // Increment action count on current street
    next.street_actions++;

    if (next.to_call == 0) {
        // Both players called/checked - advance street
        if (next.street != Street::RIVER) {
            next.street = static_cast<Street>(static_cast<uint8_t>(next.street) + 1);
            next.street_actions = 0; 
        } else {
            // Stay on river for showdown
        }
    } else {
        // Someone raised - reset action count as we need both players to act again
        next.street_actions = 1; 
    }

    if (!is_terminal_node(next)) {
        // Set valid actions based on game state
        if (next.to_call == 0) {
            // No bet to call: can check (CALL) or bet (RAISE), but not fold
            next.action_mask = action::CALL_MASK | action::RAISE_MASK;
        } else {
            // Bet to call: can fold, call, or raise
            next.action_mask = action::ALL;
        }
    } else {
        next.action_mask = action::NONE;
    }

    // Swap perspectives (self becomes opp and vice versa) since it's now opponent's turn
    if (!is_terminal_node(next)) {
        std::swap(next.stack_self, next.stack_opp);
        next.position = (next.position == Position::IN_POSITION) ? Position::OUT_OF_POSITION : Position::IN_POSITION;
    }

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
    // On river with no bet to call: only terminal if both players have acted (street_actions >= 2)
    if (get_cards_left(state) == 0 && state.to_call == 0 && state.street_actions >= 2) return true; // showdown
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
