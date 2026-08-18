// SPDX-License-Identifier: Apache-2.0
// Pafwert — modern C++ port. Original Copyright 2001-2013 Mark Burnett (mb@xato.net)
#pragma once

#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace pafwert {

// Random source used by the whole engine. Backed by std::random_device,
// which draws from the OS CSPRNG on all mainstream platforms.
class Rng {
public:
    // [0, 1)
    double uniform();

    // Port of the VB Rand(Max, Min, Weight) helper: inclusive range with
    // optional weighting. A positive weight > 1 biases toward Max, a negative
    // weight biases toward Min, weight 1 (default) is uniform.
    long range(long max, long min = 0, long weight = 1);

    // Same distribution but formatted with a fixed number of decimal places.
    std::string rangeText(long max, long min, long weight, long decimalPlaces);

    // True with the given percent probability (weight skews the roll).
    bool chance(long percent, long weight = 1);

    // Random character from a set; weight biases toward the front of the set
    // when negative (sets are ordered by letter frequency).
    std::string pickChar(std::string_view chars, long weight = 1);

    // Random item from a delimited list (single-character delimiter, default
    // space). Empty items are legal choices; the result is trimmed.
    std::string pickOne(std::string_view list, long weight = 1, char delim = ' ');

    std::size_t index(std::size_t count);  // uniform 0 .. count-1

private:
    std::random_device dev_;
};

}  // namespace pafwert
