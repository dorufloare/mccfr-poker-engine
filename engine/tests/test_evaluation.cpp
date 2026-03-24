// tests/test_evaluation.cpp
// Comprehensive tests for evaluate_showdown() in eval namespace.
//
// Card encoding: bit = suit*13 + rank
//   Suits: 0=Clubs, 1=Diamonds, 2=Hearts, 3=Spades
//   Ranks: 0=Two, 1=Three, 2=Four, 3=Five, 4=Six, 5=Seven, 6=Eight,
//          7=Nine, 8=Ten, 9=Jack, 10=Queen, 11=King, 12=Ace

#include "evaluation.h"
#include "cards.h"
#include <cstdio>
#include <cstdlib>
#include <string_view>

// ── Card building helpers ────────────────────────────────────────────────

// Single card: rank 0-12, suit 0-3
static inline cards::CardsMask C(int rank, int suit) noexcept {
    return 1ULL << (suit * 13 + rank);
}
// Two hole cards
static inline cards::CardsMask H(int r1, int s1, int r2, int s2) noexcept {
    return C(r1, s1) | C(r2, s2);
}
// Board from individual cards (variadic-style via initialiser)
static inline cards::CardsMask B(std::initializer_list<cards::CardsMask> cards) noexcept {
    cards::CardsMask m = 0;
    for (auto c : cards) m |= c;
    return m;
}

// Rank constants
constexpr int R2=0,R3=1,R4=2,R5=3,R6=4,R7=5,R8=6,R9=7,RT=8,RJ=9,RQ=10,RK=11,RA=12;
// Suit constants
constexpr int SC=0, SD=1, SH=2, SS=3;

// ── Test framework ───────────────────────────────────────────────────────

static int g_passed = 0;
static int g_failed = 0;

static void check(float got, float expected, int line, const char* name) {
    if (got == expected) {
        ++g_passed;
    } else {
        ++g_failed;
        const char* exp_str = (expected == 1.f) ? "WIN" : (expected == 0.f) ? "LOSS" : "TIE";
        const char* got_str = (got      == 1.f) ? "WIN" : (got      == 0.f) ? "LOSS" : "TIE";
        std::printf("  FAIL [line %d] %s\n        expected %s, got %s\n", line, name, exp_str, got_str);
    }
}

// result from my perspective (1=win, 0=loss, 0.5=tie)
#define WIN(me, opp, board, name)  check(eval::evaluate_showdown(me, opp, board), 1.f,  __LINE__, name)
#define LOSS(me, opp, board, name) check(eval::evaluate_showdown(me, opp, board), 0.f,  __LINE__, name)
#define TIE(me, opp, board, name)  check(eval::evaluate_showdown(me, opp, board), 0.5f, __LINE__, name)

// ── Test sections ────────────────────────────────────────────────────────

