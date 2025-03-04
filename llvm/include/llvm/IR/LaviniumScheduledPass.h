
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
namespace {
enum class OwnedData{
    TRUE,
    FALSE
};
}

class LaviniumScheduledPasses {
  std::vector<std::string> Ids; // Passes are identified by ID
  const std::vector<std::vector<std::string>::const_iterator>* CiterIds; // To allow store only constant iterator
  OwnedData ownedData = OwnedData::TRUE;





public:
  template <typename... Args>
  LaviniumScheduledPasses(Args &&...args) : Ids{std::forward<Args>(args)...}, CiterIds{nullptr} {}
  LaviniumScheduledPasses(const std::vector<std::vector<std::string>::const_iterator>& tmp) : Ids{}, CiterIds{&tmp}, ownedData(OwnedData::FALSE) {}
  LaviniumScheduledPasses() = default;
  LaviniumScheduledPasses(const LaviniumScheduledPasses &) = default;
  LaviniumScheduledPasses(LaviniumScheduledPasses &&) = default;
  LaviniumScheduledPasses &operator=(const LaviniumScheduledPasses &) = default;
  LaviniumScheduledPasses &operator=(LaviniumScheduledPasses &&) = default;

  template <typename... Args> void pushBack(Args &&...args) {
    (Ids.push_back(std::forward<Args>(args)), ...);
  }

void pop(unsigned int n){
  assert(ownedData == OwnedData::TRUE);
  assert(Ids.size() >= n);
  for (unsigned int i = 0 ; i< n; i++ ){
    Ids.pop_back();
  }
}

const std::string& at(size_t n) const{
    if(ownedData == OwnedData::FALSE){
      return *CiterIds->at(n);
    }
    return  Ids.at(n);
}

  auto size() const{
    if(ownedData == OwnedData::FALSE)
      return CiterIds->size();
    return Ids.size();
}

  bool operator==(const LaviniumScheduledPasses &rhs) const {
    if (this->size() != rhs.size())
      return false;
    auto size = this->size();

    for (size_t i = 0; i < size; i++) {
      if (this->at(i) != rhs.at(i)) {
        return false;
      }
    }
    return true;
  }

  auto &getIds() const { assert(owned() && "We have no data :("); return this->Ids; }
  auto &getCiterIds() const { return *this->CiterIds; }
  bool owned() const {return ownedData == OwnedData::TRUE;}

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



  bool isEmpty() const { return Ids.empty(); };
  void clear() { Ids.clear(); }
  explicit operator bool() const { return this->Ids.size() > 0; };
};
} // namespace Lavinium

template <> struct std::hash<Lavinium::LaviniumScheduledPasses> {
  std::size_t
  operator()(const Lavinium::LaviniumScheduledPasses &s) const noexcept {
    auto seed = 0;
    if(s.owned()){
    auto & ids = s.getIds();
    auto begin = ids.begin();
    auto end = ids.end();

    for (; begin < end; ++begin) {

      seed ^= std::hash<std::string>{}(*begin) + 0x9e3779b9 + (seed << 6) +
              (seed >> 2);
    }}else{
    for (auto &view : s.getCiterIds()) {
      seed ^= std::hash<std::string>{}(*view) + 0x9e3779b9 + (seed << 6) +
              (seed >> 2);
    }
    }
    return seed;
  }
};
