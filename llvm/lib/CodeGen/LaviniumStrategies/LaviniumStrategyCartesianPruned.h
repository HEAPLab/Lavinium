#pragma once

#include "LaviniumStrategyEnum.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Lavinium.h"
#include "llvm/IR/LaviniumStrategy.h"
#include "llvm/IR/LaviniumTypes.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/LineIterator.h"
#include "llvm/Support/MemoryBuffer.h"
#include <any>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace Lavinium {

namespace {
using csiter = std::vector<std::string>::const_iterator;
using Sequence = std::vector<csiter>;



template<typename Metric>
class CartesianElement
{
  enum class CartesianState{
  ORIG, 
  INVE,
  PRUN,
  SKIP,
  };

  std::vector<Sequence>& AllPassSnippet;
  const std::vector<std::string>& availablePasses;
  const CachedPassesMetric<Metric>& cachedPassesMetric;
  std::unordered_map<std::string, csiter> availablePassesLUT;
  size_t allPassIndex = 0;
  size_t availablePassesIndex = 0;
  enum CartesianState CS = CartesianState::ORIG;

  template<typename T>
  friend class StrategyCartesianPruned;

//Return the cartesian product of element in position (AllPassSnippet X availablePassesIndex)
//NB AllPassSnippet[allPassIndex] are vector that is flattned at the beggining of the result
std::optional<Sequence>  originalCartesian(){
  if(allPassIndex >= AllPassSnippet.size()){
    return std::nullopt;
  }
  CS = CartesianState::INVE;
  Sequence res;
  assert(allPassIndex < AllPassSnippet.size() && "All pass index out of bound");
  assert(availablePassesIndex < availablePasses.size() && "Avaialable pass index out of bound");
  const Sequence& firstPart = AllPassSnippet[allPassIndex];
  std::copy(firstPart.begin(), firstPart.end(), std::back_inserter(res));
  auto second_part = availablePasses.begin() + availablePassesIndex; 
  res.push_back(second_part);
  if(cachedPassesMetric.find((const Sequence &)res) != cachedPassesMetric.end() ||
    std::any_of(res.begin(), res.end()-1,[second_part](csiter iter){return *iter == *second_part;})){
    CS = CartesianState::SKIP;
    return std::nullopt;
  }
  return res;
}


//Return the cartesian product of element in position (availablePassesIndex X AllPassSnippet)
//NB AllPassSnippet[allPassIndex] are vector that is flattned at the end of the result
std::optional<Sequence>  inverseCartesian(){
  CS = CartesianState::PRUN;
  Sequence res;
  assert(allPassIndex < AllPassSnippet.size() && "All pass index out of bound");
  assert(availablePassesIndex < availablePasses.size() && "Avaialable pass index out of bound");
  res.push_back(availablePasses.begin() + availablePassesIndex);
  const Sequence& secondPart = AllPassSnippet[allPassIndex];
  std::copy(secondPart.begin(), secondPart.end(), std::back_inserter(res));
  return res;
}

//the only stable iterator are the one that refear to availablePasses. AllPassSnippet can be invalidated by next due to prune
Sequence toStableIterator(Sequence& seq){
  Sequence res;
  res.reserve(seq.size());
  for(csiter elem : seq){
    res.push_back(availablePassesLUT[*elem]);
  }
  return res;
}


void dump(){
  for(auto x : cachedPassesMetric){
    llvm::dbgs() << x.first.toString() << "\n";
  }
}

// Check if the original sequence and is inverse are variant. if so they are added to the passes to test.
void prune(){
  Sequence original;
  const Sequence& firstPart = AllPassSnippet[allPassIndex];
  std::copy(firstPart.begin(), firstPart.end(), std::back_inserter(original));
  original.push_back(availablePasses.begin() + availablePassesIndex);


  CS = CartesianState::ORIG;
  Sequence inverted;
  inverted.push_back(availablePasses.begin() + availablePassesIndex);
  const Sequence& secondPart = AllPassSnippet[allPassIndex];
  std::copy(secondPart.begin(), secondPart.end(), std::back_inserter(inverted));
  if(cachedPassesMetric.find((const Sequence &) original) != cachedPassesMetric.end()){
    dump();
  }
  assert(cachedPassesMetric.find((const Sequence &) original) != cachedPassesMetric.end() && "original not found");
  assert(cachedPassesMetric.find((const Sequence &) inverted) != cachedPassesMetric.end() && "original not found");
  if(cachedPassesMetric.at((const Sequence &)original) != cachedPassesMetric.at((const Sequence &)inverted)){
  original = toStableIterator(original);
  inverted = toStableIterator(inverted);
  AllPassSnippet.push_back(original);
  AllPassSnippet.push_back(inverted);
  }
}

void increment(){
  availablePassesIndex++;
  if(availablePassesIndex >= availablePasses.size()){
    availablePassesIndex=0;
    //originalCartesian is in charge to check if all PassIndex is to big and terminate
    allPassIndex++;
  }
}


