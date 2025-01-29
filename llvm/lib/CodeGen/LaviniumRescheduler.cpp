
#include "LaviniumAnalyzerReset.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
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
#include "llvm/IR/Module.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Transforms/Scalar/ConstantHoisting.h"
#include <algorithm>
#include <cstdint>
#include <memory>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

using namespace llvm;
using namespace Lavinium;

#define DEBUG_TYPE "LaviniumRescheduler 1"

namespace Lavinium {
class LaviniumRescheduler : public MachineFunctionPass {
public:
  static char ID;
  LaviniumRescheduler()
      : MachineFunctionPass(ID),
        wcetExtractor(R"a(total ub="(\S+)" lb="(\S+)")a") {}
  unsigned long readWCET();
  bool runOnMachineFunction(MachineFunction &Fn) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  void ResetMF(MachineFunction &MF);
  bool trackCorrectlyInit(llvm::Function *);
  void printResult();
  StringRef getPassName() const override { return "LaviniumRescheduler"; }

private:
  std::regex wcetExtractor;
};

} // namespace Lavinium

char LaviniumRescheduler::ID = 0;
char &llvm::LaviniumReschedulerID = LaviniumRescheduler::ID;

INITIALIZE_PASS_BEGIN(LaviniumRescheduler, "LaviniumRescheduler", DEBUG_TYPE,
                      false, false)
INITIALIZE_PASS_DEPENDENCY(LaviniumAnalyzerReset)
INITIALIZE_PASS_END(LaviniumRescheduler, DEBUG_TYPE, "LaviniumRescheduler",
                    false, false)

void LaviniumRescheduler::ResetMF(MachineFunction &MF) {
  auto &Function = MF.getFunction();
  MF.reset();
  MachineModuleInfo &MMI = getAnalysis<MachineModuleInfoWrapperPass>().getMMI();
  MMI.deleteMachineFunctionForLavinium(Function);
  MMI.getOrCreateMachineFunction(Function);
}

bool LaviniumRescheduler ::trackCorrectlyInit(llvm::Function *Function) {

  auto &Tracker = Lavinium::LaviniumTracker<uint64_t>::getTrackerInstace();
  assert(Tracker.checkInit());
  assert(Tracker.isTrackingFunction(Function));
  return true;
}

void LaviniumRescheduler::printResult() {
  auto &Tracker = Lavinium::LaviniumTracker<uint64_t>::getTrackerInstace();
  auto CachedMetrics = Tracker.getCachedMetrics();
  llvm::dbgs() << "WCET Result\n";

  for (auto &[Keys, CachedMetric] : CachedMetrics) {
    llvm::dbgs() << Keys.toString();
    llvm::dbgs() << ": ";
    llvm::dbgs() << CachedMetric << "\n";
  }
}

unsigned long LaviniumRescheduler::readWCET() {
  auto res = llvm::MemoryBuffer::getFile("TotalBound.xml", true);
  if (!res) {
    llvm_unreachable("Cannot map file to memory");
  }
  auto mappedFile = std::move(*res);
  auto fileContent = mappedFile->getBuffer().str();
  std::smatch matches;
  auto success = std::regex_search(fileContent, matches, wcetExtractor);
  assert(success && "Cannot parse wcet file");
  unsigned long ret;
  auto ub = matches[1];
  auto s = StringRef{&*ub.first,
                     static_cast<size_t>(std::distance(ub.first, ub.second))};

  success = !s.getAsInteger(10, ret);
  if (!success) {
    return -1;
  }
  return ret;
}

bool LaviniumRescheduler::runOnMachineFunction(MachineFunction &MF) {
  getAnalysis<Lavinium::LaviniumAnalyzerReset>();
  llvm::Module &M = *MF.getFunction().getParent();
  MachineModuleInfo &MMI = getAnalysis<MachineModuleInfoWrapperPass>().getMMI();

  auto *Function = &MF.getFunction();
  if (Function->getName() != "main")
    return false;

  trackCorrectlyInit(Function);
  auto &Tracker = Lavinium::LaviniumTracker<uint64_t>::getTrackerInstace();

  // LAVINIUM-TODO read the file
  unsigned long x = readWCET();
  Tracker.storeMetric(x);

  // Restore original Function to run different optimizations
  for (auto &F : M) {
    if (Tracker.isClonedFunction(&F) || F.isDeclaration())
      continue;
    Tracker.restoreOriginalFunction(&F);
  }

  // Add to the list of passes
  // List Pass Names HERE: ./llvm/lib/Passes/PassRegistry.def
  auto &Strategy = Tracker.getStrategy();
  std::optional<std::vector<std::string>> NextPasses =
      Strategy.suggestPasses();
  if (NextPasses) {
    llvm::dbgs() << "Scheduled Passes:\n";
    std::string removeMe = "";
    for (auto Pass : *NextPasses) {
      Tracker.addToSchedule(Pass);
      llvm::dbgs() << "Pass:\t" << Pass << "\n";
      removeMe += Pass;
    }
    Tracker.addToSchedule("mem2reg");
    Tracker.addToSchedule("loop-simplify");
    Tracker.addToSchedule("simplifycfg");
    for (auto &F : M) {
      if (Tracker.isClonedFunction(&F) || F.isDeclaration())
        continue;
      MachineFunction &MF = MMI.getOrCreateMachineFunction(F);
      Tracker.run(&F);
      ResetMF(MF);
    }
  } else {
    //LAVINIUM-TODO create real binary at end
    /*std::vector<std::string> FinalPass = Strategy.getFinal();*/
    /*for (auto Pass : FinalPass) {*/
    /*  Tracker.addToSchedule(Pass);*/
    /*}*/
    /*for (auto &F : M) {*/
    /*  if (Tracker.isClonedFunction(&F) || F.isDeclaration())*/
    /*    continue;*/
    /*  Tracker.run(&F);*/
    /*}*/

    Tracker.clearScheduled();
    printResult();
    _Exit(0);
  }
  return true;
}

void LaviniumRescheduler::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<MachineModuleInfoWrapperPass>();
  AU.addRequired<Lavinium::LaviniumAnalyzerReset>();
}

FunctionPass *llvm::createLaviniumRescheduler() {
  return new LaviniumRescheduler();
}
