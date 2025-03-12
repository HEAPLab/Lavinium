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

class StrategyRandom : public Strategy<uint64_t> {

  int Generated;
  std::random_device rd;
  std::mt19937 gen;

public:
  StrategyRandom(const CachedPassesMetric<uint64_t> *cached)
      : Strategy<uint64_t>(cached), Generated(0), gen(rd()) {}


  std::optional<std::vector<std::string>> suggestPasses() override {
    static auto p0 =  std::chrono::system_clock::now();
    const auto p1 = std::chrono::system_clock::now();
    this->logFile    << std::chrono::duration_cast<std::chrono::seconds>(p1 - p0).count() ;
    this->logFile   << "s\t] Random progress " << (float)Generated / (float)RandomSamples << std::endl;
    std::vector<std::string> tmp;
    if (Generated < RandomSamples) {
      std::uniform_int_distribution<> PassDistribution =
          std::uniform_int_distribution<>(
              0, this->availablePasses.size() -
                     1); // used to extract the passes to select
      std::uniform_int_distribution<> SizeDistribution =
          std::uniform_int_distribution<>(
              1, SequenceLength); // used to extract the passes to select
      int RandomSize = SizeDistribution(gen);
      for (int j = 0; j < RandomSize; j++) {
        tmp.push_back(this->availablePasses[PassDistribution(gen)]);
      }
      Generated++;
      return tmp;
    }
    return {};
  }

  ~StrategyRandom() override = default;
};


} // namespace Lavinium
