#pragma once

#include "llvm/ADT/DenseMap.h"
#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Lavinium.h"
#include "llvm/IR/LaviniumTypes.h"
#include "llvm/IR/LaviniumStrategy.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/LineIterator.h"
#include "llvm/Support/MemoryBuffer.h"
#include "LaviniumStrategyEnum.h"
#include <cstddef>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

namespace Lavinium {

template <typename Metric> class StrategyCartesian : public Strategy<Metric> {

// Contains a vector of const_iterators of strings. The strings are the passes.
StrategyDeepTracker <std::vector<std::vector<std::string>::const_iterator>> SDT;
size_t MaxDepth = 1;

public:

  StrategyCartesian(const CachedPassesMetric<Metric> *cached)
      : Strategy<Metric>(cached) {
    initialize();
  };

  std::optional<std::vector<std::string>>
  suggestPasses() override {
    std::vector<std::string> res;
    bool allDifferent;
    do {
      allDifferent = true;
      res.clear();

      auto &Its = SDT.Data;    // the vector of iterators
      auto &Depth = SDT.Depth; // the depth of the exploration

      if (Its.at(Depth) == this->availablePasses.end()) {
        bool canContinue = cascadeAdvanceCartesian();
        if (!canContinue) {
          return std::nullopt;
        }
      }
      auto &It = Its.at(Depth);
      
      if(anyPassesIsEquals()){
        It = std::next(It);
        allDifferent=false;
        continue;
      }

      for (size_t i = 0; i <= Depth; i++) {
        res.push_back(Its[i]->c_str());
      }
      It = std::next(It);
      
    } while(!allDifferent);

    return res;
  }


  ~StrategyCartesian() override = default;

private:

  // returns true if any of the passes in the SDT is repeated
  bool anyPassesIsEquals(){
    auto &Its = SDT.Data;    // the vector of iterators
    auto &Depth = SDT.Depth; // the depth of the exploration
      for (size_t i = 0; i <= Depth; i++) {
        for (size_t j = i+1; j <= Depth; j++) {
            if(Its.at(i) == Its.at(j)){
              return true;
            }
        }
      }
      return false;
  }


  void initialize() {
    MaxDepth = LaviniumDepth;
    SDT= {  std::vector<typename decltype(this->availablePasses)::const_iterator>(MaxDepth + 1, this->availablePasses.begin()), // Data
            0 };                                                                                                    // Depth
  }

  // return true if can continue scheduling
  bool cascadeAdvanceCartesian() {
    auto end = this->availablePasses.end();
    auto begin = this->availablePasses.begin();

    auto &Its = SDT.Data;    // the vector of iterators
    auto &Depth = SDT.Depth; // the depth of the exploration
    for (int i = Depth; i >= 0; i--) {
      if (Its[i] == end) {
        Its[i] = begin;
      } else {
        Its[i]++;
        if (Its[i] != end) {
          return true;
        } else {
          Its[i] = begin;
        }
      }
    }
    if (Depth == MaxDepth) {
      return false;
    } else {
      Depth++;
      return true;
    }
  }

};

} // namespace Lavinium
