#pragma once

#include "llvm/IR/Function.h"
#include <optional>
#include <string>
#include <vector>

// clang-format off
// ╔═╗┌┬┐┬─┐┌─┐┌┬┐┌─┐┌─┐┬ ┬
// ╚═╗ │ ├┬┘├─┤ │ ├┤ │ ┬└┬┘
// ╚═╝ ┴ ┴└─┴ ┴ ┴ └─┘└─┘ ┴
// clang-format on

namespace Lavinium {

// Add to the list of passes
// List Pass Names HERE: ./llvm/lib/Passes/PassRegistry.def
class Strategy {
protected:
  Strategy() = default;
  Strategy(const Strategy &) = delete;
  Strategy(Strategy &&) = delete;

public:
  virtual ~Strategy() = default;
  virtual std::optional<std::vector<std::string>>
  suggestPasses(llvm::Function *) = 0;
  virtual std::vector<std::string> getFinal(llvm::Function *) = 0;
};

} // namespace Lavinium