#pragma once
#include "llvm/CodeGen/MachineFunctionPass.h"

namespace Lavinium {

class LaviniumAnalyzerReset : public llvm::MachineFunctionPass {
private:
  uint64_t count = 0;

public:
  static char ID;
  bool runOnMachineFunction(llvm::MachineFunction &Fn) override;
  LaviniumAnalyzerReset() : MachineFunctionPass(ID) {}
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  llvm::StringRef getPassName() const override {
    return "LaviniumAnalyzerReset";
  }
  uint64_t getValue();
};

} // namespace Lavinium