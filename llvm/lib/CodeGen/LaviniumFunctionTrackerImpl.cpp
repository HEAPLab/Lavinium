#include "LaviniumFunctionTrackerImpl.h"
#include "llvm/ADT/STLExtras.h"

namespace Lavinium {

// Store a copy of a original function to be able to restore for later uses
void FunctionTrackerImpl::trackFunction(llvm::Function *Function) {

  assert( ClonedPair.count(Function) == 0 && "Store multiple times a function");

  llvm::ValueToValueMapTy VM;
  auto *ClonedFunction = llvm::CloneFunction(Function, VM);
  ClonedPair.insert(std::pair{Function, ClonedFunction});
}

bool FunctionTrackerImpl::isTrackingFunction(const llvm::Function *lft) {
  return ClonedPair.count(lft) == 1;
}


bool FunctionTrackerImpl::isClonedFunction(const llvm::Function *lft) {
  for (auto [Function, Cloned] : ClonedPair){
    if (lft == Cloned)
      return true;
  }
  return false;
}


void FunctionTrackerImpl::restoreOriginalFunction(llvm::Function *Function) {
  assert(ClonedPair.count(Function) == 1  &&      "Restoring a not saved Function");
  auto *ClonedFunction = ClonedPair.at(Function);
  llvm::dbgs() << "Cloning function " << ClonedFunction->getName() << " into " << Function->getName() << "\n";
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
  assert(ClonedPair.count(Function) == 1  &&      
         "Release a not saved Function");

  auto *ClonedFunction = ClonedPair.at(Function);
  ClonedFunction->deleteBody();
  ClonedFunction->eraseFromParent();
  ClonedPair = {};
}

} // namespace Lavinium
