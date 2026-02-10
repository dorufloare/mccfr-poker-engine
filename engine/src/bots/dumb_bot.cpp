#include "bot.h"

// DumbBot: very basic behavior
// - Call if no bet
// - Fold if opponent bets
// - Raise if in position and no bet (optional)
class DumbBot : public Bot {
public:
    action::Action decide(const state::DecisionState& state) noexcept override {
        // always call if nothing to call
        if (state.to_call == 0) {
            return {action::ActionType::CALL, 0};
        }
        // otherwise fold
        return {action::ActionType::FOLD, 0};
    }
};