  public:
  
  CartesianElement( std::vector<Sequence>& SCPAllPassSnippet, const std::vector<std::string>& SCPavailablePasses, const CachedPassesMetric<Metric>& SCPcachedPassesMetric ) : 
    AllPassSnippet(SCPAllPassSnippet), 
    availablePasses(SCPavailablePasses), 
    cachedPassesMetric(SCPcachedPassesMetric), availablePassesLUT() {
    availablePassesLUT.reserve(availablePasses.size());
    csiter begin = availablePasses.begin();
    csiter end = availablePasses.end();
    for(;begin != end; ++begin){
      availablePassesLUT[*begin] = begin;
    }
  }


  //Like an iterator (allPassIndex, availablePassesIndex) pair points to the first not explored
  std::optional<Sequence> next(){
    switch (CS){
      case CartesianState::ORIG:
      {
        auto ret = originalCartesian();
        if(CS == CartesianState::SKIP){
          return next();
        }
        return ret;
      }
        break;
      case CartesianState::INVE:
        return inverseCartesian();
        break;
      case CartesianState::PRUN:
        prune();
        increment();
        return next();
        break;
      case CartesianState::SKIP:
        increment();
        CS = CartesianState::ORIG;
        return next();
      break;
    }
  }

    

};

} // namespace





template <typename Metric>
class StrategyCartesianPruned : public Strategy<Metric> {

  // Contains a vector of const_iterators of strings. The strings are the
  // passes.
  std::vector<Sequence> AllPassSnippet;
  // Contains a vectore of const_iterator of SDT elements used for Greedy
  StrategyDeepTracker<
      std::pair<std::vector<std::vector<Sequence>::const_iterator>,
                std::vector<Sequence>::const_iterator>>
      GreedySDT;

  std::map<std::string, std::set<std::string>> mapIdempotentPasses;

  // Vector of set of sequences of passes.
  // Each each element of the vector is the set of sequences obtained with the
  // exploration at the depth corresponding to the index.
  std::vector<std::vector<Sequence>> SOPD; // Sequences Of Passes per Depth
  size_t currDepth = 1;
  size_t singleIdx = 0;
  size_t currIdx = 0;
  std::unique_ptr<CartesianElement<Metric>> CE;

  Sequence currSequence;
  Sequence prevSequence;

  size_t MaxDepth = 1;

  bool cartesianExplored = false;

public:
  StrategyCartesianPruned(const CachedPassesMetric<Metric> *cached)
      : Strategy<Metric>(cached), CE() {
    initialize();
    CE = std::unique_ptr<CartesianElement<Metric>>( new CartesianElement<Metric>(AllPassSnippet, this->availablePasses, *this->cachedPassesMetric));
  };

