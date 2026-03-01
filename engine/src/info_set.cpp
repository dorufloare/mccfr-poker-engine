#include "info_set.h"
#include <algorithm>

namespace mccfr {

void InfoSet::get_current_strategy(float* out) const {
    float norm = 0.f;

    for (int i = 0; i < num_actions(); ++i) {
        out[i] = std::max(regret_sum[i], 0.f);
        norm += out[i];
    }

    for (int i = 0; i < num_actions(); ++i) {
        out[i] = (norm > 0.f ? out[i] / norm : 1.f / num_actions());
    }
}

void InfoSet::get_average_strategy(float* out) const {
    float norm = 0.f;
    for (int i = 0; i < num_actions(); ++i) {
        norm += strategy_sum[i];
    }
    for (int i = 0; i < num_actions(); ++i) {
        out[i] = (norm > 0.f ? strategy_sum[i] / norm : 1.f / num_actions());
    }
}

void InfoSet::accumulate_strategy(const float* strategy, float reach_prob) {
    for (int i = 0; i < num_actions(); ++i) {
        strategy_sum[i] += reach_prob * strategy[i];
    }
}

}
