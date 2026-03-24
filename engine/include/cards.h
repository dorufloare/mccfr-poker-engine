#pragma once
#include <cstdint>
#include <initializer_list>
#include <utility>
#include "utils.h"
#include <string>

namespace cards {

const int RANKS = 13;
const int SUITS = 4;

enum class Suit : uint8_t { CLUBS, DIAMONDS, HEARTS, SPADES };
enum class Rank : uint8_t {
    TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN,
    JACK, QUEEN, KING, ACE
};

using CardsMask = uint64_t;

constexpr CardsMask rank_mask(int r) noexcept {
    return (1ULL << r)
         | (1ULL << (r + 13))
         | (1ULL << (r + 26))
         | (1ULL << (r + 39));
}

inline CardsMask card_to_mask(Rank rank, Suit suit) noexcept {
    return 1ULL << (static_cast<int>(suit) * 13 + static_cast<int>(rank));
}

inline CardsMask make_hole_cards(Rank r1, Suit s1, Rank r2, Suit s2) noexcept {
    return card_to_mask(r1, s1) | card_to_mask(r2, s2);
}

inline CardsMask make_hole_cards(std::pair<Rank, Suit> card1, std::pair<Rank, Suit> card2) noexcept {
    return card_to_mask(card1.first, card1.second) | card_to_mask(card2.first, card2.second);
}

inline CardsMask make_board_cards(std::initializer_list<std::pair<Rank, Suit>> cards) noexcept {
    CardsMask mask = 0;
    for (const auto &c : cards) mask |= card_to_mask(c.first, c.second);
    return mask;
}

template<typename... Args>
inline CardsMask make_board_cards(Args... args) noexcept {
    CardsMask mask = 0;
    ((mask |= card_to_mask(args.first, args.second)), ...);
    return mask;
}

inline CardsMask combine_masks(std::initializer_list<CardsMask> masks) noexcept {
    CardsMask result = 0;
    for (CardsMask m : masks) result |= m;
    return result;
}

inline CardsMask draw_random_card(uint32_t& rng, uint64_t used_mask, int num_ranks = 13, int num_suits = 4) {
    int total = num_ranks * num_suits;
    for (;;) {
        uint32_t r = random_utils::fast_rand(rng);
        int idx = static_cast<int>((r >> 8) % total);  // use upper bits, less biased
        CardsMask card = 1ull << idx;
        if ((card & used_mask) == 0)
            return card;
    }
}

inline CardsMask draw_random_cards(uint32_t& rng, uint64_t used_mask, int num_cards, int num_ranks = 13, int num_suits = 4) {
    CardsMask result = 0;
    for (int i = 0; i < num_cards; ++i) {
        result |= draw_random_card(rng, used_mask | result, num_ranks, num_suits);
    } 
    return result;

}

inline int cardsInMask(cards::CardsMask x) noexcept {
    return __builtin_popcountll(x);
}

inline std::string mask_to_string(CardsMask mask) {
    std::string result;
    for (int s = 0; s < SUITS; ++s) {
        for (int r = 0; r < RANKS; ++r) {
            if (mask & (1ULL << (s * RANKS + r))) {
                char rank_char = "23456789TJQKA"[r];
                char suit_char = "CDHS"[s];
                result += rank_char;
                result += suit_char;
                result += ' ';
            }
        }
    }
    return result.empty() ? "None" : result;
}

};// namespace cards

