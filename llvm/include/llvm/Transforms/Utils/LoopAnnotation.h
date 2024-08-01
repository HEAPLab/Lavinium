#ifndef LLVM_TRANSFORMS_UTILS_LOOPANNOTATION_H
#define LLVM_TRANSFORMS_UTILS_LOOPANNOTATION_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class AssumptionCache;
class DominatorTree;
class Loop;
class LoopInfo;
class MemorySSAUpdater;
class ScalarEvolution;

/// This pass is responsible for loop annotation.
class LoopAnnotationPass : public PassInfoMixin<LoopAnnotationPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
};

} // end namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_LOOPSIMPLIFY_H