static void test_category_ordering() {
    std::printf("Category ordering...\n");

    // SF > Quads
    // board: J♠Q♠K♠J♣J♦  me: A♠T♠ → royal flush  opp: J♥3♦ → quad jacks
    {
        auto board = B({C(RJ,SS), C(RQ,SS), C(RK,SS), C(RJ,SC), C(RJ,SD)});
        WIN(H(RA,SS,RT,SS), H(RJ,SH,R3,SD), board, "SF beats quads");
    }

    // Quads > Full house
    // me:  A♥A♠ + board A♣A♦K♥2♣3♦  → quad aces (AAAA + K)
    // opp: K♣K♦ + same board          → full house KKK+AA
    {
        auto board = B({C(RA,SC), C(RA,SD), C(RK,SH), C(R2,SC), C(R3,SD)});
        WIN(H(RA,SH,RA,SS), H(RK,SC,RK,SD), board, "Quads beats full house");
    }

    // Full house > Flush
    // board: K♥K♠5♥8♥J♥
    // me:  K♣J♣ → FH KKKJJ
    // opp: A♥3♥ → flush A-high hearts (A♥3♥5♥8♥J♥)
    {
        auto board = B({C(RK,SH), C(RK,SS), C(R5,SH), C(R8,SH), C(RJ,SH)});
        WIN(H(RK,SC,RJ,SC), H(RA,SH,R3,SH), board, "Full house beats flush");
    }

    // Flush > Straight
    // board: A♥4♥8♥T♦K♣
    // me:  2♥3♥ → flush A-high (A♥8♥4♥3♥2♥)
    // opp: J♣Q♣ → broadway straight (T-J-Q-K-A)
    {
        auto board = B({C(RA,SH), C(R4,SH), C(R8,SH), C(RT,SD), C(RK,SC)});
        WIN(H(R2,SH,R3,SH), H(RJ,SC,RQ,SC), board, "Flush beats straight");
    }

    // Straight > Trips
    // board: A♣4♦8♥T♠K♥
    // me:  J♦Q♣ → broadway (T-J-Q-K-A)
    // opp: A♦A♥ → trips AAA +K,T kickers
    {
        auto board = B({C(RA,SC), C(R4,SD), C(R8,SH), C(RT,SS), C(RK,SH)});
        WIN(H(RJ,SD,RQ,SC), H(RA,SD,RA,SH), board, "Straight beats trips");
    }

    // Trips > Two pair
    // board: A♣A♦A♥T♠K♥
    // me:  A♠2♦ → trips AAA + K,T kickers  (wait: 4 aces = quads, use board with 3 aces)
    // actually board A♣A♦A♥ = 3 aces; me A♠ = 4th ace → quads
    // change: use trips on board with pair in hand:
    // board: K♣K♦K♥2♣3♦ (three kings)
    // me:  A♦4♥  → trips KKK + A,4 (no pair = just trips)
    // opp: T♣J♦  → trips KKK + J,T (same trips, lower kickers)
    // ... but this tests "higher kicker in trips" not "trips > two pair"
    // for trips > two pair: use different boards for each player to have different hands
    // board: A♣A♦A♥8♥T♠ (three aces on board)
    // me:  2♦3♥ → trips AAA + T,8 kickers (no pair)
    // opp: T♦8♣ → trips AAA + pair T-8? No: A♣A♦A♥ = trip A, + opp T♦T♠ = pair T, 8♣8♥ = pair 8 → FH AAATT!
    // simpler: me has trips, opp has two pair from different hole cards
    // board: A♣A♦A♥8♥T♠
    // me:  2♦3♥ → trips AAAA? No only 3 aces. trips AAA + T,8 kickers ✓
    // opp: K♣Q♦ → AAAKQ = trips AAA + K,Q kickers → me (2,3 kickers) loses? No: trips AAA same, K>2 kicker → opp wins
    // change: both players have same board trips, compare kickers
    // This would test trips kicker comparison not trips>two pair. 
    // Let me use: board = 5♣5♦5♥2♣3♦ (trips 5s on board, no pair)
    // me: K♣K♦ → FH 555KK ... that's full house
    // me: A♦Q♥ → trips 555 + A,Q kickers ✓  (no pair from A+Q)
    // opp: T♦J♥ → trips 555 + J,T kickers ✓  (no pair)
    // Same trips → kicker comparison, still not trips vs two pair cross-category.
    // REAL approach: give me actual trips with help from hole, opp gets two pair
    // board: K♣K♦2♥5♠7♣
    // me:  K♥ A♦ → trips KKK + A,7 kickers  (KKK from board+hole)
    // opp: A♠Q♦ → pair KK + pair AQ? No: A♠ alone no pair, Q♦ alone no pair. KK from board, best = KK+AQ7 = one pair KK
    // opp: A♠A♦(wait A♦conflict)... use A♠A♥ → tripss KKK + AA would be FH
    // opp: J♦J♥ → KKJJ + kicker from board (A,2,5,7... no A) → two pair (KKJJ + 7 kicker) ✓
    {
        auto board = B({C(RK,SC), C(RK,SD), C(R2,SH), C(R5,SS), C(R7,SC)});
        WIN(H(RK,SH,RA,SD), H(RJ,SD,RJ,SH), board, "Trips beats two pair");
    }

    // Two pair > One pair
    // board: A♣A♦3♥6♠9♣
    // me:  K♣K♦ → two pair AA+KK + 9 kicker
    // opp: Q♣J♦ → one pair AA + Q,J,9 kickers  (no straight possible: A,A,3,6,9,Q,J)
    {
        auto board = B({C(RA,SC), C(RA,SD), C(R3,SH), C(R6,SS), C(R9,SC)});
        WIN(H(RK,SC,RK,SD), H(RQ,SC,RJ,SD), board, "Two pair beats one pair");
    }

    // One pair > High card
    // board: 2♣5♦7♥J♠K♥
    // me:  K♣3♦ → pair KK + J,7,5 kickers
    // opp: A♦Q♣ → high card A,K,Q,J,7
    {
        auto board = B({C(R2,SC), C(R5,SD), C(R7,SH), C(RJ,SS), C(RK,SH)});
        WIN(H(RK,SC,R3,SD), H(RA,SD,RQ,SC), board, "One pair beats high card");
    }
}

