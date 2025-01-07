//===- LoopSimplify.cpp - Loop Canonicalization Pass ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This pass performs several transformations to transform natural loops into a
// simpler form, which makes subsequent analyses and transformations simpler and
// more effective.
//
// Loop pre-header insertion guarantees that there is a single, non-critical
// entry edge from outside of the loop to the loop header.  This simplifies a
// number of analyses and transformations, such as LICM.
//
// Loop exit-block insertion guarantees that all exit blocks from the loop
// (blocks which are outside of the loop that have predecessors inside of the
// loop) only have predecessors from inside of the loop (and are thus dominated
// by the loop header).  This simplifies transformations such as store-sinking
// that are built into LICM.
//
// This pass also guarantees that loops will have exactly one backedge.
//
// Indirectbr instructions introduce several complications. If the loop
// contains or is entered by an indirectbr instruction, it may not be possible
// to transform the loop and make these guarantees. Client code should check
// that these conditions are true before relying on them.
//
// Similar complications arise from callbr instructions, particularly in
// asm-goto where blockaddress expressions are used.
//
// Note that the simplifycfg pass will clean up blocks which are split out but
// end up being unnecessary, so usage of this pass should not pessimize
// generated code.
//
// This pass obviously modifies the CFG, but updates loop information and
// dominator information.
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/LoopAnnotation.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/InstructionSimplify.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/MemorySSAUpdater.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionAliasAnalysis.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include <cstdlib>
#include <deque>
#include <system_error>
#include <tuple>

using namespace llvm;

#define DEBUG_TYPE "loop-annota"

llvm::cl::opt<std::string> LoopCounterFile{
    "loop-counter-file", cl::desc("Path for loop counter file"),
    cl::init("loop.lav")};

STATISTIC(NumAnnotation, "Number of annotated Loop");

bool isEmpty(int FD) {
  sys::fs::file_status Result;
  std::error_code err = sys::fs::status(FD, Result);
  if (err) {
    llvm_unreachable("No info about the file");
  }
  return Result.getSize() == 0;
}

int getFile(const Twine &P) {
  int ResultFD = 0;
  auto retError = llvm::sys::fs::openFileForReadWrite(
      P, ResultFD, llvm::sys::fs::CD_OpenAlways, llvm::sys::fs::OF_None);
  if (retError) {
    llvm_unreachable("Cannot open file");
  }
  return ResultFD;
}

bool isEmptyFile(int FD) {}

void closeFile(int FD) {
  auto retError = llvm::sys::fs::closeFile(FD);
  if (retError) {
    llvm_unreachable("Cannot close file");
  }
}

bool writeLoopToFile(Loop *L, DominatorTree *DT, LoopInfo *LI,
                     ScalarEvolution *SE, int FD) {

  auto *loopHeader = L->getHeader();
  loopHeader->getTerminator();
  raw_fd_ostream out(FD, false);
  int val = SE->getSmallConstantTripCount(L);

  if (val) {
    out << loopHeader->getParent()->getName() << " " << loopHeader->getName()
        << ": " << val << "\n";
  } else {
    out << loopHeader->getParent()->getName() << " " << loopHeader->getName()
        << ": " << "$VAL$" << "\n";
  }
  NumAnnotation++;
  return false;
}

void writeFunctionToFile(int FD, Module &M,
                         llvm::AnalysisManager<Function> &AM) {
  {
    raw_fd_ostream out(FD, false);
    out << "#Loop Lavinium File\n";
  }
  for (auto &F : M) {
    if (F.isDeclaration())
      continue;
    LoopInfo *LI = &AM.getResult<LoopAnalysis>(F);
    DominatorTree *DT = &AM.getResult<DominatorTreeAnalysis>(F);
    ScalarEvolution *SE = &AM.getResult<ScalarEvolutionAnalysis>(F);
    std::deque<Loop *> queue;
    for (auto *L : *LI) {
      queue.push_back(L);
    }
    while (!queue.empty()) {
      Loop *L = queue.front();
      for (auto *SL : *L) {
        queue.push_back(SL);
      }
      writeLoopToFile(L, DT, LI, SE, FD);
      queue.pop_front();
    }
  }
}

void readFileToMetadata(int FD, Module &M) {
  auto res = llvm::MemoryBuffer::getOpenFile(FD, LoopCounterFile, -1);
  if (!res) {
    llvm_unreachable("Cannot map file to memory");
  }
  auto mappedFile = std::move(*res);
  auto stringRef = mappedFile->getBuffer();
  auto iter = stringRef.split('\n');
  // Remove headline
  iter = iter.second.split('\n');
  auto &C = M.getContext();
  auto splitLine = [](StringRef ref) -> std::tuple<StringRef, StringRef, int> {
    auto tmp = ref.split(": ");
    auto [FName, BBName] = tmp.first.split(' ');
    int iterations = -1;
    tmp.second.consumeInteger(10, iterations);
    return {FName, BBName, iterations};
  };

  while (!iter.first.empty()) {

    auto [FName, BBName, iterations] = splitLine(iter.first);
    ;
    if (auto *F = dyn_cast<Function>(M.getNamedValue(FName))) {
      for (auto &BB : *F) {
        if (BB.getName().equals(BBName)) {
          auto *MD = MDNode::get(C, {ConstantAsMetadata::get(ConstantInt::get(
                                        Type::getInt32Ty(C), iterations))});
          for (auto &I : BB) {
            I.setMetadata("lavinium.iterloop", MD);
          }
        }
      }
    }
    if (iterations == -1) {
      llvm_unreachable("File cannot be parsed");
    }
    iter = iter.second.split('\n');
  }
}

PreservedAnalyses LoopAnnotationPass::run(Module &M,
                                          ModuleAnalysisManager &MAM) {
  auto &AM = MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();
  int FD = getFile(LoopCounterFile);

  if (isEmpty(FD)) {
    writeFunctionToFile(FD, M, AM);
    closeFile(FD);
    exit(0);
  } else {
    readFileToMetadata(FD, M);
  }

  closeFile(FD);
  return PreservedAnalyses::all();
}
