// Limits.const.h. protacoc 0.6.0, source Limits.const.yaml (sha1 932fbc356699). Do not edit.
#pragma once

#include <cstdint>

namespace demo
{

/// Limits shared by every program of the demo.
/// [const; source Limits.const.yaml]
namespace Limits
{
/// spots on a dispatch board
constexpr int32_t MaxSpots = 4;
/// riders on duty
constexpr int32_t MaxRiders = 8;
}  // namespace Limits

}  // namespace demo
