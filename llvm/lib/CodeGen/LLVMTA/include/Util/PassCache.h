#pragma once

#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
class PassCache {
public:
  static PassCache *getInstance();

  void storeMachineDominatorTree(llvm::MachineFunction *MF,
                                 llvm::MachineDominatorTree *MDT);
  llvm::MachineDominatorTree *
  getMachineDominatorTree(llvm::MachineFunction *MF);

  void storeMachineLoopInfo(llvm::MachineFunction *MF,
                            llvm::MachineLoopInfo *MLI);
  llvm::MachineLoopInfo *getMachineLoopInfo(llvm::MachineFunction *MF);

  void storeLoopInfo(llvm::Function *F, llvm::LoopInfoWrapperPass *LI);
  llvm::LoopInfoWrapperPass *getLoopInfo(llvm::Function *F);

  void storeSCEVPass(llvm::Function *F, llvm::ScalarEvolutionWrapperPass *SCEV);
  llvm::ScalarEvolutionWrapperPass *getSCEVPass(llvm::Function *F);

  void storeDominatorTreePass(llvm::Function *F,
                              llvm::DominatorTreeWrapperPass *SCEV);
  llvm::DominatorTreeWrapperPass *getDominatorTreePass(llvm::Function *F);

  void
  storeTargetLibraryInfoWrapperPass(llvm::Function *F,
                                    llvm::TargetLibraryInfoWrapperPass *TLI);
  llvm::TargetLibraryInfoWrapperPass *
  getTargetLibraryInfoWrapperPass(llvm::Function *F);

  void storeAssumptionCacheTracker(llvm::Function *F,
                                   llvm::AssumptionCacheTracker *AC);
  llvm::AssumptionCacheTracker *getAssumptionCacheTracker(llvm::Function *F);

  void reset();

private:
  PassCache();
  ~PassCache();
  llvm::SmallVector<
      std::pair<llvm::MachineFunction *, llvm::MachineDominatorTree *>, 10>
      MDTStorage;
  llvm::SmallVector<std::pair<llvm::MachineFunction *, llvm::MachineLoopInfo *>,
                    10>
      MLIStorage;
  llvm::SmallVector<std::pair<llvm::Function *, llvm::LoopInfoWrapperPass *>,
                    10>
      LIStorage;
  llvm::SmallVector<
      std::pair<llvm::Function *, llvm::ScalarEvolutionWrapperPass *>, 10>
      SCEVStorage;
  llvm::SmallVector<
      std::pair<llvm::Function *, llvm::DominatorTreeWrapperPass *>, 10>
      DTStorage;
  llvm::SmallVector<
      std::pair<llvm::Function *, llvm::TargetLibraryInfoWrapperPass *>, 10>
      TLIStorage;
    llvm::SmallVector<
      std::pair<llvm::Function *, llvm::AssumptionCacheTracker *>, 10>
      ASStorage;
};