  std::optional<std::vector<std::string>> suggestPasses() override {

    // verify whether the current sequence improved the WCET (wrt the previous)
    // and eventually add it to the SDT
    /*verifyImprovement();*/

    std::vector<std::string> res;

    if (cartesianExplored) {
      std::vector<std::string> res;
      Sequence tmp;
      const Sequence &ltmp = tmp;
      do {
        auto &[Prevs, Current] = GreedySDT.Data;
        tmp.clear();

        if (Current == AllPassSnippet.end()) {
          std::vector<std::vector<Sequence>::const_iterator> tmp =
              AdvanceGreedy();
          if (Prevs.size() == tmp.size()) {
            return std::nullopt;
          }
          Prevs = tmp;
          Current = AllPassSnippet.begin();
        }
        for (const auto &Prev : Prevs) {
          std::copy(Prev->begin(), Prev->end(), std::back_inserter(tmp));
        }
        std::copy(Current->begin(), Current->end(), std::back_inserter(tmp));

        Current = std::next(Current);
      } while (this->cachedPassesMetric->find(ltmp) !=
               this->cachedPassesMetric->end());
      for (const auto &t : tmp) {
        res.push_back(t->c_str());
      }
      return res;

    } else { // explore the cartesian space
      Sequence seq;

      // take the single pass
      /*if (schedInverse == true) {*/
      /*  schedInverse = false;*/
      /*  seq = SOPD.size() > 0 ? Sequence(SOPD.at(currDepth - 1).at(currIdx))*/
      /*                        : Sequence();*/
      /*  prevSequence = Sequence(seq);*/
      /*  auto singlePass = this->availablePasses.cbegin() + schedInverseIndex;*/
      /*  Sequence tmp;*/
      /*  tmp.push_back(singlePass);*/
      /*  std::copy(seq.begin(), seq.end(), std::back_inserter(tmp));*/
      /*  if (this->cachedPassesMetric->find((const Sequence &)tmp) ==*/
      /*      this->cachedPassesMetric->end()) {*/
      /*    for (auto elem : tmp) {*/
      /*      res.push_back(*elem.base());*/
      /*    }*/
      /*    return res;*/
      /*  }*/
      /*}*/
      /**/
      /*bool newPass = false;*/
      /*// check whether we have already explored prev, if yes, add the new pass*/
      /*// to the sequence*/
      /*do {*/
      /*  // take the passes at currDepth-1*/
      /*  seq = SOPD.size() > 0 ? Sequence(SOPD.at(currDepth - 1).at(currIdx))*/
      /*                        : Sequence();*/
      /*  prevSequence = Sequence(seq);*/
      /**/
      /*  const Sequence &p = prevSequence;*/
      /*  LaviniumScheduledPasses prev{p};*/
      /*  auto singlePass = this->availablePasses.cbegin() + singleIdx;*/
      /*  newPass = true;*/
      /*  if (this->cachedPassesMetric->find(prev) !=*/
      /*      this->cachedPassesMetric->end()) {*/
      /*    auto entry = mapIdempotentPasses.find(*p.back());*/
      /*    std::string lastPass = *singlePass;*/
      /**/
      /*    // the entry cannot be found*/
      /*    if (entry == mapIdempotentPasses.end() ||*/
      /*        entry->second.find(lastPass) ==*/
      /*            entry->second.end()) { // the pass can be scheduled*/
      /*      // concatenate the two*/
      /*      seq.push_back(singlePass);*/
      /*      if(this->cachedPassesMetric->find((const Sequence &)seq) == this->cachedPassesMetric->end()){*/
      /*      schedInverse = true;*/
      /*      schedInverseIndex = singleIdx;*/
      /**/
      /*      }else {*/
      /*        newPass = false;*/
      /*        seq.pop_back();*/
      /*      }*/
      /*    } else {*/
      /*      newPass = false;*/
      /*    }*/
      /*    cartesianExplored = cascadeAdvanceCartesian();*/
      /*  }*/
      /*} while (!newPass && !cartesianExplored);*/
      /*if (cartesianExplored) {*/
      /*  initGreedy();*/
      /*}*/
      
      auto optSeq = CE->next();

      if (!optSeq.has_value()){
        cartesianExplored=true;
        initGreedy();
        return suggestPasses();
      }
      seq = *optSeq;
      for (auto elem : seq) {
        res.push_back(*elem.base());
      }
    }
    return res;
  }

  ~StrategyCartesianPruned() override = default;

private:
  void initGreedy() {
    GreedySDT = StrategyDeepTracker<
        std::pair<std::vector<std::vector<Sequence>::const_iterator>,
                  std::vector<Sequence>::const_iterator>>{
        {{}, AllPassSnippet.cbegin()}, 0};
  }

