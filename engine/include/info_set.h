#pragma once
#include <vector>

namespace mccfr {

struct InfoSet {
    std::vector<float> regret_sum;
    std::vector<float> strategy_sum;

    explicit InfoSet(int n_actions)
        : regret_sum(n_actions, 0.f),
          strategy_sum(n_actions, 0.f) {}

    int num_actions() const { return regret_sum.size(); }

    void get_strategy(float* out, float reach_prob) const;
};



}
