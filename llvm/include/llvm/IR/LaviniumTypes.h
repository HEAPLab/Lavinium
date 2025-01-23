#pragma once

#include "LaviniumScheduledPass.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Function.h"
#include <unordered_map>

// clang-format off
// ╦  ┌─┐┬  ┬┬┌┐┌┬┬ ┬┌┬┐╔╦╗┬ ┬┌─┐┌─┐┌─┐
// ║  ├─┤└┐┌┘││││││ ││││ ║ └┬┘├─┘├┤ └─┐
// ╩═╝┴ ┴ └┘ ┴┘└┘┴└─┘┴ ┴ ╩  ┴ ┴  └─┘└─┘
// clang-format on

namespace Lavinium {

template <typename Metric>
using CachedPassesMetric = std::unordered_map<LaviniumScheduledPasses, Metric>;

} // namespace Lavinium