static void test_high_card() {
    std::printf("High card...\n");

    // A-high beats K-high
    // board: 2♣5♦7♥9♠J♥  (no pairs, no straights, no flushes)
    {
        auto board = B({C(R2,SC), C(R5,SD), C(R7,SH), C(R9,SS), C(RJ,SH)});
        WIN(H(RA,SC,R3,SD), H(RK,SD,RQ,SC), board, "High card: A-high beats K-high");
    }

    // Same top card, second card decides (J vs T)
    // board: A♣9♦7♥4♠2♦
    // me:  K♣J♣ → A,K,J,9,7  (best 5)
    // opp: K♦T♣ → A,K,T,9,7
    {
        auto board = B({C(RA,SC), C(R9,SD), C(R7,SH), C(R4,SS), C(R2,SD)});
        WIN(H(RK,SC,RJ,SC), H(RK,SD,RT,SC), board, "High card: 3rd kicker decides");
    }

    // 5-card limit: 4th kicker decides, 5th/6th/7th irrelevant
    // board: A♣K♦Q♥J♠2♦
    // me:  9♣3♥ → A,K,Q,J,9 (3♥ is 6th/7th – cut off)
    // opp: 8♣3♦ → A,K,Q,J,8
    {
        auto board = B({C(RA,SC), C(RK,SD), C(RQ,SH), C(RJ,SS), C(R2,SD)});
        WIN(H(R9,SC,R3,SH), H(R8,SC,R3,SD), board, "High card: 5-card limit, 5th kicker decides");
    }

    // Board is better than hole cards → both use board → tie
    // board: A♣K♦Q♥J♠9♣
    {
        auto board = B({C(RA,SC), C(RK,SD), C(RQ,SH), C(RJ,SS), C(R9,SC)});
        TIE(H(R2,SD,R3,SH), H(R4,SD,R5,SH), board, "High card: both use board → tie");
    }

    // Picks 5th kicker from board with better hole card
    // board: A♣K♦Q♥7♠3♦
    // me:  J♣2♦ → A,K,Q,J,7  (J♣ makes it in, 2♦ cut off)
    // opp: T♣2♥ → A,K,Q,T,7
    {
        auto board = B({C(RA,SC), C(RK,SD), C(RQ,SH), C(R7,SS), C(R3,SD)});
        WIN(H(RJ,SC,R2,SD), H(RT,SC,R2,SH), board, "High card: hole card improves 4th kicker");
    }
}

static void test_one_pair() {
    std::printf("One pair...\n");

    // Higher pair wins
    // board: 2♣5♦7♥9♠J♥
    // me:  K♣K♦ → pair KK
    // opp: Q♣Q♦ → pair QQ
    {
        auto board = B({C(R2,SC), C(R5,SD), C(R7,SH), C(R9,SS), C(RJ,SH)});
        WIN(H(RK,SC,RK,SD), H(RQ,SC,RQ,SD), board, "One pair: higher pair wins");
    }

    // Same pair, first kicker decides
    // board: K♣2♦5♥7♠9♣
    // me:  K♦A♣ → KK + A,9,7 kickers
    // opp: K♥Q♣ → KK + Q,9,7 kickers
    {
        auto board = B({C(RK,SC), C(R2,SD), C(R5,SH), C(R7,SS), C(R9,SC)});
        WIN(H(RK,SD,RA,SC), H(RK,SH,RQ,SC), board, "One pair: 1st kicker decides");
    }

    // Same pair, same 1st kicker, 2nd kicker decides
    // board: K♣A♦2♥5♠7♣
    // me:  K♦Q♣ → KK + A,Q,7 kickers
    // opp: K♥J♣ → KK + A,J,7 kickers
    {
        auto board = B({C(RK,SC), C(RA,SD), C(R2,SH), C(R5,SS), C(R7,SC)});
        WIN(H(RK,SD,RQ,SC), H(RK,SH,RJ,SC), board, "One pair: 2nd kicker decides");
    }

    // Same pair, same 3 kickers → tie
    // board: K♣A♦Q♥J♠2♣
    // me:  K♦3♦ → KK + A,Q,J (3♦ cut off as 6th)
    // opp: K♥4♥ → KK + A,Q,J (4♥ cut off)
    {
        auto board = B({C(RK,SC), C(RA,SD), C(RQ,SH), C(RJ,SS), C(R2,SC)});
        TIE(H(RK,SD,R3,SD), H(RK,SH,R4,SH), board, "One pair: tied kickers → tie");
    }

    // Pair on board; hole card kicker decides
    // board: K♣K♦2♥5♠7♣
    // me:  A♦3♥ → pair KK + A,7,5 kickers
    // opp: Q♣3♦ → pair KK + Q,7,5 kickers
    {
        auto board = B({C(RK,SC), C(RK,SD), C(R2,SH), C(R5,SS), C(R7,SC)});
        WIN(H(RA,SD,R3,SH), H(RQ,SC,R3,SD), board, "One pair: board pair, kicker from hole");
    }
}

