

#include "LaviniumMachineInstCount.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include <cstdint>

using namespace Lavinium;
using namespace llvm;

char LaviniumMachineInstCount::ID = 0;
char &llvm::LaviniumMachineInstCountID = LaviniumMachineInstCount::ID;

INITIALIZE_PASS(LaviniumMachineInstCount, "LaviniumMachineInstCount",
                DEBUG_TYPE, false, true)

bool LaviniumMachineInstCount::runOnMachineFunction(llvm::MachineFunction &MF) {
  count = 0;
  for (auto &MB : MF) {
    for (auto &MI : MB) {
      count++;
    }
  }
  return false;
}
uint64_t LaviniumMachineInstCount::getValue() { return count; }

void LaviniumMachineInstCount::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.getPreservesAll();
  AU.addRequired<llvm::LoopInfoWrapperPass>();
  AU.addRequired<llvm::MachineModuleInfoWrapperPass>();
}

llvm::FunctionPass *llvm::createLaviniumMachineInstCount() {
  return new LaviniumMachineInstCount();
}