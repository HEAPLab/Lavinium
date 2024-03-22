
#include "LaviniumMachineInstCount.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionAliasAnalysis.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LaviniumTracker.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/Scalar/ConstantHoisting.h"
#include <cstdint>
#include <memory>
#include <unordered_map>

using namespace llvm;
using namespace Lavinium;

#define DEBUG_TYPE "LaviniumRescheduler 1"

namespace Lavinium {

class LaviniumRescheduler : public MachineFunctionPass {
public:
  static char ID;
  LaviniumRescheduler() : MachineFunctionPass(ID) {}
  bool runOnMachineFunction(MachineFunction &Fn) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  void ResetMF(MachineFunction &MF);
  bool trackCorrectlyInit(llvm::Function *);
  void printResult(llvm::Function *Function);
  StringRef getPassName() const override { return "LaviniumRescheduler"; }
};

} // namespace Lavinium

char LaviniumRescheduler::ID = 0;
char &llvm::LaviniumReschedulerID = LaviniumRescheduler::ID;

INITIALIZE_PASS_BEGIN(LaviniumRescheduler, "LaviniumRescheduler", DEBUG_TYPE,
                      false, false)
INITIALIZE_PASS_DEPENDENCY(LaviniumMachineInstCount)
INITIALIZE_PASS_END(LaviniumRescheduler, DEBUG_TYPE, "LaviniumRescheduler",
                    false, false)

void LaviniumRescheduler::ResetMF(MachineFunction &MF) {
  auto &Function = MF.getFunction();
  MF.reset();
  MachineModuleInfo &MMI = getAnalysis<MachineModuleInfoWrapperPass>().getMMI();
  MMI.deleteMachineFunctionFor(Function);
}

bool LaviniumRescheduler ::trackCorrectlyInit(llvm::Function *Function) {

  auto &Tracker = Lavinium::LaviniumTracker<uint64_t>::getTrackerInstace();
  assert(Tracker.checkInit());
  assert(Tracker.isTrackingFunction(Function));
  return true;
}

void LaviniumRescheduler::printResult(llvm::Function *Function) {
  auto &Tracker = Lavinium::LaviniumTracker<uint64_t>::getTrackerInstace();
  auto CachedMetrics = Tracker.getCachedMetrics(Function);
  llvm::dbgs() << "WCET Result of " << Function->getName() << '\n';

  for (auto &[Keys, CachedMetric] : CachedMetrics) {
    llvm::dbgs() << Keys.toString();
    llvm::dbgs() << ": ";
    llvm::dbgs() << CachedMetric << "\n";
  }
}

bool LaviniumRescheduler::runOnMachineFunction(MachineFunction &MF) {
  auto &WCETAnalysis = getAnalysis<Lavinium::LaviniumMachineInstCount>();

  auto *Function = &MF.getFunction();
  trackCorrectlyInit(Function);
  auto &Tracker = Lavinium::LaviniumTracker<uint64_t>::getTrackerInstace();

  // Store previus metric given by WCET passes
  auto Wcet = WCETAnalysis.getValue();
  Tracker.storeMetric(Function, Wcet);

  // Restore original Function to run different optimizations
  Tracker.restoreOriginalFunction(Function);

  // Add to the list of passes
  // List Pass Names HERE: ./llvm/lib/Passes/PassRegistry.def
  auto &Strategy = Tracker.getStrategy();
  std::optional<std::vector<std::string>> NextPasses = Strategy.suggestPasses();
  if (NextPasses) {
    for (auto Pass : *NextPasses) {
      Tracker.addToSchedule(Pass);
    }
    Tracker.run(Function);
    ResetMF(MF);
  } else {
    std::string FinalPass = Strategy.getFinal();
    Tracker.addToSchedule(FinalPass);
    Tracker.run(Function);
    Tracker.clearScheduled();
  }
}
return true;
}

void LaviniumRescheduler::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<MachineModuleInfoWrapperPass>();
  AU.addRequired<Lavinium::LaviniumMachineInstCount>();
}

FunctionPass *llvm::createLaviniumRescheduler() {
  return new LaviniumRescheduler();
}