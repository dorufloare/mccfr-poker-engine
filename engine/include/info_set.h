#pragma once
#include <cstdint>
#include <vector>

namespace mccfr {

struct InfoSet {
    std::vector<float> regret_sum;
    std::vector<float> strategy_sum;
    uint64_t visit_count;

    explicit InfoSet(int n_actions)
        : regret_sum(n_actions, 0.f),
          strategy_sum(n_actions, 0.f),
          visit_count(0) {}

    int num_actions() const { return regret_sum.size(); }

    void get_current_strategy(float* out) const;
    void get_average_strategy(float* out) const;
    void accumulate_strategy(const float* strategy, float reach_prob);
};

}