static void test_two_pair() {
    std::printf("Two pair...\n");

    // Higher top pair wins
    // board: A♣5♦2♥8♠T♣
    // me:  A♦5♣ → AA+55 + T kicker
    // opp: T♦8♣ → TT+88 + A kicker
    {
        auto board = B({C(RA,SC), C(R5,SD), C(R2,SH), C(R8,SS), C(RT,SC)});
        WIN(H(RA,SD,R5,SC), H(RT,SD,R8,SC), board, "Two pair: higher top pair wins");
    }

    // Same top pair, second pair decides
    // board: A♣A♦2♥5♠7♣
    // me:  K♦K♥ → AA+KK + 7 kicker
    // opp: Q♦Q♥ → AA+QQ + 7 kicker
    {
        auto board = B({C(RA,SC), C(RA,SD), C(R2,SH), C(R5,SS), C(R7,SC)});
        WIN(H(RK,SD,RK,SH), H(RQ,SD,RQ,SH), board, "Two pair: 2nd pair decides");
    }

    // Same two pairs, kicker decides
    // board: A♣A♦K♥K♠2♣
    // me:  Q♦3♥ → AAKK + Q kicker
    // opp: J♦3♦ → AAKK + J kicker
    {
        auto board = B({C(RA,SC), C(RA,SD), C(RK,SH), C(RK,SS), C(R2,SC)});
        WIN(H(RQ,SD,R3,SH), H(RJ,SD,R3,SD), board, "Two pair: kicker decides");
    }

    // Same two pairs, same kicker → tie
    // board: A♣A♦K♥K♠Q♣
    // me:  2♦3♥  → best 5 = AAKK+Q (hole cards irrelevant)
    // opp: 4♦5♥  → best 5 = AAKK+Q
    {
        auto board = B({C(RA,SC), C(RA,SD), C(RK,SH), C(RK,SS), C(RQ,SC)});
        TIE(H(R2,SD,R3,SH), H(R4,SD,R5,SH), board, "Two pair: tied → tie");
    }

    // 3 pairs available: picks best two + best kicker
    // board: A♣A♦K♥Q♠2♣
    // me:  K♦Q♥ → 3 pairs: AA, KK, QQ → best = AA+KK + Q kicker
    // opp: Q♦J♦ → 2 pairs: AA, QQ → AA+QQ + K kicker
    {
        auto board = B({C(RA,SC), C(RA,SD), C(RK,SH), C(RQ,SS), C(R2,SC)});
        WIN(H(RK,SD,RQ,SH), H(RQ,SD,RJ,SD), board, "Two pair: 3 pairs available, picks best 2");
    }
}

static void test_three_of_a_kind() {
    std::printf("Three of a kind...\n");

    // Higher trips wins
    // board: K♣K♦K♥2♣3♦
    // me:  A♦4♥  → trips KKK + A,4 kickers
    // opp: Q♣J♦  → trips KKK + Q,J kickers
    {
        auto board = B({C(RK,SC), C(RK,SD), C(RK,SH), C(R2,SC), C(R3,SD)});
        WIN(H(RA,SD,R4,SH), H(RQ,SC,RJ,SD), board, "Trips: higher kicker wins");
    }

    // Same trips (board), 2nd kicker decides  
    // board: A♣A♦A♥2♣3♦
    // me:  K♦Q♥ → AAA + K,Q kickers
    // opp: K♥J♦ → AAA + K,J kickers
    {
        auto board = B({C(RA,SC), C(RA,SD), C(RA,SH), C(R2,SC), C(R3,SD)});
        WIN(H(RK,SD,RQ,SH), H(RK,SH,RJ,SD), board, "Trips: 2nd kicker decides");
    }

    // Board trips, same kicker1, 2nd kicker decides
    // board: A♣A♦A♥2♣3♦
    // me:  K♦Q♥ → AAAKQ
    // opp: K♥J♦ → AAAKJ
    // (same as above, already tested – replace with tied scenario)
    // Tied trips (same board, hole cards beat by board) → tie
    // board: A♣A♦A♥K♣Q♦
    // me:  2♦3♥  → best 5 = AAAKQ (hole cards cut off)
    // opp: 4♦5♥  → best 5 = AAAKQ
    {
        auto board = B({C(RA,SC), C(RA,SD), C(RA,SH), C(RK,SC), C(RQ,SD)});
        TIE(H(R2,SD,R3,SH), H(R4,SD,R5,SH), board, "Trips: both use board → tie");
    }

    // Trips beats two pair (cross-category, already in ordering; add specific concrete case)
    // board: A♣A♦2♥5♠7♣
    // me:  A♥3♦  → trips AAA + 7,5 kickers
    // opp: K♣K♦  → two pair AAKK + 7 kicker
    {
        auto board = B({C(RA,SC), C(RA,SD), C(R2,SH), C(R5,SS), C(R7,SC)});
        WIN(H(RA,SH,R3,SD), H(RK,SC,RK,SD), board, "Trips beats two pair");
    }
}

