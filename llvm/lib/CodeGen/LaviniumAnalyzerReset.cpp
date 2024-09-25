

#include "LaviniumAnalyzerReset.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/Dominators.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include <cstdint>

#define DEBUG_TYPE "LaviniumAnalyzer"

using namespace Lavinium;
using namespace llvm;

char LaviniumAnalyzerReset::ID = 0;
char &llvm::LaviniumAnalyzerResetID = LaviniumAnalyzerReset::ID;

INITIALIZE_PASS(LaviniumAnalyzerReset, "LaviniumAnalyzerReset", DEBUG_TYPE,
                false, true)

bool LaviniumAnalyzerReset::runOnMachineFunction(llvm::MachineFunction &MF) {
  getAnalysis<TargetLibraryInfoWrapperPass>().releaseMemory();
  getAnalysis<LoopInfoWrapperPass>().releaseMemory();
  getAnalysis<MachineModuleInfoWrapperPass>().releaseMemory();
  getAnalysis<AssumptionCacheTracker>().releaseMemory();
  getAnalysis<DominatorTreeWrapperPass>().releaseMemory();

  return false;
}
uint64_t LaviniumAnalyzerReset::getValue() { return count; }

void LaviniumAnalyzerReset::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequiredTransitive<LoopInfoWrapperPass>();
  AU.addRequiredTransitive<MachineModuleInfoWrapperPass>();
  AU.addRequiredTransitive<AssumptionCacheTracker>();
  AU.addRequiredTransitive<DominatorTreeWrapperPass>();
  AU.addRequiredTransitive<TargetLibraryInfoWrapperPass>();
}

llvm::FunctionPass *llvm::createLaviniumAnalyzerReset() {
  return new LaviniumAnalyzerReset();
}