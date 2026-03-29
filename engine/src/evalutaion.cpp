#include "evaluation.h"
#include "cards.h"
#include <cstdint>
#include <bit>

namespace eval {

// ── Hand rank encoding ───────────────────────────────────────────────────
// Upper 4 bits  = hand category (0-8, higher = better)
// Lower 28 bits = tiebreaker ranks packed 4 bits each, best rank first
// Comparing two uint32_t values directly breaks ties correctly.

static constexpr uint32_t HIGH_CARD      = 0u << 28;
static constexpr uint32_t ONE_PAIR       = 1u << 28;
static constexpr uint32_t TWO_PAIR       = 2u << 28;
static constexpr uint32_t THREE_OF_KIND  = 3u << 28;
static constexpr uint32_t STRAIGHT       = 4u << 28;
static constexpr uint32_t FLUSH          = 5u << 28;
static constexpr uint32_t FULL_HOUSE     = 6u << 28;
static constexpr uint32_t FOUR_OF_KIND   = 7u << 28;
static constexpr uint32_t STRAIGHT_FLUSH = 8u << 28;

// Highest set bit position in a non-zero 16-bit mask
static inline int msb(uint16_t m) noexcept {
    return 31 - __builtin_clz((unsigned)m);
}

// Pack up to 5 ranks (4 bits each) from a mask, highest rank first, into bits [0..19]
static inline uint32_t top5(uint16_t mask) noexcept {
    uint32_t result = 0;
    int shift = 20;
    for (int r = 12; r >= 0 && shift > 0; --r) {
        if (mask & (1 << r)) {
            shift -= 4;
            result |= (uint32_t)r << shift;
        }
    }
    return result;
}

// Returns the top rank of a straight in `mask`, or -1 if none.
// A-2-3-4-5 (wheel) returns 3 (five-high).
static inline int find_straight(uint16_t mask) noexcept {
    for (int top = 12; top >= 4; --top)
        if (((mask >> (top - 4)) & 0x1F) == 0x1F) return top;
    if ((mask & 0x100F) == 0x100F) return 3; // A-2-3-4-5
    return -1;
}

// Evaluate the best 5-card hand from up to 7 cards.
// Returns a uint32_t: higher value = stronger hand.
static uint32_t eval_hand(cards::CardsMask hand) noexcept {
    // Split into 4 suit masks (13 bits each)
    uint16_t sm[4];
    for (int s = 0; s < 4; ++s)
        sm[s] = (uint16_t)((hand >> (s * 13)) & 0x1FFF);

    // Union of all ranks present, and per-rank counts
    uint16_t rank_or = sm[0] | sm[1] | sm[2] | sm[3];
    uint8_t  rc[13]  = {};
    for (int r = 0; r < 13; ++r)
        for (int s = 0; s < 4; ++s)
            if (sm[s] & (1 << r)) ++rc[r];

    // Flush detection
    int flush_suit = -1;
    for (int s = 0; s < 4; ++s)
        if (std::popcount((unsigned)sm[s]) >= 5) { flush_suit = s; break; }

    // Straight flush
    if (flush_suit >= 0) {
        int sf = find_straight(sm[flush_suit]);
        if (sf >= 0) return STRAIGHT_FLUSH | (uint32_t)sf;
    }

    // Build pair/trip/quad masks
    uint16_t quads = 0, trips = 0, pairs = 0;
    for (int r = 0; r < 13; ++r) {
        if      (rc[r] == 4) quads |= (1 << r);
        else if (rc[r] == 3) trips |= (1 << r);
        else if (rc[r] == 2) pairs |= (1 << r);
    }

    // Four of a kind
    if (quads) {
        int q = msb(quads);
        int k = -1;
        for (int r = 12; r >= 0 && k < 0; --r)
            if (r != q && (rank_or & (1 << r))) k = r;
        return FOUR_OF_KIND | ((uint32_t)q << 4) | (uint32_t)k;
    }

    // Full house: best trip + best available pair (other trips count as pairs)
    if (trips) {
        int t          = msb(trips);
        uint16_t pairs2 = pairs | (trips & (uint16_t)~(1 << t));
        if (pairs2) {
            int p = msb(pairs2);
            return FULL_HOUSE | ((uint32_t)t << 4) | (uint32_t)p;
        }
    }

    // Flush (top 5 cards of flush suit)
    if (flush_suit >= 0)
        return FLUSH | top5(sm[flush_suit]);

    // Straight
    int st = find_straight(rank_or);
    if (st >= 0) return STRAIGHT | (uint32_t)st;

    // Three of a kind
    if (trips) {
        int t = msb(trips);
        uint16_t km = rank_or & (uint16_t)~(1 << t);
        int k0 = -1, k1 = -1;
        for (int r = 12; r >= 0; --r) {
            if (!(km & (1 << r))) continue;
            if      (k0 < 0) k0 = r;
            else if (k1 < 0) { k1 = r; break; }
        }
        return THREE_OF_KIND | ((uint32_t)t << 8) | ((uint32_t)k0 << 4) | (uint32_t)k1;
    }

    // Two pair (pick best two pairs + best kicker)
    if (std::popcount((unsigned)pairs) >= 2) {
        int p1 = msb(pairs);
        int p2 = msb((uint16_t)(pairs & ~(1 << p1)));
        int k  = -1;
        for (int r = 12; r >= 0 && k < 0; --r)
            if (r != p1 && r != p2 && (rank_or & (1 << r))) k = r;
        return TWO_PAIR | ((uint32_t)p1 << 8) | ((uint32_t)p2 << 4) | (uint32_t)k;
    }

    // One pair
    if (pairs) {
        int p = msb(pairs);
        uint16_t km = rank_or & (uint16_t)~(1 << p);
        int k0 = -1, k1 = -1, k2 = -1;
        for (int r = 12; r >= 0; --r) {
            if (!(km & (1 << r))) continue;
            if      (k0 < 0) k0 = r;
            else if (k1 < 0) k1 = r;
            else if (k2 < 0) { k2 = r; break; }
        }
        return ONE_PAIR | ((uint32_t)p << 12) | ((uint32_t)k0 << 8) | ((uint32_t)k1 << 4) | (uint32_t)k2;
    }

    // High card
    return HIGH_CARD | top5(rank_or);
}

inline bool has_open_ended_straight_draw(cards::CardsMask hand) noexcept {
    uint16_t rank_or = 0;
    for (int s = 0; s < 4; ++s)
        rank_or |= (uint16_t)((hand >> (s * 13)) & 0x1FFF);

    for (int top = 12; top >= 5; --top)
        if (((rank_or >> (top - 5)) & 0x3F) == 0x3E) return true;
    if ((rank_or & 0x180F) == 0x180E) return true; // A-2-3-4 open-ended

    return false;
}

inline bool has_gunshot_straight_draw(cards::CardsMask hand) noexcept {
    uint16_t rank_or = 0;
    for (int s = 0; s < 4; ++s)
        rank_or |= (uint16_t)((hand >> (s * 13)) & 0x1FFF);

    for (int top = 12; top >= 5; --top)
        if (((rank_or >> (top - 5)) & 0x3F) == 0x3D) return true;
    if ((rank_or & 0x180F) == 0x180B) return true; // A-2-3-5 gutshot

    return false;

} 

inline bool has_flush_draw(cards::CardsMask hand) noexcept {
    for (int s = 0; s < 4; ++s)
        if (std::popcount((unsigned)((hand >> (s * 13)) & 0x1FFF)) == 4) return true;
    return false;
}

//  Public API 
float evaluate_hand_EHS(const state::DecisionState& state, uint32_t& rng, int num_simulations) {
    if (num_simulations <= 0) return 0.5f;

    int ahead_now = 0;
    int tied_now = 0;
    int behind_now = 0;

    float np_num = 0.0f; // A->B + 0.5*A->T + 0.5*T->B
    float pp_num = 0.0f; // B->A + 0.5*B->T + 0.5*T->A

    const cards::CardsMask known = state.hole_cards | state.board_cards;

    for (int i = 0; i < num_simulations; ++i) {
        cards::CardsMask used = known;

        cards::CardsMask opp = cards::draw_random_card(rng, used);
        used |= opp;
        opp |= cards::draw_random_card(rng, used);
        used |= opp;

        cards::CardsMask board_full = state.board_cards;
        for (int j = cards::cardsInMask(board_full); j < 5; ++j) {
            cards::CardsMask c = cards::draw_random_card(rng, used);
            board_full |= c;
            used |= c;
        }

        const float r_now = evaluate_showdown(state.hole_cards, opp, state.board_cards);
        const float r_full = evaluate_showdown(state.hole_cards, opp, board_full);

        if (r_now == 1.0f) {
            ++ahead_now;
            if (r_full == 0.0f) np_num += 1.0f;      // A->B
            else if (r_full == 0.5f) np_num += 0.5f; // A->T
        } else if (r_now == 0.5f) {
            ++tied_now;
            if (r_full == 0.0f) np_num += 0.5f;      // T->B
            else if (r_full == 1.0f) pp_num += 0.5f; // T->A
        } else {
            ++behind_now;
            if (r_full == 1.0f) pp_num += 1.0f;      // B->A
            else if (r_full == 0.5f) pp_num += 0.5f; // B->T
        }
    }

    const float N = static_cast<float>(num_simulations);
    const float HS = (ahead_now + 0.5f * tied_now) / N;

    //ties count as half ahead and half behind for the purposes of NP/PP calculation
    const float np_den = ahead_now + 0.5f * tied_now;
    const float pp_den = behind_now + 0.5f * tied_now;

    //probability of your hand getting worse, given that you are not behind currently
    const float NP = (np_den > 0.0f) ? (np_num / np_den) : 0.0f;

    //probability of your hand improving, given that you are not ahead currently
    const float PP = (pp_den > 0.0f) ? (pp_num / pp_den) : 0.0f;

    return HS * (1.0f - NP) + (1.0f - HS) * PP;
}

// keep draw types the same for now, probably not worth separating them yet
uint8_t get_straight_draw(cards::CardsMask hand) noexcept {
   if (has_open_ended_straight_draw(hand)) return 1;
   if (has_gunshot_straight_draw(hand)) return 1;
   return 0;
}

uint8_t get_flush_draw(cards::CardsMask hand) noexcept {
    return has_flush_draw(hand) ? 1 : 0;
}

int highest_pair(cards::CardsMask mask) noexcept {
    for (int r = cards::RANKS - 1; r >= 0; --r) {
        int count = 0;
        for (int s = 0; s < cards::SUITS; ++s)
            if (mask & (1ULL << (s * cards::RANKS + r))) ++count;
        if (count >= 2) return r;
    }
    return -1;
}

int highest_card(cards::CardsMask mask) noexcept {
    for (int r = cards::RANKS - 1; r >= 0; --r)
        for (int s = 0; s < cards::SUITS; ++s)
            if (mask & (1ULL << (s * cards::RANKS + r))) return r;
    return -1;
}

float evaluate_showdown(cards::CardsMask my_hole, cards::CardsMask opp_hole, cards::CardsMask board) noexcept {
    uint32_t my_rank  = eval_hand(my_hole  | board);
    uint32_t opp_rank = eval_hand(opp_hole | board);
    if (my_rank > opp_rank) return 1.f;
    if (my_rank < opp_rank) return 0.f;
    return 0.5f;
}

float evaluate_showdown_simple(cards::CardsMask my_hole, cards::CardsMask opp_hole, cards::CardsMask board) noexcept {
    return evaluate_showdown(my_hole, opp_hole, board);
}

float evaluate_hand_monte_carlo(const state::DecisionState& state, uint32_t rng, int num_simulations) {
    int wins = 0, ties = 0;
    cards::CardsMask known = state.hole_cards | state.board_cards;

    for (int i = 0; i < num_simulations; ++i) {
        cards::CardsMask used = known;

        cards::CardsMask opp = cards::draw_random_card(rng, used);
        used |= opp;
        opp  |= cards::draw_random_card(rng, used);
        used |= opp;

        cards::CardsMask board = state.board_cards;
        for (int j = cards::cardsInMask(board); j < 5; ++j) {
            cards::CardsMask c = cards::draw_random_card(rng, used);
            board |= c;
            used  |= c;
        }

        float r = evaluate_showdown(state.hole_cards, opp, board);
        if      (r == 1.f)  ++wins;
        else if (r == 0.5f) ++ties;
    }

    return (wins + ties * 0.5f) / num_simulations;
}

}; // namespace eval