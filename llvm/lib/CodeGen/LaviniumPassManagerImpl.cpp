#include "LaviniumPassManagerImpl.h"
namespace Lavinium {

PassManagerWrapperImpl::PassManagerWrapperImpl() {
  passBuilder.registerModuleAnalyses(MAM);
  passBuilder.registerCGSCCAnalyses(CGAM);
  passBuilder.registerFunctionAnalyses(FAM);
  passBuilder.registerLoopAnalyses(LAM);
  passBuilder.crossRegisterProxies(LAM, FAM, CGAM, MAM);
}

void PassManagerWrapperImpl::run(llvm::Function *Function,
                                 const std::vector<std::string> &Scheduled) {
    auto FPM = this->getFunctionPassManager(Scheduled);
  FPM.run(*Function, FAM);
}

llvm::FunctionPassManager PassManagerWrapperImpl::getFunctionPassManager(
    const std::vector<std::string> &Scheduled) {
  llvm::FunctionPassManager FPM;
  FAM.clear();
  CGAM.clear();
  MAM.clear();
  LAM.clear();
  std::string PassPipeline = "";
  auto size = Scheduled.size();
  for (size_t i = 0; i < size; i++) {
    PassPipeline += Scheduled[i];
    if (i < size - 1) {
      PassPipeline.push_back(',');
    }
  }
  auto error = passBuilder.parsePassPipeline(FPM, PassPipeline);
  if (error) {
    llvm::dbgs() << "Crashato con PassPipeline:" << PassPipeline << "\n";
  }
  return FPM;
}

} // namespace Lavinium