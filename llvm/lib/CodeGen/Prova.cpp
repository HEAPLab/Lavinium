//===-- BasicBlockSections.cpp ---=========--------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// BasicBlockSections implementation.
//
// The purpose of this pass is to assign sections to basic blocks when
// -fbasic-block-sections= option is used. Further, with profile information
// only the subset of basic blocks with profiles are placed in separate sections
// and the rest are grouped in a cold section. The exception handling blocks are
// treated specially to ensure they are all in one seciton.
//
// Basic Block Sections
// ====================
//
// With option, -fbasic-block-sections=list, every function may be split into
// clusters of basic blocks. Every cluster will be emitted into a separate
// section with its basic blocks sequenced in the given order. To get the
// optimized performance, the clusters must form an optimal BB layout for the
// function. We insert a symbol at the beginning of every cluster's section to
// allow the linker to reorder the sections in any arbitrary sequence. A global
// order of these sections would encapsulate the function layout.
// For example, consider the following clusters for a function foo (consisting
// of 6 basic blocks 0, 1, ..., 5).
//
// 0 2
// 1 3 5
//
// * Basic blocks 0 and 2 are placed in one section with symbol `foo`
//   referencing the beginning of this section.
// * Basic blocks 1, 3, 5 are placed in a separate section. A new symbol
//   `foo.__part.1` will reference the beginning of this section.
// * Basic block 4 (note that it is not referenced in the list) is placed in
//   one section, and a new symbol `foo.cold` will point to it.
//
// There are a couple of challenges to be addressed:
//
// 1. The last basic block of every cluster should not have any implicit
//    fallthrough to its next basic block, as it can be reordered by the linker.
//    The compiler should make these fallthroughs explicit by adding
//    unconditional jumps..
//
// 2. All inter-cluster branch targets would now need to be resolved by the
//    linker as they cannot be calculated during compile time. This is done
//    using static relocations. Further, the compiler tries to use short branch
//    instructions on some ISAs for small branch offsets. This is not possible
//    for inter-cluster branches as the offset is not determined at compile
//    time, and therefore, long branch instructions have to be used for those.
//
// 3. Debug Information (DebugInfo) and Call Frame Information (CFI) emission
//    needs special handling with basic block sections. DebugInfo needs to be
//    emitted with more relocations as basic block sections can break a
//    function into potentially several disjoint pieces, and CFI needs to be
//    emitted per cluster. This also bloats the object file and binary sizes.
//
// Basic Block Labels
// ==================
//
// With -fbasic-block-sections=labels, we encode the offsets of BB addresses of
// every function into the .llvm_bb_addr_map section. Along with the function
// symbols, this allows for mapping of virtual addresses in PMU profiles back to
// the corresponding basic blocks. This logic is implemented in AsmPrinter. This
// pass only assigns the BBSectionType of every function to ``labels``.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/CodeGen/BasicBlockSectionUtils.h"
#include "llvm/CodeGen/BasicBlockSectionsProfileReader.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/InitializePasses.h"
#include "llvm/Target/TargetMachine.h"
#include <optional>

using namespace llvm;

namespace {

class WCETEstimator : public MachineFunctionPass {
public:
  static char ID;

  WCETEstimator() : MachineFunctionPass(ID) {
    initializeWCETEstimatorPass(*PassRegistry::getPassRegistry());
  }

  StringRef getPassName() const override {
    return "WCET Estimator Analysis";
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override;

  /// Identify basic blocks that need separate sections and prepare to emit them
  /// accordingly.
  bool runOnMachineFunction(MachineFunction &MF) override;
};

} // end anonymous namespace

char WCETEstimator::ID = 0;

INITIALIZE_PASS_BEGIN(
    WCETEstimator, "wcet-estimate",
    "Estimates the WCET of some annotated functions.",
    false, false)
INITIALIZE_PASS_END(WCETEstimator, "wcet-estimate",
                    "Estimates the WCET of some annotated functions.",
                    false, false)

bool WCETEstimator::runOnMachineFunction(MachineFunction &MF) {
  for (auto &MBB : MF) {
    for (auto &MI : MBB) {
      errs() << MI << " " << MI.getOpcode() << "\n";
    }
  }

  return false;
}

void WCETEstimator::getAnalysisUsage(AnalysisUsage &AU) const {
  /* 
  AU.setPreservesAll();
  AU.addRequired<BasicBlockSectionsProfileReader>(); 
  */ // TODO are we preserving stuff?
  MachineFunctionPass::getAnalysisUsage(AU);
}

MachineFunctionPass *llvm::createWCETEstimatorPass() {
  return new WCETEstimator();
}
