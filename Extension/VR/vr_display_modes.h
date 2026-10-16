#pragma once
#include <array>
#include <algorithm>
#include <utility>
#include <vector>
namespace dingosdk::vr::display {
inline constexpr std::array<unsigned, 10> square_sizes{1440, 1600, 1800, 2016, 2304, 2592, 2880, 3240, 3600, 3840};
// Square, forced-windowed VR launches use this same catalog on every monitor.
// Resolution is saved as an index by the game, so mixing physical monitor modes
// into it made setup select different sizes when the output/driver list changed.
inline std::vector<std::pair<unsigned, unsigned>> windowed_catalog(unsigned requested) {
    std::vector<std::pair<unsigned, unsigned>> result;
    for (const auto size : square_sizes) result.emplace_back(size, size);
    if (requested && std::find(square_sizes.begin(), square_sizes.end(), requested) == square_sizes.end())
        result.emplace_back(requested, requested);
    std::sort(result.begin(), result.end());
    return result;
}
}
