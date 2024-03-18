#pragma once

#include "llvm/IR/Function.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/ValueMapper.h"

namespace Lavinium {

class PassManagerWrapper {
public:
  virtual void run(llvm::Function *Function,
                   const std::vector<std::string> &Scheduled) = 0;
};

} // namespace Lavinium