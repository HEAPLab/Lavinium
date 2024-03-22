#pragma once

#include "LaviniumStrategy.h"
#include "LaviniumTypes.h"
#include "llvm/IR/Function.h"
#include <optional>

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
  Strategy() = delete;
  Strategy(const Strategy &) = delete;
  Strategy(Strategy &&) = delete;

public:
  virtual ~Strategy() = default;
  virtual std::optional<std::vector<std::string>> suggestPasses() = 0;
};

} // namespace Lavinium