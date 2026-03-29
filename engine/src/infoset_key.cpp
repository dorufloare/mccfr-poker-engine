#include "infoset_key.h"
#include "cards.h"
#include "evaluation.h"
#include "training_config.h"
#include "utils.h"
#include <algorithm>
#include <functional>

namespace mccfr {

namespace {

inline uint32_t deterministic_seed(const InfoSetKey& k) {
    uint64_t mix = k.hole;
    mix ^= (k.board + 0x9e3779b97f4a7c15ULL + (mix << 6) + (mix >> 2));
    mix ^= (static_cast<uint64_t>(k.street) << 8);
    mix ^= (static_cast<uint64_t>(k.position) << 16);
    mix ^= (static_cast<uint64_t>(k.street_actions) << 24);
    mix ^= (static_cast<uint64_t>(k.to_call) << 32);
    uint32_t seed = static_cast<uint32_t>(mix ^ (mix >> 32));
    return random_utils::init_rng(seed == 0 ? 1u : seed);
}

inline int8_t ehs_bucket(const InfoSetKey& k) {
    if (k.cached_ehs_bucket != -1) 
        return k.cached_ehs_bucket;

    state::DecisionState s{};
    s.hole_cards = k.hole;
    s.board_cards = k.board;
    uint32_t rng = deterministic_seed(k);
    
    int ehs_sims = training_config::kEhsSimsPreflop;
    if (k.street == static_cast<uint8_t>(state::Street::FLOP))
        ehs_sims = training_config::kEhsSimsFlop;
    else if (k.street == static_cast<uint8_t>(state::Street::TURN))
        ehs_sims = training_config::kEhsSimsTurn;
    else if (k.street == static_cast<uint8_t>(state::Street::RIVER))
        ehs_sims = training_config::kEhsSimsRiver;

    const float ehs = std::clamp(eval::evaluate_hand_EHS(s, rng, ehs_sims), 0.0f, 1.0f);
    k.cached_ehs_bucket = static_cast<int8_t>(std::min(9, static_cast<int>(ehs * 10.0f)));
    return k.cached_ehs_bucket;
}

inline uint8_t straight_draw_type(const InfoSetKey& k) {
    return eval::get_straight_draw(k.hole | k.board);
}

inline uint8_t flush_draw_type(const InfoSetKey& k) {
    return eval::get_flush_draw(k.hole | k.board);
}

inline uint8_t to_call_bucket(int to_call) {
    if (to_call == 0) return 0;
    if (to_call <= 5) return 1;
    if (to_call <= 15) return 2;
    return 3;
}

inline uint8_t street_actions_bucket(uint8_t street_actions) {
    return static_cast<uint8_t>(std::min<uint8_t>(street_actions, 2));
}

} // namespace

bool InfoSetKey::operator==(const InfoSetKey& other) const {
    return ehs_bucket(*this) == ehs_bucket(other) &&
           straight_draw_type(*this) == straight_draw_type(other) &&
           flush_draw_type(*this) == flush_draw_type(other) &&
           street == other.street &&
           position == other.position &&
           street_actions_bucket(street_actions) == street_actions_bucket(other.street_actions) &&
           to_call_bucket(to_call) == to_call_bucket(other.to_call);
}

size_t InfoSetKeyHash::operator()(const InfoSetKey& k) const noexcept {
    size_t seed = 0;

    hash_combine(seed, std::hash<int8_t>{}(ehs_bucket(k)));
    hash_combine(seed, std::hash<uint8_t>{}(straight_draw_type(k)));
    hash_combine(seed, std::hash<uint8_t>{}(flush_draw_type(k)));
    hash_combine(seed, std::hash<uint8_t>{}(k.street));
    hash_combine(seed, std::hash<uint8_t>{}(k.position));
    hash_combine(seed, std::hash<uint8_t>{}(street_actions_bucket(k.street_actions)));
    hash_combine(seed, std::hash<uint8_t>{}(to_call_bucket(k.to_call)));

    return seed;
}

InfoSetKey make_key(const state::DecisionState& s) {
    InfoSetKey key;
    key.hole = s.hole_cards;
    key.board = s.board_cards;
    key.street = static_cast<uint8_t>(s.street);
    key.position = static_cast<uint8_t>(s.position);
    key.street_actions = s.street_actions;
    key.to_call = s.to_call;
    return key;
}

std::string infoset_key_to_string(const InfoSetKey& key) {
    return "Hole: " + cards::mask_to_string(key.hole) +
           ", Board: " + cards::mask_to_string(key.board) +
           ", Street: " + std::to_string(static_cast<int>(key.street)) +
           ", Position: " + std::to_string(static_cast<int>(key.position)) +
           ", Actions: " + std::to_string(static_cast<int>(key.street_actions)) +
           ", To Call: " + std::to_string(static_cast<int>(key.to_call));
}

} // namespace mccfr
