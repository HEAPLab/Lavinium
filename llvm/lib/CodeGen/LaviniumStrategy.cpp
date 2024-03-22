#pragma once

#include "llvm/IR/LaviniumStrategy.h"
#include "llvm/IR/Lavinium.h"
#include "llvm/IR/LaviniumTypes.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/LineIterator.h"
#include "llvm/Support/MemoryBuffer.h"
#include <vector>

namespace utils {
std::vector<std::string> loadPasses(std::string Path) {
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
} // namespace utils

namespace Lavinium {

template <typename Metric> class StrategyImpl : public Strategy {
  const CachedFunctionMetric<Metric> *cachedFunctionMetric;
  std::vector<std::string> availablePasses;

public:
  StrategyImpl(const CachedFunctionMetric<Metric> *cached)
      : cachedFunctionMetric(cached), availablePasses() {

    if (LaviniumFile != "") {
      availablePasses = utils::loadPasses(LaviniumFile);
    }
  };

  std::optional<std::vector<std::string>> suggestPasses() override {}

  ~StrategyImpl() override = default;
};

} // namespace Lavinium