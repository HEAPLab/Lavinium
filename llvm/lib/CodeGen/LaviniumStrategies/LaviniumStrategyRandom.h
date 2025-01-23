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
#include <random>

namespace Lavinium {

// LAVINIUM-TODO finish
class WeightForPass {
private:
  double weight = 1;
  double N_positive = 0;
  double N_neutral = 0;

public:
  void recordPositive() {
    N_positive++;
  }

  void recordNeutral() {
    N_neutral++;
  }

  void updateWeight() {
    
    double denominator = (N_neutral < 1) ? 1 : N_neutral; 
    weight = N_positive / denominator * 0.85 + 0.15;
  }

  double getWeight() {
    return weight;
  }

  void setWeight(double w) {
    weight = w;
  }
};

template <typename Metric> class StrategyRandom : public Strategy<Metric> {

// Contains a vector of const_iterators of strings. The strings are the passes.
StrategyDeepTracker <std::vector<std::vector<std::string>::const_iterator>> SDT;
size_t MaxDepth = 1;
size_t TargetSamples = 1;
size_t ActualSamples = 0;

CachedPassesMetric<WeightForPass> weightsMap;

std::random_device rd;
std::mt19937 gen; 

public:

  StrategyRandom(const CachedPassesMetric<Metric> *cached)
      : Strategy<Metric>(cached), gen(rd()) {
    initialize();
  };

  std::optional<std::vector<std::string>>
  suggestPasses() override {
    if (TargetSamples < ActualSamples) {
      return std::nullopt;
    }

    std::vector<std::string> res;

    std::uniform_int_distribution<> SampleDistribution(1, MaxDepth); // used to extract the lenght of the current sample
    std::uniform_int_distribution<> PassDistribtion(0, this->availablePasses.size()-1); // used to extract the passes to select

    short length = SampleDistribution(gen);
    for (int i=0; i<length; i++) {
      res.push_back(this->availablePasses[PassDistribtion(gen)]);
    }
    ActualSamples++;
    return res;
  }

  ~StrategyRandom() override = default;

private:
  void initialize() {
    MaxDepth = LaviniumDepth;
    TargetSamples = LaviniumPopulationSize;

    SDT= {  std::vector<typename decltype(this->availablePasses)::const_iterator>(MaxDepth + 1, this->availablePasses.begin()), // Data
            0 };                                                                                                    // Depth
  }

  void NormalizeWeights() {
    double tot = 0;
    for (auto p : weightsMap) {
      tot+=p.second.getWeight();
    }

    for (auto p : weightsMap) {
      p.second.setWeight(p.second.getWeight() / tot);
    }
  }
};

} // namespace Lavinium