static void test_straight() {
    std::printf("Straight...\n");

    // Broadway (A-high) beats K-high straight
    // board: T♣J♦Q♥2♠3♦
    // me:  K♣A♦ → T-J-Q-K-A
    // opp: K♦9♣ → 9-T-J-Q-K
    {
        auto board = B({C(RT,SC), C(RJ,SD), C(RQ,SH), C(R2,SS), C(R3,SD)});
        WIN(H(RK,SC,RA,SD), H(RK,SD,R9,SC), board, "Straight: broadway beats K-high");
    }

    // 8-high beats 7-high
    // board: 4♣5♦6♥2♠A♦
    // me:  7♣8♦ → 4-5-6-7-8
    // opp: 7♦3♣ → 3-4-5-6-7
    {
        auto board = B({C(R4,SC), C(R5,SD), C(R6,SH), C(R2,SS), C(RA,SD)});
        WIN(H(R7,SC,R8,SD), H(R7,SD,R3,SC), board, "Straight: 8-high beats 7-high");
    }

    // Wheel A-2-3-4-5 is a straight
    // board: 2♣3♦4♥8♠K♦
    // me:  A♣5♦ → A-2-3-4-5 = 5-high straight
    // opp: T♦J♣ → no straight (2,3,4,8,T,J,K has no 5-consecutive)
    {
        auto board = B({C(R2,SC), C(R3,SD), C(R4,SH), C(R8,SS), C(RK,SD)});
        WIN(H(RA,SC,R5,SD), H(RT,SD,RJ,SC), board, "Straight: wheel beats no straight");
    }

    // Wheel (5-high) loses to 6-high straight (2-3-4-5-6)
    // board: 2♣3♦4♥5♠A♦
    // me:  A♣6♣  → both wheel (A-2-3-4-5) and 2-3-4-5-6 present; best = 6-high
    // opp: 7♦K♥  → wheel (A-2-3-4-5) only (no 6 for opp)
    {
        auto board = B({C(R2,SC), C(R3,SD), C(R4,SH), C(R5,SS), C(RA,SD)});
        WIN(H(RA,SC,R6,SC), H(R7,SD,RK,SH), board, "Straight: 6-high beats wheel");
    }

    // Identical top-rank straight → tie (no kickers in straights)
    // board: T♣J♦Q♥K♠A♣  (broadway on board)
    // me:  2♦3♥  → both use board broadway
    // opp: 4♦5♥
    {
        auto board = B({C(RT,SC), C(RJ,SD), C(RQ,SH), C(RK,SS), C(RA,SC)});
        TIE(H(R2,SD,R3,SH), H(R4,SD,R5,SH), board, "Straight: same top rank → tie");
    }

    // 7 cards: picks best straight (9-high beats 8-high from same 7 cards)
    // board: 5♣6♦7♥8♠2♦
    // me:  4♣9♦ → has 4-5-6-7-8 (8-high) AND 5-6-7-8-9 (9-high) → best = 9-high
    // opp: 4♦3♣ → has 3-4-5-6-7 (7-high) AND 4-5-6-7-8 (8-high) → best = 8-high
    {
        auto board = B({C(R5,SC), C(R6,SD), C(R7,SH), C(R8,SS), C(R2,SD)});
        WIN(H(R4,SC,R9,SD), H(R4,SD,R3,SC), board, "Straight: picks best of multiple straights");
    }

    // Straight beats trips
    // board: 9♣T♦J♥A♠A♦
    // me:  Q♦K♣ → 9-T-J-Q-K straight
    // opp: A♣2♦ → trips AAA + J,T kickers
    {
        auto board = B({C(R9,SC), C(RT,SD), C(RJ,SH), C(RA,SS), C(RA,SD)});
        WIN(H(RQ,SD,RK,SC), H(RA,SC,R2,SD), board, "Straight beats trips");
    }
}

