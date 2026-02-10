#pragma once
#include "bot.h"
#include "decision_state.h"
#include "action.h"
#include "cards.h"

class BoardAwareBot : public Bot {
public:
    action::Action decide(const state::DecisionState& state) noexcept override {
        using namespace cards;

        CardsMask hole = state.hole_cards;
        CardsMask board = state.board_cards;

        // ------------------------------
        // 1️⃣ Check for high cards (Ace or King)
        // ------------------------------
        const CardsMask ACE_MASK = card_to_mask(Rank::ACE, Suit::CLUBS)
                                     | card_to_mask(Rank::ACE, Suit::DIAMONDS)
                                     | card_to_mask(Rank::ACE, Suit::HEARTS)
                                     | card_to_mask(Rank::ACE, Suit::SPADES);

        const CardsMask KING_MASK = card_to_mask(Rank::KING, Suit::CLUBS)
                                      | card_to_mask(Rank::KING, Suit::DIAMONDS)
                                      | card_to_mask(Rank::KING, Suit::HEARTS)
                                      | card_to_mask(Rank::KING, Suit::SPADES);

        if (hole & (ACE_MASK | KING_MASK)) {
            if (state.to_call == 0) return {action::ActionType::RAISE, 10};
            else return {action::ActionType::CALL, state.to_call};
        }

        // ------------------------------
        // 2️⃣ Check for pair with board
        // ------------------------------
        if ((hole & board) != 0) { // any card matches rank on board
            if (state.to_call <= state.pot_size / 10)
                return {action::ActionType::CALL, state.to_call};
            else
                return {action::ActionType::FOLD, 0};
        }

        // ------------------------------
        // 3️⃣ Check for suited cards in hand (suited connectors)
        // ------------------------------
        // Quick check: if both hole cards are same suit
        bool suited = ((hole & 0x1111111111111ULL) != 0); // naive, for demo
        if (suited && state.to_call <= state.pot_size / 20) {
            return {action::ActionType::CALL, state.to_call};
        }

        // ------------------------------
        // Default: fold
        // ------------------------------
        return {action::ActionType::FOLD, 0};
    }
};
