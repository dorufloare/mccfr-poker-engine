#pragma once
#include <cstdint>

namespace action {

static const int ACTIONS = 4;

using ActionsMask = uint8_t;

enum class ActionType : uint8_t {
    FOLD = 0,
    CALL = 1,
    BET_SMALL = 2,
    BET_BIG = 3
};

const ActionsMask FOLD_MASK  = 1 << static_cast<uint8_t>(ActionType::FOLD);
const ActionsMask CALL_MASK  = 1 << static_cast<uint8_t>(ActionType::CALL);
const ActionsMask BET_SMALL_MASK = 1 << static_cast<uint8_t>(ActionType::BET_SMALL);
const ActionsMask BET_BIG_MASK = 1 << static_cast<uint8_t>(ActionType::BET_BIG);    
const ActionsMask ALL_ACTIONS_MASK        = FOLD_MASK | CALL_MASK | BET_SMALL_MASK | BET_BIG_MASK;
const ActionsMask NONE       = 0;

struct Action {
    ActionType type;
    uint32_t amount; 
};

using History = uint32_t;

// history is a 32-bit packed sequence of actions (2 bits each)
// CALL= 01, BET_SMALL = 10, BET_BIG = 11, FOLD = 00
// oldest action is in the highest bits

inline History push(History h, ActionType a) noexcept {
    return (h << 2) | static_cast<uint8_t>(a);
}

inline ActionType last(History h) noexcept {
    return static_cast<ActionType>(h & 0b11);
}

inline History pop(History h) noexcept {
    return h >> 2;
}

inline int length(History h) noexcept {
    int len = 0;
    while (h) {
        ++len;
        h >>= 2;
    }
    return len;
}

inline int player_to_act(History h) noexcept {
    return length(h) & 1;
}

};  // namespace action