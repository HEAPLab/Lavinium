
#pragma once

#include "llvm/IR/Lavinium.h"
#include "llvm/IR/LaviniumScheduledPass.h"
#include "llvm/IR/LaviniumStrategy.h"
#include "llvm/IR/LaviniumTypes.h"
#include <algorithm>
#include <cstddef>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace Lavinium {

namespace {
using csiter = std::vector<std::string>::const_iterator;
using Sequence = std::vector<csiter>;

} // namespace

template <typename Metric> class StrategyGenetic : public Strategy<Metric> {

  // Contains a vector of const_iterators of strings. The strings are the
  // passes.
  std::vector<Sequence> StoredSequences;
  int Evaluated = 0;
  size_t Index = 0;

  std::random_device rd;
  std::mt19937 gen;

public:
  StrategyGenetic(const CachedPassesMetric<Metric> *cached)
      : Strategy<Metric>(cached), rd(), gen(rd()) {
    initialize();
  };

  std::optional<std::vector<std::string>> suggestPasses() override {
    this->logFile << "Genetic progress: " << (float)Evaluated / (float)GeneticSamples << std::endl;
    if (Evaluated >= GeneticSamples) {
      return std::nullopt;
    }

    if (Index >= StoredSequences.size()) {
      evolve();
    }

    Evaluated++;
    return materialize(StoredSequences[Index++]);
  }

  ~StrategyGenetic() override = default;

private:
  void initialize() {
    this->availablePasses.push_back("no-op-function");
    std::uniform_int_distribution<> PassDistribution =
        std::uniform_int_distribution<>(
            0, this->availablePasses.size() -
                   1); // used to extract the passes to select
    std::uniform_int_distribution<> SizeDistribution =
        std::uniform_int_distribution<>(
            0, SequenceLength); // used to extract the passes to select
    for (int i = 0; i < GeneticPool; i++) {
      int RandomSize = SizeDistribution(gen);
      std::vector<csiter> tmp;
      for (int j = 0; j < RandomSize; j++) {
        tmp.push_back(this->availablePasses.cbegin() + PassDistribution(gen));
      }
      for (int j = 0; j < SequenceLength - RandomSize; j++) {
        tmp.push_back(this->availablePasses.cbegin() +
                      (this->availablePasses.size() - 1));
      }
      StoredSequences.push_back(std::move(tmp));
    }
  }

  std::pair<Sequence, Sequence> split_merge(Sequence Lft, Sequence Rgt,
                                            size_t SplitPointIndex) {
    assert(Lft.size() > SplitPointIndex && "split after elements");
    assert(Rgt.size() > SplitPointIndex && "split after elements");
    std::vector<csiter> tmp{Lft.begin() + SplitPointIndex, Lft.end()};
    Lft.resize(SplitPointIndex);
    Lft.insert(Lft.end(), Rgt.begin() + SplitPointIndex, Rgt.end());
    Rgt.resize(SplitPointIndex);
    Rgt.insert(Rgt.end(), tmp.begin(), tmp.end());
    return {Lft, Rgt};
  }

  std::vector<std::string> materialize(Sequence &Seq) {
    std::vector<std::string> Tmp;
    for (auto &Pass : Seq) {
      Tmp.push_back(*Pass);
    }
    return Tmp;
  }

  bool cmpSequences(const Sequence &lft, const Sequence &rgt) const {
    LaviniumScheduledPasses l{lft};
    LaviniumScheduledPasses r{rgt};
    return this->cachedPassesMetric->find(l)->second <
           this->cachedPassesMetric->find(r)->second;
  }

  void evolve() {
    // Reset index restarting at the first Sample
    Index = 0;
    assert(GeneticPool % 4 == 0 && "SequenceLength has to be a multiple of 4");
    auto Sorter = [this](const Sequence &lft, const Sequence &rgt) {
      return this->cmpSequences(lft, rgt);
    };

    // Sort the sample according to the WCET and remove the bottom half
    std::vector<Sequence> tmp;
    std::sort(StoredSequences.begin(), StoredSequences.end(), Sorter);
    StoredSequences.resize(StoredSequences.size() / 2);

    // Crossover of the samples
    std::uniform_int_distribution<> RandomSequencePicker(
        0, StoredSequences.size() - 1); // Used to select a sequence randomly
    std::uniform_int_distribution<> RandomIndexPoint(
        0,
        SequenceLength - 1); // Used to select a split point and a gene to mutate
    for (int i = 0; i < StoredSequences.size() / 2; i++) {
      int F = RandomSequencePicker(gen);
      int S = RandomSequencePicker(gen);
      int SplitPoint = RandomIndexPoint(gen);
      std::pair<Sequence, Sequence> crossedover =
          split_merge(StoredSequences.at(F), StoredSequences.at(S), SplitPoint);
      tmp.push_back(std::move(crossedover.first));
      tmp.push_back(std::move(crossedover.second));
    }
    StoredSequences.insert(StoredSequences.end(), tmp.begin(), tmp.end());

    // Add 15% of mutation
    RandomSequencePicker =
        std::uniform_int_distribution<>(0, StoredSequences.size() - 1);
    std::uniform_int_distribution<> PassDistribution =
        std::uniform_int_distribution<>(
            0, this->availablePasses.size() -
                   1); // used to extract the passes to select
    int toMutate = SequenceLength * 0.15;
    for (int i = 0; i < toMutate; i++) {
      int randomindex = RandomIndexPoint(gen);
      auto &RS = StoredSequences.at(RandomSequencePicker(gen));
      RS.at(randomindex) =
          this->availablePasses.cbegin() + PassDistribution(gen); 
    }

  assert(StoredSequences.size() == GeneticPool);
    for (auto& stored : StoredSequences){
      assert(stored.size() == SequenceLength);
    }
  }
};

} // namespace Lavinium
