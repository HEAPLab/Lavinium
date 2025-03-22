#pragma once

#include "LaviniumFunctionTracker.h"
#include "../lib/CodeGen/LaviniumFunctionTrackerImpl.h"
#include "../lib/CodeGen/LaviniumPassManagerImpl.h"
#include "../lib/CodeGen/LaviniumStrategies/LaviniumStrategies.h"
#include "LaviniumScheduledPass.h"
#include "LaviniumStrategy.h"
#include "Lavinium.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LaviniumPassManagerWrapper.h"
#include "llvm/IR/LaviniumTypes.h"
#include "llvm/IR/OptBisect.h"
#include "llvm/Pass.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
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

  std::string LaviniumAnalysisEntryPt;

public:
  void Init(std::unique_ptr<FunctionTracker> FT,
            std::unique_ptr<PassManagerWrapper> PMW,
            std::unique_ptr<Strategy<Metric>> ST) {
    setFunctionTracker(std::move(FT));
    setPassManagerWrapper(std::move(PMW));
    setStrategy(std::move(ST));
  }

  void setStrategy(std::unique_ptr<Strategy<Metric>> &&ST) { AppliedStrategy.swap(ST); }

  void setLaviniumAnalysisEntryPt(std::string name) { LaviniumAnalysisEntryPt = std::move(name); }

  const std::string &getLaviniumAnalysisEntryPt() { return LaviniumAnalysisEntryPt; }

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

  Strategy<Metric> &getStrategy() const { return *AppliedStrategy; }

  // Return if the store has overritten an already present entry
  bool storeMetric(Metric &M) {
    assert(checkInit() && "Not Init");
    
    if (Scheduled.isEmpty()) {
      Scheduled = LaviniumScheduledPasses("baseline");
    }
    else {
      Scheduled.pop(3);
    }
    CachedFunctions.insert(
        std::pair{std::move(Scheduled), M});

    
    return true;

  }

  // Retrive the stored Metrics for a functions
  const CachedPassesMetric<Metric> &
  getCachedMetrics() const {
    return CachedFunctions;
  }

  // Check if a pass is scheduled
  bool needToResetCounter() const { return !Scheduled.isEmpty(); }

  // Get Instance of the tracker
  static LaviniumTracker &getTrackerInstace() {
    static LaviniumTracker<Metric> instance;
    return instance;
  }

  // Get Instance of the tracker initializing it to 
  template <class StrategyType>
  static LaviniumTracker &GetTrackerInstanceAndInit() {
    auto &Tracker = LaviniumTracker<Metric>::getTrackerInstace();
    auto &cached = Tracker.getCachedFunctions();

    // if it 
    if (Tracker.checkInit()) {
      cached.clear();
      Tracker.setStrategy(std::move(std::make_unique<StrategyType>(&cached)));
    }
    else {
      // initialize the tracker passing it the 
      Tracker.Init(std::make_unique<FunctionTrackerImpl>(),
                    std::make_unique<PassManagerWrapperImpl>(),
                    std::make_unique<StrategyType>(&cached));
    }

    return Tracker;
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

  auto getFunctionToAnalyze(llvm::Module &Md) {
    // initialize if empty
    if (FunctionsToAnalyze.size() == 0) {
      for (llvm::Function &F : Md) {
        // check whether the function is a cloned one (or a declaration), if not, it is an original and we push it
        if (!isClonedFunction(&F) && !F.isDeclaration()) {
          FunctionsToAnalyze.push_back(&F);
        }
      }
    }

    if (CurrentFunction == nullptr) {
      assert(FunctionsToAnalyze.size() > 0); // we cannot have a module with no functions
      CurrentFunction = FunctionsToAnalyze[0];
    }

    return CurrentFunction;
  }

  llvm::Function *incrementCurrFunction() {
    auto i = std::find(FunctionsToAnalyze.begin(), FunctionsToAnalyze.end(), CurrentFunction);
    assert(i != FunctionsToAnalyze.end() && "Cannot find function in the list of functions to analyze");
    if (i+1 != FunctionsToAnalyze.end()) {
      CurrentFunction = *(i+1);
    }
    else CurrentFunction = nullptr;

    return CurrentFunction;
  }

  std::vector<std::string> findOptimizedSequence() {
    auto best = CachedFunctions.begin();

    for (auto elem = CachedFunctions.begin(); elem != CachedFunctions.end(); ++elem) {
      if ((elem->second < best->second) || 
          (elem->second == best->second && elem->first.size() < best->first.size()

           )) {
        best = elem;
      }
    }
     

    return best->first.getIds();
  }

  void reinstantiateStrategy() {
    
  }

private:
  LaviniumTracker()
      : CachedFunctions(), Scheduled(), functionTracker(), PassManager(){};
  LaviniumTracker(const LaviniumTracker &) = delete;
  LaviniumTracker(LaviniumTracker &&) = delete;

  // Members
  CachedPassesMetric<Metric> CachedFunctions;
  LaviniumScheduledPasses Scheduled;
  std::unique_ptr<FunctionTracker> functionTracker;
  std::unique_ptr<PassManagerWrapper> PassManager;
  std::unique_ptr<Strategy<Metric>> AppliedStrategy;
  template <typename ANY> friend LaviniumTracker<ANY> getInstace();
  std::vector<llvm::Function*> FunctionsToAnalyze;
  llvm::Function *CurrentFunction = nullptr;
};

struct LaviniumTrackerInitializer {
  static void InitTracker() {
    // add here the new strategies:
    const static std::map<std::string, std::function<void ()>> ExpToStrategy = {
      {"cartesian", [](){return &LaviniumTracker<uint64_t>::GetTrackerInstanceAndInit<StrategyCartesian<uint64_t>>();}},
      {"greedy", [](){return &LaviniumTracker<uint64_t>::GetTrackerInstanceAndInit<StrategyGreedy<uint64_t>>();}},
      {"genetic", [](){return &LaviniumTracker<uint64_t>::GetTrackerInstanceAndInit<StrategyGenetic<uint64_t>>();}},
      {"association", [](){return &LaviniumTracker<uint64_t>::GetTrackerInstanceAndInit<StrategyAssociation>();}},
      {"random", [](){return &LaviniumTracker<uint64_t>::GetTrackerInstanceAndInit<StrategyRandom>();}},
      {"cartesian-pruned", [](){return &LaviniumTracker<uint64_t>::GetTrackerInstanceAndInit<StrategyCartesianPruned<uint64_t>>();}},
      {"none", [](){return &LaviniumTracker<uint64_t>::GetTrackerInstanceAndInit<StrategyNone>();}},
    };

    // call the strategy initializer
    ExpToStrategy.at(LaviniumStrategyName)();
  }
};

} // namespace Lavinium
