#include "info_set.h"
#include <algorithm>

namespace mccfr {

void InfoSet::get_strategy(float* out, float reach_prob) const {
    (void)reach_prob; // unused in current implementation
    float norm = 0.f;

    for (int i = 0; i < num_actions(); ++i) {
        out[i] = std::max(regret_sum[i], 0.f);
        norm += out[i];
    }

    for (int i = 0; i < num_actions(); ++i) {
        out[i] = (norm > 0.f ? out[i] / norm : 1.f / num_actions());
    }
}

}
