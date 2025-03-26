#include "llvm/IR/LaviniumCurrentFunctionGetter.h"
#include <cstdint>

#include "llvm/IR/LaviniumTracker.h"

std::string getFunctionNameToAnalyze(){
      auto &Tracker = Lavinium::LaviniumTracker<uint64_t>::getTrackerInstace();
      std::string functionName = Tracker.getFunctionToAnalyze()->getName().str();
      return functionName;
}
