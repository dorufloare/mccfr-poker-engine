#pragma once
#include <cstdint>
#include "cards.h"

namespace hand_eval {

//simple pair bucket evaluator for basic test bot
inline uint8_t pair_bucket(cards::CardsMask cards) {
    for (int r = 12; r >= 0; --r) {
        int count = cards::cardsInMask(cards & cards::rank_mask(r));
        if (count >= 2) {
            if (r >= 10) return 3; // JJ+
            if (r >= 6)  return 2; // 77–TT
            return 1;             // 22–66
        }
    }
    return 0; // no pair
}

};