static void test_flush() {
    std::printf("Flush...\n");

    // A-high flush beats K-high flush (same suit, board has 3 hearts)
    // board: 2♥5♥8♥9♥Q♦
    // me:  A♥3♦ → A♥9♥8♥5♥2♥ = A-high flush
    // opp: K♥3♣ → K♥9♥8♥5♥2♥ = K-high flush
    {
        auto board = B({C(R2,SH), C(R5,SH), C(R8,SH), C(R9,SH), C(RQ,SD)});
        WIN(H(RA,SH,R3,SD), H(RK,SH,R3,SC), board, "Flush: A-high beats K-high");
    }

    // Same top card, 2nd card decides
    // board: A♥5♥8♥9♥Q♦
    // me:  K♥2♦ → A♥K♥9♥8♥5♥
    // opp: J♥2♣ → A♥J♥9♥8♥5♥
    {
        auto board = B({C(RA,SH), C(R5,SH), C(R8,SH), C(R9,SH), C(RQ,SD)});
        WIN(H(RK,SH,R2,SD), H(RJ,SH,R2,SC), board, "Flush: 2nd card decides");
    }

    // Board has 4 flush cards, player with suited hole wins
    // board: 2♥5♥8♥J♥Q♦
    // me:  A♥3♦ → 5 hearts flush (A♥J♥8♥5♥2♥)
    // opp: K♣T♣ → only 4 hearts, no flush → best = pair? no pairs → high card
    {
        auto board = B({C(R2,SH), C(R5,SH), C(R8,SH), C(RJ,SH), C(RQ,SD)});
        WIN(H(RA,SH,R3,SD), H(RK,SC,RT,SC), board, "Flush: suited hole vs. no flush");
    }

    // 7 flush cards: picks best 5
    // board: 2♥4♥6♥8♥T♥   (5 hearts on board)
    // me:  A♥K♥ → 7 hearts; best 5 = A♥K♥T♥8♥6♥
    // opp: A♦K♦ → only board hearts → best 5 for board = T♥8♥6♥4♥2♥
    {
        auto board = B({C(R2,SH), C(R4,SH), C(R6,SH), C(R8,SH), C(RT,SH)});
        WIN(H(RA,SH,RK,SH), H(RA,SD,RK,SD), board, "Flush: 7 flush cards, picks best 5");
    }

    // Same 5-card flush (board) → tie
    // board: A♥K♥Q♥J♥9♥
    {
        auto board = B({C(RA,SH), C(RK,SH), C(RQ,SH), C(RJ,SH), C(R9,SH)});
        TIE(H(R2,SD,R3,SC), H(R4,SD,R5,SC), board, "Flush: identical flush → tie");
    }
}

static void test_full_house() {
    std::printf("Full house...\n");

    // Higher trips in FH wins (AAAKK vs KKKAA)
    // board: A♣A♦K♣K♦2♣
    // me:  A♥3♦ → trips AAA + pair KK = FH AAAKK
    // opp: K♥3♥ → trips KKK + pair AA = FH KKKAA
    {
        auto board = B({C(RA,SC), C(RA,SD), C(RK,SC), C(RK,SD), C(R2,SC)});
        WIN(H(RA,SH,R3,SD), H(RK,SH,R3,SH), board, "FH: higher trips wins (AAAKK > KKKAA)");
    }

    // Same trips, better pair wins
    // board: A♣A♦A♥K♣Q♦
    // me:  K♦2♣ → FH AAA+KK
    // opp: Q♣3♦ → FH AAA+QQ
    {
        auto board = B({C(RA,SC), C(RA,SD), C(RA,SH), C(RK,SC), C(RQ,SD)});
        WIN(H(RK,SD,R2,SC), H(RQ,SC,R3,SD), board, "FH: same trips, better pair wins");
    }

    // Double-trips: takes best trip + second trip as pair
    // board: A♣A♦A♥K♣K♦ (3 aces + 2 kings on board)
    // me:  K♥2♣ → 3 aces + 3 kings → FH AAA+KK (uses spare K as pair partner)
    // opp: K♠2♦
    // both same FH AAAKK → tie
    {
        auto board = B({C(RA,SC), C(RA,SD), C(RA,SH), C(RK,SC), C(RK,SD)});
        TIE(H(RK,SH,R2,SC), H(RK,SS,R2,SD), board, "FH: double-trips on board → tie");
    }

    // FH beats flush (already tested in ordering; concrete case here)
    // board: K♥K♠5♥8♥J♥
    // me:  K♣J♣ → trips KKK + pair JJ = FH KKKJJ
    // opp: A♥3♥ → flush A♥J♥8♥5♥3♥
    {
        auto board = B({C(RK,SH), C(RK,SS), C(R5,SH), C(R8,SH), C(RJ,SH)});
        WIN(H(RK,SC,RJ,SC), H(RA,SH,R3,SH), board, "FH beats flush");
    }

    // Identical FH (both use board) → tie
    // board: A♣A♦A♥K♣K♦
    {
        auto board = B({C(RA,SC), C(RA,SD), C(RA,SH), C(RK,SC), C(RK,SD)});
        TIE(H(R2,SD,R3,SH), H(R4,SD,R5,SH), board, "FH: identical → tie");
    }
}

