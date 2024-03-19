#pragma once

#include "llvm/IR/Function.h"
#include "llvm/IR/LaviniumPassManagerWrapper.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/ValueMapper.h"

namespace Lavinium {

class PassManagerWrapperImpl : public PassManagerWrapper {
  llvm::PassBuilder passBuilder;
  llvm::FunctionAnalysisManager FAM;
  llvm::FunctionPassManager
  getFunctionPassManager(const std::vector<std::string> &Scheduled);

public:
  ~PassManagerWrapperImpl() override = default;
  PassManagerWrapperImpl();
  virtual void run(llvm::Function *Function,
                   const std::vector<std::string> &Scheduled) override;
};

} // namespace Lavinium