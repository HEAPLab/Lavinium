#pragma once

#include "llvm/IR/Function.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/ValueMapper.h"

namespace Lavinium {

class FunctionTracker {
public:
  // Store a copy of a original function to be able to restore for later uses
  virtual void trackFunction(llvm::Function *Function) = 0;
  virtual bool isTrackingFunction(const llvm::Function *lft) = 0;
  virtual void restoreOriginalFunction(llvm::Function *Function) = 0;
  virtual void untrackFunction(llvm::Function *Function) = 0;
  virtual ~FunctionTracker() = default;

protected:
  std::optional<std::pair<llvm::Function *, llvm::Function *>> ClonedPair;
};

} // namespace Lavinium