  std::vector<std::vector<Sequence>::const_iterator> AdvanceGreedy() {
    std::vector<std::vector<Sequence>::const_iterator> ret;
    auto minimumPasses = findMinimum()->getIds();
    int i = 0;
    for (const auto &minimumPass : minimumPasses) {
      ret.push_back(std::find_if(AllPassSnippet.cbegin(), AllPassSnippet.cend(),
                                 [&minimumPass](const Sequence &elem) {
                                   if (elem.size() != 1)
                                     return false;
                                   return *elem[0] == minimumPass;
                                 }));
      assert(ret[i] != AllPassSnippet.end() && "Minimum not founded");
      i++;
    }
    return ret;
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
      if (key.getIds()[0] == "baseline") {
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

    // // SOPD has 2 items at the beginning, one for depth 0 (single passes), one
    // // for depth 1 (pairs of passes)
    SOPD.push_back(std::vector<Sequence>());
    SOPD.push_back(std::vector<Sequence>());

    // insert the available passes into the allpasssnippet and sopd
    for (csiter Item = this->availablePasses.cbegin();
         Item != this->availablePasses.cend(); ++Item) {
      AllPassSnippet.push_back(
          Sequence{Item}); // push the vector in the SOPD at depth 0
      SOPD.at(0).push_back(Sequence{Item});
      mapIdempotentPasses.insert(std::pair<std::string, std::set<std::string>>(
          *Item, std::set<std::string>()));
      mapIdempotentPasses.find(*Item)->second.insert(*Item);
    }
  }

  void verifyImprovement() {
    if (currSequence.size() == 0 || prevSequence.size() == 0)
      return;
    const Sequence &c = currSequence;
    const Sequence &p = prevSequence;
    LaviniumScheduledPasses curr{c};
    LaviniumScheduledPasses prev{p};

    auto begin = this->cachedPassesMetric->begin();
    auto end = this->cachedPassesMetric->end();

    if (std::find(AllPassSnippet.begin(), AllPassSnippet.end(), currSequence) ==
        AllPassSnippet.end()) {
      AllPassSnippet.push_back(currSequence);
      SOPD.at(currDepth).push_back(currSequence);
    }

    currSequence = Sequence();
    prevSequence = Sequence();
  }

  void saveIdempotentPairs(std::vector<Sequence> sequences) {
    for (auto sequence : sequences) {
      Sequence s1 = Sequence(sequence);
      Sequence s2 = Sequence();
      
      s2.push_back(sequence[sequence.size()-1]);
      std::copy(sequence.begin(), sequence.end()-1, std::back_inserter(s2));

      const Sequence &cs1 = s1;
      const Sequence &cs2 = s2;
      LaviniumScheduledPasses original{cs1};
      LaviniumScheduledPasses inverted{cs2};
      assert(this->cachedPassesMetric->find(original) != this->cachedPassesMetric->end());
      assert(this->cachedPassesMetric->find(inverted) != this->cachedPassesMetric->end());

      if (this->cachedPassesMetric->find(original)->second ==
          this->cachedPassesMetric->find(inverted)
              ->second) { // they are idempotent

        mapIdempotentPasses[original.at(original.size()-1)].insert(inverted.at(0));
        mapIdempotentPasses[(inverted.at(0))].insert(original.at(0));
      }
    }
  }

  // return false if can continue scheduling cartesian
  bool cascadeAdvanceCartesian() {

    // increment the iterator of the singleIdx
    singleIdx++;

    // check if we reached the end of the available passes (i.e. we should test
    // the next sequence)
    if (singleIdx >= this->availablePasses.size()) {
      currIdx++;
      singleIdx = 0;
    }
    // check if we reached the end of the sequences at the current depth (i.e.
    // we should go deeper)
    if (currIdx >= SOPD.at(currDepth - 1).size()) {
      // check whether we had an improvement
      if (SOPD.at(currDepth).size() == 0) {
        return true; // we had no improvement, we can run the greedy
      }
      currDepth++;
      currIdx = 0;
      // create a new vector for the next depth
      SOPD.push_back(std::vector<Sequence>());
        // fill a data structure containing idempotent pairs
        saveIdempotentPairs(SOPD.at(currDepth-1));
    }
    return false;
  }
};


} // namespace Lavinium
