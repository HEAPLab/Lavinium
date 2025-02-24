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

namespace {
using csiter = std::vector<std::string>::const_iterator;
using Sequence = std::vector<csiter>;

} // namespace

template <typename Metric> class StrategyCartesianPruned : public Strategy<Metric> {

// Contains a vector of const_iterators of strings. The strings are the passes.
StrategyDeepTracker <std::vector<Sequence>> SDT;

// Vector of set of sequences of passes. 
// Each each element of the vector is the set of sequences obtained with the exploration at the depth corresponding to the index.
std::vector<std::vector<Sequence>> SOPD;// Sequences Of Passes per Depth 
size_t currDepth = 1;
size_t singleIdx = 0;
size_t currIdx = 0;

Sequence currSequence;
Sequence prevSequence;

size_t MaxDepth = 1;

bool cartesianExplored = false;

public:

  StrategyCartesianPruned(const CachedPassesMetric<Metric> *cached)
      : Strategy<Metric>(cached) {
    initialize();
  };

  std::optional<std::vector<std::string>>
  suggestPasses() override { 

    // verify whether the current sequence improved the WCET (wrt the previous) and eventually add it to the SDT
    verifyImprovement();

    std::vector<std::string> res;

    if (cartesianExplored) {
      // greedy TODO
      return std::nullopt;
    }
    else { // explore the cartesian space
      Sequence seq;
      
      // take the single pass
      auto singlePass = this->availablePasses.cbegin() + singleIdx;

      // take the passes at currDepth-1
      seq = SOPD.size() > 0 ? Sequence(SOPD.at(currDepth-1).at(currIdx)) : Sequence();
      prevSequence = Sequence(seq);

      const Sequence &p = prevSequence;
      LaviniumScheduledPasses prev{p};
      // check whether we have already explored prev, if yes, add the new pass to the sequence
      if (this->cachedPassesMetric->find(prev) != this->cachedPassesMetric->end()) {
        // concatenate the two
        seq.push_back(singlePass);

        cartesianExplored = cascadeAdvanceCartesian();
      }

      currSequence = Sequence(seq);

      for (auto elem : seq) {
        res.push_back(*elem.base());
      }
    }
    return res;
  }


  ~StrategyCartesianPruned() override = default;

private:

  void initialize() {
    MaxDepth = LaviniumDepth;

    for (int i=0; i<=MaxDepth; i++) {
      SOPD.push_back(std::vector<Sequence>());
    }
    for (csiter Item = this->availablePasses.cbegin(); Item != this->availablePasses.cend(); ++Item) {
      SDT.Data.push_back(Sequence{Item}); // push the vector in the SOPD at depth 0
      SOPD.at(0).push_back(Sequence{Item});
    } 
  }

  void verifyImprovement() {
    if (currSequence.size() == 0 || prevSequence.size() == 0) return;
    const Sequence &c = currSequence;
    const Sequence &p = prevSequence;
    LaviniumScheduledPasses curr{c};
    LaviniumScheduledPasses prev{p};

    auto begin = this->cachedPassesMetric->begin();
    auto end = this->cachedPassesMetric->end();
    while (begin != end) {
      std::cout << begin->second << "\n";
      begin++;
    }

    if(this->cachedPassesMetric->find(prev) == this->cachedPassesMetric->end() || this->cachedPassesMetric->find(curr)->second >
      this->cachedPassesMetric->find(prev)->second) {
      // we are improving => push the current sequence into SDT and SOPD
      
      SDT.Data.push_back(currSequence);
      SOPD.at(currDepth).push_back(currSequence);   
    }

    currSequence = Sequence();
    prevSequence = Sequence();
  }

  // return true if can continue scheduling
  bool cascadeAdvanceCartesian() {

    // increment the iterator of the singleIdx
    singleIdx++;

    // check if we reached the end of the available passes (i.e. we should test the next sequence)
    if (singleIdx >= this->availablePasses.size()) {
      currIdx++;
      singleIdx = 0;
    }
    // check if we reached the end of the sequences at the current depth (i.e. we should go deeper)
    if (currIdx >= SOPD.at(currDepth-1).size()) {
      currDepth++;
      currIdx = 0;
    }
    return currDepth >= MaxDepth;
  }

};

} // namespace Lavinium
