// SPDX-License-Identifier: Apache-2.0
#include "random.hpp"

#include <cmath>
#include <cstdlib>

#include "text.hpp"

namespace pafwert {

double Rng::uniform() {
    return std::uniform_real_distribution<double>(0.0, 1.0)(dev_);
}

long Rng::range(long max, long min, long weight) {
    if (max == 0) max = 9;
    if (weight == 0) weight = 1;

    double ceiling = static_cast<double>(max);
    for (long i = 0; i < std::labs(weight); ++i)
        ceiling = uniform() * (ceiling - min) + min;
    if (weight > 0) ceiling = max - (ceiling - min);
    return std::lround(ceiling);
}

std::string Rng::rangeText(long max, long min, long weight, long decimalPlaces) {
    if (decimalPlaces <= 0) return std::to_string(range(max, min, weight));

    if (max == 0) max = 9;
    if (weight == 0) weight = 1;
    double ceiling = static_cast<double>(max);
    for (long i = 0; i < std::labs(weight); ++i)
        ceiling = uniform() * (ceiling - min) + min;
    if (weight > 0) ceiling = max - (ceiling - min);

    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.*f", static_cast<int>(decimalPlaces), ceiling);
    return buf;
}

bool Rng::chance(long percent, long weight) {
    return range(100, 1, weight) <= percent;
}

std::string Rng::pickChar(std::string_view chars, long weight) {
    if (chars.empty()) return {};
    long i = range(static_cast<long>(chars.size()), 1, weight);
    if (i < 1) i = 1;
    if (i > static_cast<long>(chars.size())) i = static_cast<long>(chars.size());
    return std::string(1, chars[static_cast<std::size_t>(i - 1)]);
}

std::string Rng::pickOne(std::string_view list, long weight, char delim) {
    std::vector<std::string> items = split(list, delim);
    if (items.empty()) return {};
    std::string picked;
    if (items.size() > 1) {
        long i = range(static_cast<long>(items.size()) - 1, 0, weight);
        if (i < 0) i = 0;
        if (i >= static_cast<long>(items.size())) i = static_cast<long>(items.size()) - 1;
        picked = items[static_cast<std::size_t>(i)];
    } else {
        picked = items.front();
    }
    return trim(picked);
}

std::size_t Rng::index(std::size_t count) {
    if (count == 0) return 0;
    return std::uniform_int_distribution<std::size_t>(0, count - 1)(dev_);
}

}  // namespace pafwert
