#include "decision_state.h"
#include <algorithm>
#include <utility>

namespace state {

DecisionState apply_action(const DecisionState& state, action::ActionType action) {
    DecisionState next = state;

    if (action == action::ActionType::FOLD) {
        // Current player folds - opponent wins the pot
        next.stack_self = 0;    
        next.stack_opp += next.pot_size;
        next.pot_size = 0;
        next.action_mask = 0;
        next.to_call = 0;
        
        // Swap perspectives so "self" becomes the winner (opponent)
        std::swap(next.stack_self, next.stack_opp);
        std::swap(next.hole_cards, next.opp_hole_cards);
        next.position = (next.position == Position::IN_POSITION) 
            ? Position::OUT_OF_POSITION 
            : Position::IN_POSITION;
        return next;
    }

    if (action == action::ActionType::CALL) {
        Chips call_amount = std::min(state.to_call, state.stack_self);

        next.stack_self -= call_amount;
        next.pot_size += call_amount;

        next.to_call = 0; 
    }

    if (action == action::ActionType::BET_SMALL) {
        Chips call_part = std::min(state.to_call, next.stack_self);
        Chips remaining = next.stack_self - call_part;
        Chips raise_part = static_cast<Chips>(std::max(2, state.pot_size / 2));
        raise_part = std::min(raise_part, remaining);

        Chips total = call_part + raise_part;
        next.stack_self -= total;
        next.pot_size += total;
        next.to_call = raise_part;
    }

    if (action == action::ActionType::BET_BIG) {
        Chips call_part = std::min(state.to_call, next.stack_self);
        Chips remaining = next.stack_self - call_part;
        Chips raise_part = static_cast<Chips>(std::max(5, static_cast<int>(state.pot_size)));
        raise_part = std::min(raise_part, remaining);

        Chips total = call_part + raise_part;
        next.stack_self -= total;
        next.pot_size += total;
        next.to_call = raise_part;
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
        next.street_actions = 1; // The raiser has acted
    }

    // Swap perspectives: after any action, it's the opponent's turn
    // This keeps state consistent with MCCFR traversal which swaps reach probabilities
    std::swap(next.stack_self, next.stack_opp);
    std::swap(next.hole_cards, next.opp_hole_cards);
    next.position = (next.position == Position::IN_POSITION) 
        ? Position::OUT_OF_POSITION 
        : Position::IN_POSITION;

    if (!is_terminal_node(next)) {
        action::ActionsMask mask = action::CALL_MASK;
        if (next.to_call > 0)
            mask |= action::FOLD_MASK;
        if (next.stack_self > next.to_call)
            mask |= action::BET_SMALL_MASK | action::BET_BIG_MASK;
        next.action_mask = mask;
    } else {
        next.action_mask = action::NONE;
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
