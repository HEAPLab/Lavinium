#pragma once

#include "llvm/ADT/DenseMap.h"
#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Lavinium.h"
#include "llvm/IR/LaviniumStrategy.h"
#include "llvm/IR/LaviniumTypes.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/LineIterator.h"
#include "llvm/Support/MemoryBuffer.h"
#include <cstddef>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

namespace Lavinium {

template <typename Metric> class StrategyGreedy : public Strategy<Metric> {

  // Contains a vector of const_iterators of strings. The strings are the
  // passes.
  StrategyDeepTracker<std::vector<std::vector<std::string>::const_iterator>>
      SDT;
  size_t MaxDepth = 1;

public:
  StrategyGreedy(const CachedPassesMetric<Metric> *cached)
      : Strategy<Metric>(cached) {
    initialize();
  };

  std::optional<std::vector<std::string>> suggestPasses() override {

    auto &Its = SDT.Data;    // the vector of iterators
    auto &Depth = SDT.Depth; // the depth of the exploration

    this->logFile << "Greedy's depth: " << Depth << std::endl;

    std::vector<std::string> res;
    if (Its.at(Depth) == this->availablePasses.end()) {
      bool canContinue = cascadeAdvanceGreedy();
      if (!canContinue) {
        return std::nullopt;
      }
    }

    auto &It = Its.at(Depth);
    for (size_t i = 0; i <= Depth; i++) {
      res.push_back(Its[i]->c_str());
    }
    It = std::next(It);
    return res;

    return std::nullopt;
  }

  ~StrategyGreedy() override = default;

private:
  void initialize() {
    MaxDepth = LaviniumDepth;
    SDT = {
        std::vector<typename decltype(this->availablePasses)::const_iterator>(
            MaxDepth + 1, this->availablePasses.begin()), // Data
        0};
  }

  // return true if can continue scheduling
  bool cascadeAdvanceGreedy() {
    auto &Its = SDT.Data;    // the vector of iterators
    auto &Depth = SDT.Depth; // the depth of the exploration
    if (Depth == MaxDepth) {
      return false;
    } else {
      auto IDS = findMinimum(Depth)->getIds();
      auto &minimum_last = IDS[IDS.size() - 1];
      Its[Depth] = std::find_if(
          this->availablePasses.begin(), this->availablePasses.end(),
          [&minimum_last](auto &elem) { return elem == minimum_last; });
      Depth++;
      return true;
    }
  }

  // Find the minimum wcet for a certain level of depth
  auto findMinimum(int Depth) {
    typename CachedPassesMetric<Metric>::mapped_type min_data;
    const typename CachedPassesMetric<Metric>::key_type *min_key;
    auto begin = this->cachedPassesMetric->begin();
    auto end = this->cachedPassesMetric->end();

    min_data = begin->second;
    min_key = &(begin->first);

    while (begin != end) {
      auto &[key, data] = *begin;
      // LAVINIUM-TODO: -4 is a magic number due to always scheduling mem2reg
      // loopsimplify simplifycfg at the end
      if (key.size() - 4 != (size_t)Depth) {
        begin++;
        continue;
      }
      if (min_data > data) {
        min_data = std::min(data, min_data);
        min_key = &key;
      }
      begin++;
    }
    return min_key;
  }
};

} // namespace Lavinium