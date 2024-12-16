
#include "Util/PassCache.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"

PassCache *PassCache::getInstance() {
  static PassCache *storage = nullptr;
  if (storage == nullptr) {
    storage = new PassCache();
  }
  return storage;
}

PassCache::PassCache()
    : MDTStorage(), MLIStorage(), LIStorage(), SCEVStorage() {}

void PassCache::storeMachineDominatorTree(llvm::MachineFunction *MF,
                                          llvm::MachineDominatorTree *MDT) {
  assert(MF && "nullptr");
  assert(MDT && "nullptr");
  MDTStorage.push_back(std::pair{MF, MDT});
}

llvm::MachineDominatorTree *
PassCache::getMachineDominatorTree(llvm::MachineFunction *MF) {
  for (auto [sMF, sMDT] : MDTStorage) {
    if (MF == sMF) {
      return sMDT;
    }
  }
  return nullptr;
}

void PassCache::storeMachineLoopInfo(llvm::MachineFunction *MF,
                                     llvm::MachineLoopInfo *MLI) {
  assert(MF && "nullptr");
  assert(MLI && "nullptr");
  MLIStorage.push_back(std::pair{MF, MLI});
}

llvm::MachineLoopInfo *
PassCache::getMachineLoopInfo(llvm::MachineFunction *MF) {
  for (auto [sMF, sMLI] : MLIStorage) {
    if (MF == sMF) {
      return sMLI;
    }
  }
  return nullptr;
}

void PassCache::storeLoopInfo(llvm::Function *F,
                              llvm::LoopInfoWrapperPass *LI) {
  assert(F && "nullptr");
  assert(LI && "nullptr");
  LIStorage.push_back(std::pair{F, LI});
}

llvm::LoopInfoWrapperPass *PassCache::getLoopInfo(llvm::Function *F) {
  for (auto [sF, sLI] : LIStorage) {
    if (F == sF) {
      return sLI;
    }
  }
  return nullptr;
}

void PassCache::storeSCEVPass(llvm::Function *F,
                              llvm::ScalarEvolutionWrapperPass *SCEV) {
  assert(F && "nullptr");
  assert(SCEV && "nullptr");
  SCEVStorage.push_back(std::pair{F, SCEV});
}

llvm::ScalarEvolutionWrapperPass *PassCache::getSCEVPass(llvm::Function *F) {
  for (auto [sF, sSCEV] : SCEVStorage) {
    if (F == sF) {
      return sSCEV;
    }
  }
  return nullptr;
}

void PassCache::storeDominatorTreePass(llvm::Function *F,
                                       llvm::DominatorTreeWrapperPass *DT) {
  assert(F && "nullptr");
  assert(DT && "nullptr");
  DTStorage.push_back(std::pair{F, DT});
}

llvm::DominatorTreeWrapperPass *
PassCache::getDominatorTreePass(llvm::Function *F) {
  for (auto [sF, DT] : DTStorage) {
    if (F == sF) {
      return DT;
    }
  }
  return nullptr;
}

void PassCache::storeTargetLibraryInfoWrapperPass(
    llvm::Function *F, llvm::TargetLibraryInfoWrapperPass *TLI) {
  assert(F && "nullptr");
  assert(TLI && "nullptr");
  TLIStorage.push_back(std::pair{F, TLI});
}

llvm::TargetLibraryInfoWrapperPass *
PassCache::getTargetLibraryInfoWrapperPass(llvm::Function *F) {
  for (auto [sF, TLI] : TLIStorage) {
    if (F == sF) {
      return TLI;
    }
  }
  return nullptr;
}

void PassCache::storeAssumptionCacheTracker(llvm::Function *F,
                                       llvm::AssumptionCacheTracker *AS) {
  assert(F && "nullptr");
  assert(AS && "nullptr");
  ASStorage.push_back(std::pair{F, AS});
}

llvm::AssumptionCacheTracker *
PassCache::getAssumptionCacheTracker(llvm::Function *F) {
  for (auto [sF, AS] : ASStorage) {
    if (F == sF) {
      return AS;
    }
  }
  return nullptr;
}

void PassCache::reset() {
  for (auto MLI : MLIStorage) {
    delete MLI.second;
  }
  MLIStorage.clear();

  for (auto LI : LIStorage) {
    delete LI.second;
  }
  LIStorage.clear();

  for (auto MDT : MDTStorage) {
    delete MDT.second;
  }
  MDTStorage.clear();

  for (auto SCEV : SCEVStorage) {
    delete SCEV.second;
  }
  SCEVStorage.clear();

  for (auto DT : DTStorage) {
    delete DT.second;
  }
  DTStorage.clear();

  for (auto TLI : TLIStorage) {
    delete TLI.second;
  }
  TLIStorage.clear();

  for (auto AS : ASStorage) {
    delete AS.second;
  }
  ASStorage.clear();
}