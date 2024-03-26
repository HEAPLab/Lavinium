#pragma once

#include "llvm/ADT/DenseMap.h"
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

namespace {
enum class EXPTYPE { Cartesian, MinimumOnly };
} // namespace

namespace Lavinium {

template <class T> struct StrategyDeepTracker {
  T Data;
  size_t Depth = 0;
};

template <typename Metric> class StrategyImpl : public Strategy {
  const CachedFunctionMetric<Metric> *cachedFunctionMetric;
  std::vector<std::string> availablePasses;
  llvm::DenseMap<llvm::Function *,
                 StrategyDeepTracker<
                     std::vector<decltype(availablePasses)::const_iterator>>>
      Iterators;
  EXPTYPE Type;
  size_t MaxDepth = 1;

  inline std::vector<std::string> loadPasses(std::string Path) {
    auto File = llvm::MemoryBuffer::getFile(Path, true);
    std::vector<std::string> Res;
    if (File) {
      auto &Buffer = *File.get();
      auto LineIterator = llvm::line_iterator(Buffer, true, '#');
      auto EndLineIterator = llvm::line_iterator();

      // First line Must be  or CARTESIAN or MINIMUM
      std::string type = LineIterator->str();
      if (type == "CARTESIAN") {
        Type = EXPTYPE::Cartesian;
      } else if (type == "MINIMUM") {
        Type = EXPTYPE::MinimumOnly;
      } else {
        assert(type == "CARTESIAN" && "Not Found Type");
      }
      LineIterator++;

      // Second Line Must be the depth of the analysis
      std::string number = LineIterator->str();
      MaxDepth = std::stol(number) - 1;
      LineIterator++;

      while (LineIterator != EndLineIterator) {
        Res.push_back(LineIterator->str());
        LineIterator++;
      }
      return Res;
    }
    assert(File && "File Not Found");
    return Res;
  }

public:
  StrategyImpl(const CachedFunctionMetric<Metric> *cached)
      : cachedFunctionMetric(cached), availablePasses() {

    if (LaviniumFile != "") {
      availablePasses = loadPasses(LaviniumFile);
    }
  };

  void initialize(llvm::Function *Function) {
    Iterators.insert(
        {Function,
         {std::vector<typename decltype(availablePasses)::const_iterator>(
              MaxDepth + 1, availablePasses.begin()),
          0}});
  }

  // return true if can continue scheduling
  bool cascadeAdvanceMinimum(llvm::Function *Function, size_t currentLevel) {

    auto &[Its, Depth] = Iterators.getOrInsertDefault(Function);
    if (Depth == MaxDepth) {
      return false;
    } else {
      auto &minimum_last = findMinimum(Function, Depth)->getIds().back();
      Its[Depth] = std::find_if(
          availablePasses.begin(), availablePasses.end(),
          [&minimum_last](auto &elem) { return elem == minimum_last; });
      Depth++;
      return true;
    }
  }

  // return true if can continue scheduling
  bool cascadeAdvanceCartesian(llvm::Function *Function, size_t currentLevel) {
    auto end = availablePasses.end();
    auto begin = availablePasses.begin();

    auto &[Its, Depth] = Iterators.getOrInsertDefault(Function);
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

  std::optional<std::vector<std::string>>
  suggestCartesian(llvm::Function *Function) {

    if (!Iterators.contains(Function)) {
      initialize(Function);
    }

    auto &[Its, Depth] = Iterators.getOrInsertDefault(Function);

    std::vector<std::string> res;
    if (Its.at(Depth) == availablePasses.end()) {
      bool canContinue = cascadeAdvanceCartesian(Function, Depth);
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
  }

  std::optional<std::vector<std::string>>
  suggestMinimum(llvm::Function *Function) {

    if (!Iterators.contains(Function)) {
      initialize(Function);
    }

    auto &[Its, Depth] = Iterators.getOrInsertDefault(Function);

    std::vector<std::string> res;
    if (Its.at(Depth) == availablePasses.end()) {
      bool canContinue = cascadeAdvanceMinimum(Function, Depth);
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

  std::optional<std::vector<std::string>>
  suggestPasses(llvm::Function *Function) override {

    if (Type == EXPTYPE::Cartesian) {
      return suggestCartesian(Function);
    }
    if (Type == EXPTYPE::MinimumOnly) {
      return suggestMinimum(Function);
    }
    return std::nullopt;
  }

  // Find the minimum wcet for a certain level of depth
  auto findMinimum(llvm::Function *Function, int Depth) {
    auto &cachedFunction = cachedFunctionMetric->at(Function);
    typename CachedPassesType<Metric>::mapped_type min_data;
    const typename CachedPassesType<Metric>::key_type *min_key;
    auto begin = cachedFunction.begin();
    auto end = cachedFunction.end();

    min_data = begin->second;
    min_key = &(begin->first);

    while (begin != end) {
      auto &[key, data] = *begin;
      if (key.size() - 1 != (size_t)Depth) {
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

  std::vector<std::string> getFinal(llvm::Function *Function) override {
    auto &cachedFunction = cachedFunctionMetric->at(Function);
    assert(cachedFunction.size() > 0 &&
           "Getting the minimum of an empty vector");
    auto minimum = std::min_element(
        cachedFunction.begin(), cachedFunction.end(), [](auto &lft, auto &rgt) {
          if (lft.second == rgt.second) {
            return lft.first.size() < rgt.first.size();
          }
          return lft.second < rgt.second;
        });

    return minimum->first.getIds();
  }

  ~StrategyImpl() override = default;
};

} // namespace Lavinium