#include "llvm/IR/LaviniumFunctionTracker.h"

namespace Lavinium {

class FunctionTrackerImpl : public FunctionTracker {

  // Store a copy of a original function to be able to restore for later uses
  void trackFunction(llvm::Function *Function) override;

  bool isTrackingFunction(const llvm::Function *lft) override;

  void restoreOriginalFunction(llvm::Function *Function) override;

  void untrackFunction(llvm::Function *Function) override;
};
} // namespace Lavinium