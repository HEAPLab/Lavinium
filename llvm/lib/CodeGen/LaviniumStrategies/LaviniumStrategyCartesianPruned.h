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
#include <set>

namespace Lavinium {

namespace {
using csiter = std::vector<std::string>::const_iterator;
using Sequence = std::vector<csiter>;

} // namespace

template <typename Metric> class StrategyCartesianPruned : public Strategy<Metric> {

// Contains a vector of const_iterators of strings. The strings are the passes.
std::vector<Sequence> AllPassSnippet;
// Contains a vectore of const_iterator of SDT elements used for Greedy
StrategyDeepTracker<std::pair<std::vector<std::vector<Sequence>::const_iterator>, std::vector<Sequence>::const_iterator>> GreedySDT;

std::map<std::string, std::set<std::string>> mapIdempotentPasses;

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
    std::vector<std::string> res;
    Sequence tmp;
    const Sequence& ltmp =tmp;
    do{
    auto &[Prevs, Current] = GreedySDT.Data;
    tmp.clear();

    if (Current == AllPassSnippet.end()) {
      std::vector<std::vector<Sequence>::const_iterator> tmp = AdvanceGreedy();
      if (Prevs.size() == tmp.size()) return std::nullopt;
      Prevs = tmp;
      Current = AllPassSnippet.begin();
    }
    for (const auto& Prev : Prevs){
      std::copy(Prev->begin(), Prev->end(), std::back_inserter(tmp));
    }
    std::copy(Current->begin(), Current->end(), std::back_inserter(tmp));

    Current = std::next(Current);
    }
    while(this->cachedPassesMetric->find(ltmp) != this->cachedPassesMetric->end());
    for(const auto & t : tmp){
          res.push_back(t->c_str());
    }   
    return res;

    }
    else { // explore the cartesian space
      Sequence seq;
      
      // take the single pass

      // take the passes at currDepth-1
      seq = SOPD.size() > 0 ? Sequence(SOPD.at(currDepth-1).at(currIdx)) : Sequence();
      prevSequence = Sequence(seq);

      const Sequence &p = prevSequence;
      LaviniumScheduledPasses prev{p};
      bool newPass = false;
      // check whether we have already explored prev, if yes, add the new pass to the sequence
      do{
      auto singlePass = this->availablePasses.cbegin() + singleIdx;
      newPass = true;
      if (this->cachedPassesMetric->find(prev) != this->cachedPassesMetric->end()) {
        auto entry = mapIdempotentPasses.find(*p.back());
        std::string lastPass = *singlePass;
        // the entry cannot be found
        if (entry == mapIdempotentPasses.end() || entry->second.find(lastPass) == entry->second.end()) { // the pass can be scheduled
          // concatenate the two
          seq.push_back(singlePass);
        }else {
          newPass = false;
        }
      }
      cartesianExplored = cascadeAdvanceCartesian();
      }while(!newPass && !cartesianExplored);
      if(cartesianExplored){
        initGreedy();
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


  void initGreedy(){
    GreedySDT = StrategyDeepTracker<std::pair<std::vector<std::vector<Sequence>::const_iterator>, std::vector<Sequence>::const_iterator>>{{{}, AllPassSnippet.cbegin()},0};
  }

  // return true if can continue scheduling
  std::vector<std::vector<Sequence>::const_iterator> AdvanceGreedy() {

      std::vector<std::vector<Sequence>::const_iterator> ret;
      auto minimumPasses = findMinimum()->getIds();
      int i = 0;
      for (const auto & minimumPass: minimumPasses){
      ret.push_back( std::find_if(
            AllPassSnippet.cbegin(), AllPassSnippet.cend(),
          [&minimumPass](const Sequence &elem) { 
          if(elem.size() != 1) return false;
          return *elem[0] == minimumPass; 
          }));
      assert(ret[i] != AllPassSnippet.end() && "Minimum not founded");
      i++;
      return ret;
    }
  }

  // Find the minimum wcet for a certain up to a certain level of depth
  auto findMinimum() {
    typename CachedPassesMetric<Metric>::mapped_type min_data;
    const typename CachedPassesMetric<Metric>::key_type *min_key;
    auto begin = this->cachedPassesMetric->begin();
    auto end = this->cachedPassesMetric->end();

    min_data = begin->second;
    min_key = &(begin->first);

    while (begin != end) {
      auto &[key, data] = *begin;
      if(key.getIds()[0] == "baseline"){
        begin++;
        continue;
        }
      if (min_data > data) {
        min_data = std::min(data, min_data);
        min_key = &key;
      } else if (min_data == data && key.size() < min_key->size()) {
        min_data = std::min(data, min_data);
        min_key = &key;
      }
      begin++;
    }
    return min_key;
  }




  void initialize() {

    // SOPD has 2 items at the beginning, one for depth 0 (single passes), one for depth 1 (pairs of passes)
    SOPD.push_back(std::vector<Sequence>());
    SOPD.push_back(std::vector<Sequence>());

    // insert the available passes into the allpasssnippet and sopd
    for (csiter Item = this->availablePasses.cbegin(); Item != this->availablePasses.cend(); ++Item) {
      AllPassSnippet.push_back(Sequence{Item}); // push the vector in the SOPD at depth 0
      SOPD.at(0).push_back(Sequence{Item});
      mapIdempotentPasses.insert(std::pair<std::string, std::set<std::string>>(*Item, std::set<std::string>()));
      mapIdempotentPasses.find(*Item)->second.insert(*Item);
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
      llvm::dbgs() << begin->second << "\n";
      begin++;
    }

    if(this->cachedPassesMetric->find(prev) == this->cachedPassesMetric->end() || this->cachedPassesMetric->find(curr)->second >
      this->cachedPassesMetric->find(prev)->second) {
      // we are improving => push the current sequence into SDT and SOPD
      
      AllPassSnippet.push_back(currSequence);
      SOPD.at(currDepth).push_back(currSequence);   
    }

    currSequence = Sequence();
    prevSequence = Sequence();
  }

  void saveIdempotentPairs() {
    for (auto sequence : SOPD[1]) {
      Sequence s1 = Sequence(sequence);
      Sequence s2 = Sequence();
      s2.push_back(sequence[1]);
      s2.push_back(sequence[0]);


      
      const Sequence &cs1 = s1;
      const Sequence &cs2 = s2;
      LaviniumScheduledPasses original{cs1};
      LaviniumScheduledPasses inverted{cs2};

      if (this->cachedPassesMetric->find(original)->second ==
          this->cachedPassesMetric->find(inverted)->second) { // they are idempotent
        if (mapIdempotentPasses.find(original.at(0)) == mapIdempotentPasses.end()) { // there is no entry for the pass
          mapIdempotentPasses.insert(std::pair<std::string, std::set<std::string>>(original.at(0), std::set<std::string>()));
        }
        if (mapIdempotentPasses.find(inverted.at(0)) == mapIdempotentPasses.end()) { // there is no entry for the pass
          mapIdempotentPasses.insert(std::pair<std::string, std::set<std::string>>(inverted.at(0), std::set<std::string>()));
        }
        mapIdempotentPasses.find(original.at(0))->second.insert(inverted.at(0));
        mapIdempotentPasses.find(inverted.at(0))->second.insert(original.at(0));
      }
    }
  }

  // return false if can continue scheduling cartesian
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
      // check whether we had an improvement
      if (SOPD.at(currDepth).size() == 0) {
        return true; // we had no improvement, we can run the greedy
      }
      currDepth++;
      currIdx = 0;
      // create a new vector for the next depth
      SOPD.push_back(std::vector<Sequence>());
      if (currDepth == 2) { // if we have explored all combinations of passes (pairs)
        // fill a data structure containing idempotent pairs
        saveIdempotentPairs();
      }
    }
    return false;
  }

};

} // namespace Lavinium
