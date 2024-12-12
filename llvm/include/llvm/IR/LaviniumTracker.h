#pragma once

#include "LaviniumFunctionTracker.h"
#include "LaviniumScheduledPass.h"
#include "LaviniumStrategy.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LaviniumPassManagerWrapper.h"
#include "llvm/IR/LaviniumTypes.h"
#include "llvm/IR/OptBisect.h"
#include "llvm/Pass.h"
#include <algorithm>
#include <cassert>
#include <memory>
#include <utility>

// clang-format off
// ╦  ┌─┐┬  ┬┬┌┐┌┬┬ ┬┌┬┐╔╦╗┬─┐┌─┐┌─┐┬┌─┌─┐┬─┐
// ║  ├─┤└┐┌┘││││││ ││││ ║ ├┬┘├─┤│  ├┴┐├┤ ├┬┘
// ╩═╝┴ ┴ └┘ ┴┘└┘┴└─┘┴ ┴ ╩ ┴└─┴ ┴└─┘┴ ┴└─┘┴└─
// clang-format on

namespace Lavinium {

template <typename Metric> class LaviniumTracker {

private:
  void setFunctionTracker(std::unique_ptr<FunctionTracker> &&FT) {
    functionTracker.swap(FT);
  }

  void setPassManagerWrapper(std::unique_ptr<PassManagerWrapper> &&PMW) {
    PassManager.swap(PMW);
  }

  void setStrategy(std::unique_ptr<Strategy> &&ST) { AppliedStrategy.swap(ST); }

public:
  void Init(std::unique_ptr<FunctionTracker> FT,
            std::unique_ptr<PassManagerWrapper> PMW,
            std::unique_ptr<Strategy> ST) {
    setFunctionTracker(std::move(FT));
    setPassManagerWrapper(std::move(PMW));
    setStrategy(std::move(ST));
  }

  bool checkInit() const {
    return static_cast<bool>(AppliedStrategy) &&
           static_cast<bool>(functionTracker) && static_cast<bool>(PassManager);
  }

  template <typename... Args> void addToSchedule(Args &&...args) {
    assert(checkInit() && ("Call   before init"));
    Scheduled.pushBack(std::forward<Args>(args)...);
  }

  // Clear scheduled pass
  void clearScheduled() { Scheduled.clear(); }

  // Return a reference to the scheduled pass
  auto &getScheduled() const { return Scheduled; }

  Strategy &getStrategy() const { return *AppliedStrategy; }

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

  // Retrive the stored Metrics for a functions
  const CachedPassesType<Metric> &
  getCachedMetrics(llvm::Function *Function) const {
    assert(checkInit() && "Not Init");
    auto &cachedFunction = CachedFunctions.at(Function);
    return cachedFunction;
  }

  // Check if a pass is scheduled
  bool needToResetCounter() const { return !Scheduled.isEmpty(); }

  // Get Instance of the tracker
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

    /*Untrack a function a function*/
  bool isClonedFunction(llvm::Function *Function) {
    assert(checkInit() && "Not Init");
    assert(static_cast<bool>(functionTracker) && "Function Tracker not init\n");
    return functionTracker->isClonedFunction(Function);
  }

  // Run scheduled pass on the function
  void run(llvm::Function *Function) {
    assert(checkInit() && "Not Init");
    assert(functionTracker->isTrackingFunction(Function) &&
           "Pass running on an untracked function");
    PassManager->run(Function, Scheduled.getIds());
  }

  auto &getCachedFunctions() { return CachedFunctions; }

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
  std::unique_ptr<Strategy> AppliedStrategy;
  template <typename ANY> friend LaviniumTracker<ANY> getInstace();
};

} // namespace Lavinium