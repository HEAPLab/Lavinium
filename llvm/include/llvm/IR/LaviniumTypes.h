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
using CachedPassesType = std::unordered_map<LaviniumScheduledPasses, Metric>;

template <typename Metric>
using CachedFunctionMetric =
    llvm::DenseMap<llvm::Function *, CachedPassesType<Metric>>;

} // namespace Lavinium