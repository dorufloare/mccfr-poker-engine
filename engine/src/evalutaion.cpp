#include "evaluation.h"
#include "cards.h" 
#include <cstdint>
#include <bit>

namespace eval {

inline int highest_pair(cards::CardsMask mask) noexcept {
    // Check ranks from high to low
    for (int r = cards::RANKS - 1; r >= 0; --r) {
        // Count how many suits of this rank exist
        int count = 0;
        for (int s = 0; s < cards::SUITS; ++s) {
            if (mask & (1ULL << (s * cards::RANKS + r))) ++count;
        }
        if (count >= 2) return r; // found a pair
    }
    return -1; // no pair
}

inline int highest_card(cards::CardsMask mask) noexcept {
    for (int r = cards::RANKS - 1; r >= 0; --r) {
        for (int s = 0; s < cards::SUITS; ++s) {
            if (mask & (1ULL << (s * cards::RANKS + r))) return r;
        }
    }
    return -1; // empty mask, shouldn't happen
}

float evaluate_showdown(cards::CardsMask my_hole, cards::CardsMask opp_hole, cards::CardsMask board) noexcept {
    cards::CardsMask my_hand = my_hole | board;
    cards::CardsMask opp_hand = opp_hole | board;

    int my_pair = highest_pair(my_hand);
    int opp_pair = highest_pair(opp_hand);

    if (my_pair > opp_pair) return 1.f;
    if (my_pair < opp_pair) return 0.f;

    // If pairs are equal or absent, fallback to high card
    int my_high = highest_card(my_hand);
    int opp_high = highest_card(opp_hand);

    if (my_high > opp_high) return 1.f;
    if (my_high < opp_high) return 0.f;

    return 0.5f; // tie
}

float evaluate_hand_monte_carlo(const state::DecisionState& state, uint32_t rng, int num_simulations) {
    int wins = 0;
    int ties = 0;
    int total = 0;

    cards::CardsMask known_cards = state.hole_cards | state.board_cards;

    for (int i = 0; i < num_simulations; ++i) {
        cards::CardsMask used = known_cards;

        // opponent hole cards
        cards::CardsMask opp_hole = cards::draw_random_card(rng, used);
        used |= opp_hole;
        opp_hole |= cards::draw_random_card(rng, used);
        used |= opp_hole;

        // remaining board cards
        cards::CardsMask board = state.board_cards;
        int num_board_cards = cards::cardsInMask(state.board_cards);
        for (int j = num_board_cards; j < 5; ++j) {
            cards::CardsMask c = cards::draw_random_card(rng, used);
            board |= c;
            used |= c;
        }

        float result = evaluate_showdown(state.hole_cards, opp_hole, board);
        if (result == 1.0f) wins++;
        else if (result == 0.5f) ties++;
        total++;
    }

    return (wins + ties * 0.5f) / total;
}

}; // namespace eval