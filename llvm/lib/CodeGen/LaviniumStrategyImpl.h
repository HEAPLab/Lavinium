#pragma once

#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Lavinium.h"
#include "llvm/IR/LaviniumStrategy.h"
#include "llvm/IR/LaviniumTypes.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/LineIterator.h"
#include "llvm/Support/MemoryBuffer.h"
#include <iterator>
#include <optional>
#include <vector>

namespace {
inline std::vector<std::string> loadPasses(std::string Path) {
  auto File = llvm::MemoryBuffer::getFile(Path, true);
  std::vector<std::string> Res;
  if (File) {
    auto &Buffer = *File.get();
    auto LineIterator = llvm::line_iterator(Buffer, true, '#');
    auto EndLineIterator = llvm::line_iterator();
    while (LineIterator != EndLineIterator) {
      Res.push_back(LineIterator->str());
      LineIterator++;
    }
    return Res;
  }
  assert(File && "File Not Found");
  return Res;
}
} // namespace

namespace Lavinium {

template <typename Metric> class StrategyImpl : public Strategy {
  const CachedFunctionMetric<Metric> *cachedFunctionMetric;
  std::vector<std::string> availablePasses;
  llvm::DenseMap<llvm::Function *, decltype(availablePasses)::const_iterator>
      Iterator;

public:
  StrategyImpl(const CachedFunctionMetric<Metric> *cached)
      : cachedFunctionMetric(cached), availablePasses() {

    if (LaviniumFile != "") {
      availablePasses = loadPasses(LaviniumFile);
    }
  };

  std::optional<std::vector<std::string>>
  suggestPasses(llvm::Function *Function) override {
    if (!Iterator.contains(Function)) {
      Iterator.insert({Function, availablePasses.begin()});
      auto &start = Iterator.at(Function);
      return {{*start}};
    } else {

      auto &It = Iterator.getOrInsertDefault(Function);
      It = std::next(It);

      if (It != availablePasses.end()) {
        return {{*It}};
      } else {
        return std::nullopt;
      }
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