static void test_four_of_a_kind() {
    std::printf("Four of a kind...\n");

    // Higher quads win
    // board: A♣A♦K♣K♦2♣
    // me:  A♥A♠ → quad aces + K kicker
    // opp: K♥K♠ → quad kings + A kicker
    {
        auto board = B({C(RA,SC), C(RA,SD), C(RK,SC), C(RK,SD), C(R2,SC)});
        WIN(H(RA,SH,RA,SS), H(RK,SH,RK,SS), board, "Quads: higher quad wins");
    }

    // Same quads (board), kicker decides
    // board: K♣K♦K♥K♠2♣
    // me:  A♦3♥ → KKKK + A kicker
    // opp: Q♦4♥ → KKKK + Q kicker
    {
        auto board = B({C(RK,SC), C(RK,SD), C(RK,SH), C(RK,SS), C(R2,SC)});
        WIN(H(RA,SD,R3,SH), H(RQ,SD,R4,SH), board, "Quads: board quads, kicker decides");
    }

    // Quads beat full house
    // board: A♣A♦A♥K♣2♦
    // me:  A♠3♥ → quad AAAA + K kicker
    // opp: K♦K♥ → trips KKK + pair AAA on board → FH KKKAA
    {
        auto board = B({C(RA,SC), C(RA,SD), C(RA,SH), C(RK,SC), C(R2,SD)});
        WIN(H(RA,SS,R3,SH), H(RK,SD,RK,SH), board, "Quads beats FH");
    }

    // Same quads, same kicker → tie
    // board: K♣K♦K♥K♠A♣
    {
        auto board = B({C(RK,SC), C(RK,SD), C(RK,SH), C(RK,SS), C(RA,SC)});
        TIE(H(R2,SD,R3,SH), H(R4,SD,R5,SH), board, "Quads: identical → tie");
    }
}

