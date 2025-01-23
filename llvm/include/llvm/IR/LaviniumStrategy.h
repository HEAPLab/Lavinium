#pragma once

#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/LineIterator.h"
#include "llvm/Support/MemoryBuffer.h"
#include "LaviniumTypes.h"
#include <optional>
#include <string>
#include <vector>

// clang-format off
// ╔═╗┌┬┐┬─┐┌─┐┌┬┐┌─┐┌─┐┬ ┬
// ╚═╗ │ ├┬┘├─┤ │ ├┤ │ ┬└┬┘
// ╚═╝ ┴ ┴└─┴ ┴ ┴ └─┘└─┘ ┴
// clang-format on

namespace Lavinium {

template <class T> struct StrategyDeepTracker {
  T Data;
  size_t Depth = 0;
};

// Add to the list of passes
// List Pass Names HERE: ./llvm/lib/Passes/PassRegistry.def
template <class Metric>
class Strategy {

protected:
  const CachedPassesMetric<Metric> *cachedPassesMetric;
  std::vector<std::string> availablePasses;
  Strategy(const CachedPassesMetric<Metric> *cached) : cachedPassesMetric(cached), availablePasses() {
    if (LaviniumFile != "") parsePasses(LaviniumFile);
    else assert(false && "Please provide a PASSES file");
  }
  Strategy(const Strategy &) = delete;
  Strategy(Strategy &&) = delete;

public:
  virtual ~Strategy() = default;
  // Returns a vector of strings containing the names of the next passes to schedule.
  // If there are no passes to suggest, returns std::nullopt
  virtual std::optional<std::vector<std::string>> suggestPasses() = 0;

   // returns the best sequence of passes
  std::vector<std::string> getFinal() {
    auto &cachedFunction = *cachedPassesMetric;
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

  void parsePasses(std::string Path) {
    auto File = llvm::MemoryBuffer::getFile(Path, true);
    if (File) {
      auto &Buffer = *File.get();
      auto LineIterator = llvm::line_iterator(Buffer, true, '#');
      auto EndLineIterator = llvm::line_iterator();

      while (LineIterator != EndLineIterator) {
        availablePasses.push_back(LineIterator->str());
        LineIterator++;
      }
    }
    assert(File && "File Not Found");
  }
};

} // namespace Lavinium