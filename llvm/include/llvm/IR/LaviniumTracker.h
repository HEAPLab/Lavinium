#pragma once

#include "LaviniumFunctionTracker.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LaviniumPassManagerWrapper.h"
#include "llvm/Pass.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Lavinium {

class LaviniumScheduledPasses {
  std::vector<std::string> Ids; // Passes are identified by ID

public:
  template <typename... Args>
  LaviniumScheduledPasses(Args &&...args) : Ids{std::forward<Args>(args)...} {}
  LaviniumScheduledPasses() = default;
  LaviniumScheduledPasses(const LaviniumScheduledPasses &) = default;
  LaviniumScheduledPasses(LaviniumScheduledPasses &&) = default;
  LaviniumScheduledPasses &operator=(const LaviniumScheduledPasses &) = default;
  LaviniumScheduledPasses &operator=(LaviniumScheduledPasses &&) = default;

  template <typename... Args> void pushBack(Args &&...args) {
    (Ids.push_back(std::forward<Args>(args)), ...);
  }

  bool operator==(const LaviniumScheduledPasses &rhs) const {
    if (this->Ids.size() != rhs.Ids.size())
      return false;
    auto size = this->Ids.size();
    for (size_t i = 0; i < size; i++) {
      if (this->Ids[i] != rhs.Ids[i]) {
        return false;
      }
    }
    return true;
  }

  auto &getIds() const { return this->Ids; }
  std::string toString() const {
    std::string name;
    for (auto [i, Key] : llvm::enumerate(Ids)) {
      name += Key;
      if (i < Ids.size() - 1) {
        llvm::dbgs() << " - ";
      }
    }
    return name;
  };

  bool isEmpty() const { return Ids.empty(); };
  explicit operator bool() const { return this->Ids.size() > 0; };
};
} // namespace Lavinium

template <> struct std::hash<Lavinium::LaviniumScheduledPasses> {
  std::size_t
  operator()(const Lavinium::LaviniumScheduledPasses &s) const noexcept {

    auto seed = 0;
    for (auto &view : s.getIds()) {
      seed ^= std::hash<std::string>{}(view) + 0x9e3779b9 + (seed << 6) +
              (seed >> 2);
    }
    return seed;
  }
};

namespace Lavinium {

template <typename Metric>
using CachedPassesType = std::unordered_map<LaviniumScheduledPasses, Metric>;

template <typename Metric>
using CachedFunctionMetric =
    llvm::DenseMap<llvm::Function *, CachedPassesType<Metric>>;

template <typename Metric> class LaviniumTracker {

private:
  void setFunctionTracker(std::unique_ptr<FunctionTracker> &&FT) {
    functionTracker.swap(FT);
  }

  void setPassManagerWrapper(std::unique_ptr<PassManagerWrapper> &&PMW) {
    PassManager.swap(PMW);
  }

public:
  void Init(std::unique_ptr<FunctionTracker> FT,
            std::unique_ptr<PassManagerWrapper> PMW) {
    setFunctionTracker(std::move(FT));
    setPassManagerWrapper(std::move(PMW));
  }

  bool checkInit() const {
    return static_cast<bool>(functionTracker) && static_cast<bool>(PassManager);
  }

  template <typename... Args> void addToSchedule(Args &&...args) {
    assert(checkInit() && ("Call   before init"));
    Scheduled.pushBack(std::forward<Args>(args)...);
  }

  // Return a reference to the scheduled pass
  auto &getScheduled() const { return Scheduled; }

  // Return if the store has overritten an already present entry
  bool storeMetric(llvm::Function *Function, Metric &M) {
    assert(checkInit() && "Not Init");
    if (!CachedFunctions.contains(Function)) {
      auto inserted = CachedFunctions.getOrInsertDefault(Function);
      inserted.insert(std::pair{std::move(Scheduled), M});
      return false;
    } else {
      CachedFunctions.getOrInsertDefault(Function).insert(
          std::pair{std::move(Scheduled), M});
      return true;
    }
  }

  template <typename... Args>
  bool alreadyRunned(llvm::Function *Function, Args... str) {
    assert(checkInit() && "Not Init");
    if (!CachedFunctions.contains(Function))
      return false;
    CachedPassesType<Metric> &cachedFunction = CachedFunctions[Function];
    if (cachedFunction.find({str...}) == cachedFunction.end()) {
      return false;
    }
    return true;
  }

  auto &findMin(const llvm::Function *Function) const {
    assert(checkInit() && "Not Init");
    auto &cachedFunction = CachedFunctions.getOrInsertDefault(Function);
    assert(cachedFunction.size() > 0 &&
           "Getting the minimum of an empty vector");
    auto minimum = std::min_element(
        cachedFunction.begin(), cachedFunction.end(),
        [](auto &lft, auto &rgt) { return lft.second < rgt.second; });
    return *minimum;
  }

  const CachedPassesType<Metric> &
  getCachedMetrics(llvm::Function *Function) const {
    assert(checkInit() && "Not Init");
    auto &cachedFunction = CachedFunctions.at(Function);
    return cachedFunction;
  }

  bool neetToResetCounter() const { return !Scheduled.isEmpty(); }

  static LaviniumTracker &getTrackerInstace() {
    static LaviniumTracker<Metric> instance;
    return instance;
  }

  /*Start tracking of function*/
  void trackFunction(llvm::Function *Function) {
    if (static_cast<bool>(functionTracker)) {
      functionTracker->trackFunction(Function);
    }
  }
  /*Check if tracking a function*/
  bool isTrackingFunction(const llvm::Function *Function) {
    assert(checkInit() && "Not Init");
    assert(static_cast<bool>(functionTracker) && "Function Tracker not init\n");
    return functionTracker->isTrackingFunction(Function);
  }
  /*Restore the original Function*/
  void restoreOriginalFunction(llvm::Function *Function) {
    assert(checkInit() && "Not Init");
    assert(static_cast<bool>(functionTracker) && "Function Tracker not init\n");
    functionTracker->restoreOriginalFunction(Function);
  }
  /*Untrack a function a function*/
  void untrackFunction(llvm::Function *Function) {
    assert(checkInit() && "Not Init");
    assert(static_cast<bool>(functionTracker) && "Function Tracker not init\n");
    functionTracker->untrackFunction(Function);
  }

  bool isFunctionNoTracker() { return static_cast<bool>(functionTracker); }

  void run(llvm::Function *Function) {
    assert(checkInit() && "Not Init");
    assert(functionTracker->isTrackingFunction(Function) &&
           "Pass running on an untracked function");
    PassManager->run(Function, Scheduled.getIds());
  }

private:
  LaviniumTracker()
      : CachedFunctions(), Scheduled(), functionTracker(), PassManager(){};
  LaviniumTracker(const LaviniumTracker &) = delete;
  LaviniumTracker(LaviniumTracker &&) = delete;

  // Members
  CachedFunctionMetric<Metric> CachedFunctions;
  LaviniumScheduledPasses Scheduled;
  std::unique_ptr<FunctionTracker> functionTracker;
  std::unique_ptr<PassManagerWrapper> PassManager;
  template <typename ANY> friend LaviniumTracker<ANY> getInstace();
};

} // namespace Lavinium