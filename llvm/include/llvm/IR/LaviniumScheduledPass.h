
#pragma once
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/Debug.h"
#include <cstddef>
#include <string>
#include <vector>

/// clang-format off
/// ╦  ┌─┐┬  ┬┬┌┐┌┬┬ ┬┌┬┐╔═╗┌─┐┬ ┬┌─┐┌┬┐┬ ┬┬  ┌─┐┌┬┐╔═╗┌─┐┌─┐┌─┐
/// ║  ├─┤└┐┌┘││││││ ││││╚═╗│  ├─┤├┤  │││ ││  ├┤  ││╠═╝├─┤└─┐└─┐
/// ╩═╝┴ ┴ └┘ ┴┘└┘┴└─┘┴ ┴╚═╝└─┘┴ ┴└─┘─┴┘└─┘┴─┘└─┘─┴┘╩  ┴ ┴└─┘└─┘
/// clang-format on

namespace Lavinium {

class LaviniumScheduledPasses {
  std::vector<std::string> Ids; // Passes are identified by ID

public:
  template <typename... Args>
  LaviniumScheduledPasses(Args &&...args) : Ids{std::forward<Args>(args)...} {}
  LaviniumScheduledPasses() = default;
  LaviniumScheduledPasses(const LaviniumScheduledPasses &) = default;
  LaviniumScheduledPasses(LaviniumScheduledPasses &&) = default;
  LaviniumScheduledPasses &operator=(const LaviniumScheduledPasses &) = default;
  LaviniumScheduledPasses &operator=(LaviniumScheduledPasses &&) = default;

  template <typename... Args> void pushBack(Args &&...args) {
    (Ids.push_back(std::forward<Args>(args)), ...);
  }

  bool operator==(const LaviniumScheduledPasses &rhs) const {
    if (this->Ids.size() != rhs.Ids.size())
      return false;
    auto size = this->Ids.size();
    for (size_t i = 0; i < size; i++) {
      if (this->Ids[i] != rhs.Ids[i]) {
        return false;
      }
    }
    return true;
  }

  auto &getIds() const { return this->Ids; }
  std::string toString() const {
    std::string name;
    for (auto [i, Key] : llvm::enumerate(Ids)) {
      name += Key;
      if (i < Ids.size() - 1) {
        name += " - ";
      }
    }
    return name;
  };

  size_t size() const { return Ids.size(); }

  bool isEmpty() const { return Ids.empty(); };
  void clear() { Ids.clear(); }
  explicit operator bool() const { return this->Ids.size() > 0; };
};
} // namespace Lavinium

template <> struct std::hash<Lavinium::LaviniumScheduledPasses> {
  std::size_t
  operator()(const Lavinium::LaviniumScheduledPasses &s) const noexcept {

    auto seed = 0;
    for (auto &view : s.getIds()) {
      seed ^= std::hash<std::string>{}(view) + 0x9e3779b9 + (seed << 6) +
              (seed >> 2);
    }
    return seed;
  }
};
