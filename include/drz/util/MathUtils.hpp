#pragma once

#include <concepts>
#include <cstdlib>
#include <numbers>

namespace drz::math
{
// constants
template <std::floating_point T> inline constexpr T pi = std::numbers::pi_v<T>;
template <std::floating_point T> inline constexpr T tau = T {2} * pi<T>;
template <std::floating_point T> inline constexpr T deg_to_rad = pi<T> / T {180};
template <std::floating_point T> inline constexpr T rad_to_deg = T {180} / pi<T>;

// approximate floating-point compare by absolute tolerance/error
template <std::floating_point T> constexpr T nearly_equal(T a, T b, T err = 1e-6)
{
    T diff = std::abs(a - b);
    return diff <= err;
}

} // namespace drz::math
