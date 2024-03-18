#pragma once
#include "llvm/CodeGen/MachineFunctionPass.h"

#define DEBUG_TYPE "LaviniumMachineInstCount 1"

namespace Lavinium {

class LaviniumMachineInstCount : public llvm::MachineFunctionPass {
private:
  uint64_t count = 0;

public:
  static char ID;
  bool runOnMachineFunction(llvm::MachineFunction &Fn) override;
  LaviniumMachineInstCount() : MachineFunctionPass(ID) {}
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  llvm::StringRef getPassName() const override {
    return "LaviniumMachineInstCount";
  }
  uint64_t getValue();
};

} // namespace Lavinium