static void test_straight_flush() {
    std::printf("Straight flush...\n");

    // Higher SF wins (K-high vs J-high, same suit board)
    // board: 9♦T♦J♦2♣3♥
    // me:  Q♦K♦ → 9-T-J-Q-K diamonds = K-high SF
    // opp: 7♦8♦ → 7-8-9-T-J diamonds = J-high SF
    {
        auto board = B({C(R9,SD), C(RT,SD), C(RJ,SD), C(R2,SC), C(R3,SH)});
        WIN(H(RQ,SD,RK,SD), H(R7,SD,R8,SD), board, "SF: K-high beats J-high");
    }

    // Royal flush beats lower SF using shared suit board
    // board: T♦J♦Q♦2♣3♥
    // me:  K♦A♦ → T-J-Q-K-A diamonds = royal (A-high SF)
    // opp: 8♦9♦ → 8-9-T-J-Q diamonds = Q-high SF
    {
        auto board = B({C(RT,SD), C(RJ,SD), C(RQ,SD), C(R2,SC), C(R3,SH)});
        WIN(H(RK,SD,RA,SD), H(R8,SD,R9,SD), board, "SF: royal beats Q-high SF");
    }

    // SF beats quads
    // board: J♠Q♠K♠J♣J♦
    // me:  A♠T♠ → royal flush (T♠J♠Q♠K♠A♠)
    // opp: J♥3♦ → quad jacks (J♣J♦J♠J♥) + K kicker
    {
        auto board = B({C(RJ,SS), C(RQ,SS), C(RK,SS), C(RJ,SC), C(RJ,SD)});
        WIN(H(RA,SS,RT,SS), H(RJ,SH,R3,SD), board, "SF beats quads");
    }

    // Wheel SF (A-2-3-4-5 same suit) is a valid straight flush
    // board: 2♣3♣4♣5♣8♦
    // me:  A♣6♦ → A♣2♣3♣4♣5♣ = wheel SF
    // opp: T♦J♦ → no SF (only 2 diamonds), no straight → high card
    {
        auto board = B({C(R2,SC), C(R3,SC), C(R4,SC), C(R5,SC), C(R8,SD)});
        WIN(H(RA,SC,R6,SD), H(RT,SD,RJ,SD), board, "SF: wheel SF beats high card");
    }

    // Wheel SF beats regular straight
    // board: 2♣3♣4♣5♣A♦
    // me:  A♣6♦ → wheel SF (A♣2♣3♣4♣5♣); 6♦ makes a non-SF 6-high straight, but SF wins
    // opp: 7♦6♥ → 3-4-5-6-7 = 7-high regular straight
    {
        auto board = B({C(R2,SC), C(R3,SC), C(R4,SC), C(R5,SC), C(RA,SD)});
        WIN(H(RA,SC,R6,SD), H(R7,SD,R6,SH), board, "SF: wheel SF beats regular straight");
    }

    // Wheel SF loses to 6-high SF
    // board: 2♥3♥4♥5♥A♣
    // me:  A♥6♦ → A♥2♥3♥4♥5♥ = wheel SF (rank top = 3 = FIVE)
    // opp: 6♥7♦ → 2♥3♥4♥5♥6♥ = 6-high SF (rank top = 4 = SIX)
    {
        auto board = B({C(R2,SH), C(R3,SH), C(R4,SH), C(R5,SH), C(RA,SC)});
        LOSS(H(RA,SH,R6,SD), H(R6,SH,R7,SD), board, "SF: wheel SF loses to 6-high SF");
    }
}

static void test_cross_category_misc() {
    std::printf("Cross-category misc...\n");

    // Verify symmetry: if A beats B then B loses to A
    // (using existing scenario: broadway vs K-high straight)
    {
        auto board = B({C(RT,SC), C(RJ,SD), C(RQ,SH), C(R2,SS), C(R3,SD)});
        auto me  = H(RK,SC,RA,SD);
        auto opp = H(RK,SD,R9,SC);
        WIN(me, opp, board,  "Symmetry check: winner wins");
        LOSS(opp, me, board, "Symmetry check: loser loses");
    }

    // Five-card rule: trips + 2 kickers, NOT 3 kickers
    // If trips + best2 kickers tie, result is tie even if 3rd kicker differs
    // board: A♣A♦A♥K♣Q♦
    // me:  J♦2♥ → AAAKQ (J and 2 are the 6th/7th cards, cut off)
    // opp: J♥3♦ → AAAKQ (J and 3 are the 6th/7th cards, cut off)
    {
        auto board = B({C(RA,SC), C(RA,SD), C(RA,SH), C(RK,SC), C(RQ,SD)});
        TIE(H(RJ,SD,R2,SH), H(RJ,SH,R3,SD), board, "5-card rule: trips, 3rd kicker ignored");
    }

    // Five-card rule: quads + kicker, only 1 kicker counted
    // board: K♣K♦K♥K♠Q♣
    // me:  A♦2♥ → KKKK + A (2♥ is 7th card, cut off)
    // opp: A♥3♦ → KKKK + A (3♦ is 7th card, cut off)
    // → tie, not "me because different 6th card"
    {
        auto board = B({C(RK,SC), C(RK,SD), C(RK,SH), C(RK,SS), C(RQ,SC)});
        TIE(H(RA,SD,R2,SH), H(RA,SH,R3,SD), board, "5-card rule: quads, 2nd kicker ignored");
    }
}

// ── Main ─────────────────────────────────────────────────────────────────

int main() {
    std::printf("=== evaluate_showdown tests ===\n\n");

    test_category_ordering();
    test_high_card();
    test_one_pair();
    test_two_pair();
    test_three_of_a_kind();
    test_straight();
    test_flush();
    test_full_house();
    test_four_of_a_kind();
    test_straight_flush();
    test_cross_category_misc();

    std::printf("\n");
    int total = g_passed + g_failed;
    if (g_failed == 0)
        std::printf("ALL %d TESTS PASSED\n", total);
    else
        std::printf("%d/%d PASSED  |  %d FAILED\n", g_passed, total, g_failed);

    return g_failed == 0 ? 0 : 1;
}
