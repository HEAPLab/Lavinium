#pragma once

#include "llvm/IR/Lavinium.h"
#include "llvm/IR/LaviniumStrategy.h"
#include "llvm/IR/LaviniumTypes.h"
#include "llvm/LLVMTA/LLVMPasses/TimeHelper.h"
#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <vector>
#include <chrono>

namespace Lavinium {

class StrategyNone : public Strategy<uint64_t> {

  bool returned = false;

public:
  StrategyNone(const CachedPassesMetric<uint64_t> *cached)
      : Strategy<uint64_t>(cached) {}


  std::optional<std::vector<std::string>> suggestPasses() override {
    /* if (returned) return {};
    returned = true;
    return std::vector<std::string>({"loop(indvars)","dce"}); */
    return {};
  }

  ~StrategyNone() override = default;
};


} // namespace Lavinium
