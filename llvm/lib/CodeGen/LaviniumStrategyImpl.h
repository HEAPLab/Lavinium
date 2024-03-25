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
  bool cascadeAdvance(llvm::Function *Function, size_t currentLevel) {
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
    auto &It = Its.at(Depth);
    if (It == availablePasses.end()) {
      bool canContinue = cascadeAdvance(Function, Depth);
      if (!canContinue) {
        return std::nullopt;
      }
    }

    for (size_t i = 0; i <= Depth; i++) {
      res.push_back(Its[i]->c_str());
    }
    It = std::next(It);
    return res;
  }

  std::optional<std::vector<std::string>>
  suggestMinimum(llvm::Function *Function) {
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
  }

  std::vector<std::string> getFinal(llvm::Function *Function) override {
    auto &cachedFunction = cachedFunctionMetric->at(Function);
    assert(cachedFunction.size() > 0 &&
           "Getting the minimum of an empty vector");
    auto minimum = std::min_element(
        cachedFunction.begin(), cachedFunction.end(),
        [](auto &lft, auto &rgt) { return lft.second < rgt.second; });
    return minimum->first.getIds();
  }

  ~StrategyImpl() override = default;
};

} // namespace Lavinium