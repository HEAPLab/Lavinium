#include "LaviniumFunctionTrackerImpl.h"
#include "llvm/ADT/STLExtras.h"

namespace Lavinium {

// Store a copy of a original function to be able to restore for later uses
void FunctionTrackerImpl::trackFunction(llvm::Function *Function) {
  assert(!ClonedPair.has_value() && "Store multiple times a function");

  llvm::ValueToValueMapTy VM;
  auto *ClonedFunction = llvm::CloneFunction(Function, VM);
  ClonedPair = std::pair{Function, ClonedFunction};
};

bool FunctionTrackerImpl::isTrackingFunction(const llvm::Function *lft) {
  if (!ClonedPair.has_value())
    return false;
  return ClonedPair->first == lft;
}

void FunctionTrackerImpl::restoreOriginalFunction(llvm::Function *Function) {
  assert(ClonedPair.has_value() && ClonedPair->first == Function &&
         "Restoring a not saved Function");
  auto *ClonedFunction = ClonedPair->second;
  Function->deleteBody();
  llvm::ValueToValueMapTy VM;
  llvm::SmallVector<llvm::ReturnInst *, 3> Ret;
  for (auto [ArgNew, ArgOld] :
       llvm::zip(Function->args(), ClonedFunction->args())) {
    VM[&ArgOld] = &ArgNew;
  }

  llvm::CloneFunctionInto(Function, ClonedFunction, VM,
                          llvm::CloneFunctionChangeType::LocalChangesOnly, Ret);
}

void FunctionTrackerImpl::untrackFunction(llvm::Function *Function) {
  assert(ClonedPair.has_value() && ClonedPair->first == Function &&
         "Release a not saved Function");

  auto *ClonedFunction = ClonedPair->second;
  ClonedFunction->deleteBody();
  ClonedFunction->eraseFromParent();
  ClonedPair = {};
}

} // namespace Lavinium