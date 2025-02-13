#pragma once

#include "LaviniumStrategyEnum.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/Sequence.h"
#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Lavinium.h"
#include "llvm/IR/LaviniumStrategy.h"
#include "llvm/IR/LaviniumTypes.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/LineIterator.h"
#include "llvm/Support/MemoryBuffer.h"
#include <any>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <list>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <format>

namespace Lavinium {

namespace {
class WeightForPass {
private:
  double weight = 1;
  double N_positive = 0;
  double N_neutral = 0;

public:
  void recordPositive() { N_positive++; }

  void recordNeutral() { N_neutral++; }

  void reset() {
    N_neutral = 0;
    N_positive = 0;
  }

  double updateWeight() {
    double denominator = (N_neutral < 1) ? 1 : N_neutral;
    weight = N_positive / denominator * 0.85 + 0.15;
    return weight;
  }

  double getWeight() { return weight; }

  void setWeight(double w) { weight = w; }
};

// Sequence definition

enum class CARACTER {
  P, // ositive
  N, // eutral
  B, // ad
};

struct match {
  size_t positionRef;
  size_t positionMatch;
  size_t length;

  bool IinRef(size_t Index) const {
    return Index >= positionRef && Index < positionRef + length;
  }

  size_t IconvRtoM(size_t Index) {
    assert(IinRef(Index) && "cannot convert an Index not in ref");
    return Index - positionRef + positionMatch;
  }

  size_t IconvMtoR(size_t Index) {
    assert(IinMatch(Index) && "cannot convert an Index not in match");
    return Index - positionMatch + positionRef;
  }

  bool IinMatch(size_t Index) const {
    return Index >= positionMatch && Index < positionMatch + length;
  }
};
using matches_t = std::vector<match>;

struct sequence {
  const std::vector<std::string> passes;
  const int wcet;
  std::vector<CARACTER> caracter;
  std::list<std::pair<sequence *, matches_t>> child;
  std::list<std::pair<sequence *, matches_t>> parent;

  size_t size() const { return passes.size(); }
  sequence(const std::vector<std::string> &passes, int wcet)
      : passes(passes), wcet(wcet), caracter(), child{}, parent{} {}
  sequence(std::vector<std::string> &&passes, int wcet)
      : passes(passes), wcet(wcet), caracter(), child{}, parent{} {}

  bool samePasses(const sequence &rgt) const { return rgt.passes == passes; }

  void unlinkChild(sequence *s) {
    child.remove_if([s](auto rgt) { return rgt.first == s; });
  }

  void unlinkParent(sequence *s) {
    parent.remove_if([s](auto rgt) { return rgt.first == s; });
  }
};
using sequences_t = std::vector<sequence>;
using psequences_t = std::vector<sequence *>;

matches_t findMaximumCommonSubSequence(sequence *ref, sequence *matcher) {
  assert(ref != nullptr && "Ref is a nullptr");
  assert(matcher != nullptr && "Ref is a nullptr");
  matches_t res;
  size_t refSize = ref->size();
  size_t matcherSize = matcher->size();
  size_t refI = 0;
  while (refI < refSize) {
    bool matching = false;
    size_t matchedLength = 0;
    size_t matI = 0;
    for (matI = 0; matI < matcherSize; matI++) {
      if ((refI + matchedLength < refSize) &&
          ref->passes[refI + matchedLength] == matcher->passes[matI]) {
        matching = true;
        matchedLength++;
        continue;
      }
      if (matching) {
        matching = false;
        size_t prevLength = res.size() ? res[0].length : 0;
        if (matchedLength > prevLength) {
          res.clear();
        }
        if (matchedLength >= prevLength) {
          res.push_back({refI, matI - matchedLength, matchedLength});
        }
        matI -= matchedLength;
        matchedLength = 0;
      }
    }

    if (matching) {
      size_t prevLength = res.size() ? res[0].length : 0;
      if (matchedLength > prevLength) {
        res.clear();
      }
      if (matchedLength >= prevLength) {
        res.push_back({refI, matI - matchedLength, matchedLength});
      }
    }
    refI += res.size() ? res[0].length : 1;
  }
  return res;
}

// Lattice definition

class Lattice {
  using sequenceLength_t = size_t;
  std::list<sequence> storage;
  std::unordered_map<sequenceLength_t, psequences_t> IndexLenSeq;
  sequenceLength_t maxSize = 0;
  int baseline = 0;
  std::unordered_map<std::string, std::string> namingMap;

  // LAVINIUM-TODO We always discard same length passes to avoid cyclic but in
  // theory we can have substring sharing between them
  std::vector<std::pair<sequence *, matches_t>>
  findLargestSubsequence(sequence *ref) {
    assert(ref != nullptr && "Sub sequence of a nullptr? are you mad?");
    std::vector<std::pair<sequence *, matches_t>> res;
    size_t size = ref->size() > 0 ? ref->size() - 1 : 0;
    for (size_t i = size; i > 0; --i) {
      if (res.size() > 0 && res[0].second[0].length > i) {
        return res;
      }
      for (sequence *matcher : IndexLenSeq[i]) {
        if (matcher == ref) {
          continue;
        }
        matches_t matches = findMaximumCommonSubSequence(ref, matcher);
        size_t matchesSize = matches.size();
        if (matchesSize) {

          if (res.size() == 0) {
            res.emplace_back(matcher, std::move(matches));
            continue;
          }
          size_t matchLength = matches[0].length;
          size_t resLength = res[0].second[0].length;

          if (matchLength > resLength) {
            res.clear();
          }

          if (matchLength >= resLength) {
            res.emplace_back(matcher, std::move(matches));
          }
        }
      }
    }
    return res;
  }

  void findAndUpdateChildren(sequence &s) {
    auto matches = findLargestSubsequence(&s);
    for (auto &match : matches) {
      match.first->parent.push_back({&s, match.second});
      s.child.push_back({match.first, std::move(match.second)});
    }
  }

  void findAndUpdateParent(sequence &matcher) {
    sequenceLength_t sSize = matcher.size();
    for (sequenceLength_t i = sSize + 1; i <= maxSize; i++) {
      for (sequence *ref : IndexLenSeq[i]) {
        auto match = findMaximumCommonSubSequence(ref, &matcher);
        size_t matchedLength = 0;
        size_t refLength = 0;
        if (match.size()) {
          matchedLength = match[0].length;
        }
        if (ref->child.begin() != ref->child.end()) {
          auto &childMatches = ref->child.front().second;
          if (childMatches.size()) {
            refLength = childMatches[0].length;
          }
        }
        if (matchedLength > refLength) {
          for (auto &c : ref->child) {
            c.first->unlinkParent(&matcher);
          }
          ref->child.clear();
        }
        if (matchedLength >= refLength) {
          matcher.parent.push_back({ref, match});
          ref->child.push_back({&matcher, std::move(match)});
        }
      }
    }
  }

  void caracterization(sequence &s) {
    std::vector<CARACTER> tmp;
    tmp.reserve(s.size());
    // No child
    if (!s.child.size()) {
      CARACTER car = CARACTER::N;
      if (baseline > s.wcet) {
        car = CARACTER::P;
      }
      if (baseline < s.wcet) {
        car = CARACTER::B;
      }
      for (size_t i = 0; i < s.passes.size(); i++)
        tmp.push_back(car);
    }

    // one child
    if (s.child.size() == 1) {
      auto &[child, matches] = *s.child.begin();
      CARACTER car = s.wcet > child->wcet
                         ? CARACTER::B
                         : (s.wcet < child->wcet ? CARACTER::P : CARACTER::N);
      tmp.resize(s.size(), car);
      for (size_t i = 0; i < s.passes.size(); i++) {
        auto findIter =
            std::find_if(matches.begin(), matches.end(),
                         [i](const match &m) { return m.IinRef(i); });
        if (findIter != matches.end()) {
          size_t childIndex = findIter->IconvRtoM(i);
          tmp[i] = child->caracter[childIndex];
        }
      }
    }

    // more children
    if (s.child.size() > 1) {
      auto &childrenList = s.child;
      // caracterization generic
      CARACTER car = CARACTER::P;
      for (auto &[child, _] : childrenList) {
        if (child->wcet < s.wcet) {
          car = CARACTER::B;
          break;
        }
        if (child->wcet == s.wcet) {
          car = CARACTER::N;
        }
      }
      tmp.resize(s.size(), car);

      for (size_t i = 0; i < s.passes.size(); i++) {
        size_t nN = 0;
        size_t nB = 0;
        size_t nP = 0;

        for (auto &[child, matches] : childrenList) {
          for (auto &match : matches) {
            if (match.IinRef(i)) {
              size_t childIndex = match.IconvRtoM(i);
              CARACTER childCar = child->caracter[childIndex];
              switch (childCar) {
              case CARACTER::N:
                nN++;
                break;
              case CARACTER::B:
                nB++;
                break;
              case CARACTER::P:
                nP++;
                break;
              }
            }
          }
        }

        if ((nB + nP + nN) == 0) {
          continue;
        }

        if (nB >= nN && nB >= nP) {
          car = CARACTER::B;
        } else if (nN >= nB && nN >= nP) {
          car = CARACTER::N;
        } else {
          car = CARACTER::P;
        }
        tmp[i] = car;
      }
    }

    if (tmp != s.caracter) {
      std::swap(tmp, s.caracter);
      for (auto &parent : s.parent) {
        caracterization(*parent.first);
      }
    }
  }

  void caracterization(std::vector<sequence *> &s) {
    std::sort(s.begin(), s.end(), [](sequence *lft, sequence *rgt) {
      return lft->size() < rgt->size();
    });
    for (sequence *elem : s) {
      caracterization(*elem);
    }
  }

  bool remove(std::list<sequence>::iterator rgt,
              bool rescheduleCaracterization) {
    sequence *pRgt = &*rgt;
    for (auto &child : rgt->child) {
      child.first->unlinkParent(pRgt);
    }
    for (auto &parent : rgt->parent) {
      parent.first->unlinkChild(pRgt);
      if (rescheduleCaracterization)
        caracterization(*parent.first);
    }
    auto &bucketSequence = IndexLenSeq.at(rgt->size());
    auto bucketIterElem =
        std::find(bucketSequence.begin(), bucketSequence.end(), &*rgt);
    assert(bucketIterElem != bucketSequence.end() &&
           "removing a not inserted element");
    auto bucketIterLast = bucketSequence.rbegin();
    std::iter_swap(bucketIterLast, bucketIterElem);
    bucketSequence.pop_back();
    storage.erase(rgt);
    return true;
  }

  void updateIndexLenSeq(sequence &s) {
    sequenceLength_t size = s.size();
    IndexLenSeq[size].push_back(&s);
    if (maxSize < size) {
      maxSize = size;
    }
  }

  bool removeIfPresent(sequence &rgt) {
    auto iter = std::find_if(
        storage.begin(), storage.end(),
        [&rgt](const sequence &lft) { return lft.samePasses(rgt); });
    if (iter != storage.end()) {
      return remove(iter, false);
    }
    return false;
  }

  bool isPresent(sequence &rgt) {
    auto iter = std::find_if(
        storage.begin(), storage.end(),
        [&rgt](const sequence &lft) { return lft.samePasses(rgt); });
    if (iter != storage.end()) {
      return true;
    }
    return false;
  }

public:
  void setBaseLine(int base) { baseline = base; }

  void setNaming(const std::vector<std::string> &availablePasses) {
    int size = availablePasses.size();
    const char *alphabeth = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if (size <= 26) {
      for (int i = 0; i < size; i++) {
        namingMap[availablePasses[i]] = alphabeth[i];
      }
    } else {
      int c = 1;
      for (int i = 0; i < size; i++) {
        namingMap[availablePasses[i]] = alphabeth[i % 26] + std::to_string(c);
        if (i % 26 == 0) {
          c++;
        }
      }
    }
  }

  void insert(std::vector<std::string> &&passes, int wcet) {
    assert(passes.size() > 0 && "empty passes");
    {
      sequence s{std::move(passes), wcet};
      if (isPresent(s)) {
        return;
      }
      storage.push_back(std::move(s));
    }
    {
      sequence &s = storage.back();
      findAndUpdateChildren(s);
      caracterization(s);
      updateIndexLenSeq(s);
    }
  }

  std::string to_dot() {
    static int fileC = 0;
    auto CarToStr = [](CARACTER c) -> char {
      switch (c) {
      case CARACTER::B:
        return '-';
        break;
      case CARACTER::N:
        return 'o';
        break;
      case CARACTER::P:
        return '+';
        break;
      }
    };

    std::string ret;
    ret += "digraph lattice {\n";
    ret += "layout=\"sfdp\"\n";
    ret += "overlap=prism\n";
    ret += "beautify=true\n";
    ret += "\n\n";
    std::unordered_map<sequence *, int> elements;
    sequenceLength_t count = 1;
    elements.reserve(storage.size());
    for (sequenceLength_t i = 0; i <= maxSize; i++) {
      auto &sequences = IndexLenSeq[i];
      for (sequence *s : sequences) {
        ret += std::to_string(count) + " ";
        elements.insert({s, count++});
        ret += "[label=<";
        int j = 0;
        for (auto &pass : s->passes) {
          ret += namingMap[pass];
          ret += "<SUP><FONT POINT-SIZE=\"6.0\">";
          ret += CarToStr(s->caracter[j]);
          ret += "</FONT></SUP>";
          j++;
        }
        ret += "<BR/>" + std::to_string(s->wcet);
        ret += ">]\n";
      }
    }

    ret += "\n\n";
    for (sequenceLength_t i = 0; i <= maxSize; i++) {
      auto &sequences = IndexLenSeq[i];
      for (sequence *s : sequences) {
        for (auto &parentMatch : s->parent) {
          sequence *parent = parentMatch.first;
          int sIndex = elements.at(s);
          int pIndex = elements.at(parent);
          ret +=
              std::to_string(sIndex) + " -> " + std::to_string(pIndex) + "\n";
        }
      }
    }

    ret += "}\n";
    std::ofstream tmpFile;
    tmpFile.open(std::to_string(fileC++) + ".dot");
    tmpFile << ret;
    tmpFile.close();
    return ret;
  }
  std::string to_csv() {
    static int fileC = 0;
    auto CarToStr = [](CARACTER c) -> char {
      switch (c) {
      case CARACTER::B:
        return '-';
        break;
      case CARACTER::N:
        return 'o';
        break;
      case CARACTER::P:
        return '+';
        break;
      }
    };

    std::string ret;
    ret += "Id,Label,WCET,RELATIVEWCET,length\n";
    std::unordered_map<sequence *, int> elements;
    sequenceLength_t count = 1;
    elements.reserve(storage.size());
    for (sequenceLength_t i = 0; i <= maxSize; i++) {
      auto &sequences = IndexLenSeq[i];
      for (sequence *s : sequences) {
        ret += std::to_string(count) + ",";
        elements.insert({s, count++});
        int j = 0;
        for (auto &pass : s->passes) {
          ret += namingMap[pass];
          ret += CarToStr(s->caracter[j]);
          j++;
        }
        ret +=  "," + std::to_string(s->wcet);
        char buff[50] = {0};
        int size = snprintf(buff, 50, ",%.16f", baseline/ (double) s->wcet );
        ret.append(buff, buff+size);
        ret +=  "," + std::to_string(s->size());
        ret += "\n";
      }
    }
    std::ofstream tmpFile;
    tmpFile.open(std::to_string(fileC) + "_n.csv");
    tmpFile << ret;
    tmpFile.close();
    ret = "";

    ret += "Source,Target,Type,Id,Label\n";
    int c = 0;
    for (sequenceLength_t i = 0; i <= maxSize; i++) {
      auto &sequences = IndexLenSeq[i];
      for (sequence *s : sequences) {
        for (auto &parentMatch : s->parent) {
          sequence *parent = parentMatch.first;
          int sIndex = elements.at(s);
          int pIndex = elements.at(parent);
          ret += std::to_string(sIndex) + "," + std::to_string(pIndex) + "," + "Directed," + std::to_string(c++) + ",\n";
        }
      }
    }

    tmpFile.open(std::to_string(fileC++) + "_e.csv");
    tmpFile << ret;
    tmpFile.close();
    return ret;
  }

  double updateWeight(std::unordered_map<std::string, WeightForPass> &WM) {

    for (auto &[_, WFP] : WM) {
      WFP.reset();
    }

    for (sequence &s : storage) {
      int i = 0;
      for (const std::string &pass : s.passes) {
        CARACTER car = s.caracter[i];
        if (car == CARACTER::N) {
          WM[pass].recordNeutral();
        }
        if (car == CARACTER::P) {
          WM[pass].recordPositive();
        }
        i++;
      }
    }

    double tot = 0;
    for (auto &[_, WFP] : WM) {
      tot += WFP.updateWeight();
    }
    return tot;
  }
};

} // namespace

class StrategyAssociation : public Strategy<uint64_t> {

  std::unordered_map<std::string, WeightForPass> weightsMap;
  int Generated;
  std::random_device rd;
  std::mt19937 gen;
  Lattice lattice;
  std::vector<std::string> lastGenerated;
  std::pair<std::string, uint64_t> cleanPassWcet = {"", 0};
  int cleanCycle;

  std::optional<std::vector<std::string>> correlation(uint64_t wcet) {

    std::vector<std::string> tmp;
    if (AssClean)
      if (cleanCycle < lastGenerated.size()) {
        if (cleanCycle == 0) {
          cleanPassWcet = {lastGenerated[0], wcet};
          lastGenerated.erase(lastGenerated.begin());
          cleanCycle++;
          return lastGenerated;
        }
        if (cleanPassWcet.second < wcet) {
          lastGenerated.insert(lastGenerated.begin() + cleanCycle,
                               std::move(cleanPassWcet.first));
        }
        cleanPassWcet = {lastGenerated[cleanCycle], wcet};
        lastGenerated.erase(lastGenerated.begin());
        cleanCycle++;
        return lastGenerated;
      }
    cleanCycle = 0;

    double totalWeight = lattice.updateWeight(weightsMap);
    if (Generated < AssSamples) {
      std::uniform_real_distribution<> PassDistribution =
          std::uniform_real_distribution<>(
              0, totalWeight); // used to extract the passes to select
      std::uniform_int_distribution<> SizeDistribution =
          std::uniform_int_distribution<>(
              1, SequenceLength); // used to extract the passes to select
      int RandomSize = SizeDistribution(gen);
      for (int j = 0; j < RandomSize; j++) {
        double accWeight = 0;
        double randomWeight = PassDistribution(gen);
        for (auto &WM : weightsMap) {
          if (accWeight <= randomWeight &&
              accWeight + WM.second.getWeight() > randomWeight) {
            tmp.push_back(WM.first);
            break;
          }
          if (randomWeight >= totalWeight) {
            tmp.push_back(WM.first);
            break;
          }
          accWeight += WM.second.getWeight();
        }
      }
      Generated++;
      lastGenerated = tmp;
      return tmp;
    }
    Generated++;
    return std::nullopt;
  }

  public:
    StrategyAssociation(const CachedPassesMetric<uint64_t> *cached)
        : Strategy<uint64_t>(cached), Generated(0), gen(rd()), lattice(),
          lastGenerated() {
      initialize();
      lattice.setNaming(this->availablePasses);
    };

    std::optional<std::vector<std::string>> suggestPasses() override {

      if (Generated == 0) {
        Generated++;
        lastGenerated = std::vector<std::string>{"no-op-function"};
        return std::vector<std::string>{"no-op-function"};
      }

      uint64_t wcet =
          this->cachedPassesMetric->at(LaviniumScheduledPasses(lastGenerated));

      if (Generated == 1) {
        lattice.setBaseLine(wcet);
      } else {
        if(!lastGenerated.empty())
        lattice.insert(std::move(lastGenerated), wcet);
      }
      /*lattice.to_dot();*/
      lattice.to_csv();

      std::vector<std::string> tmp;
      if (Generated < AssRulePool) {
        std::uniform_int_distribution<> PassDistribution =
            std::uniform_int_distribution<>(
                0, this->availablePasses.size() -
                       1); // used to extract the passes to select
        std::uniform_int_distribution<> SizeDistribution =
            std::uniform_int_distribution<>(
                1, SequenceLength); // used to extract the passes to select
        int RandomSize = SizeDistribution(gen);
        for (int j = 0; j < RandomSize; j++) {
          tmp.push_back(this->availablePasses[PassDistribution(gen)]);
        }
        Generated++;
        lastGenerated = tmp;
        return tmp;
      }
      return correlation(wcet);
    }

    ~StrategyAssociation() override = default;

  private:
    void initialize() {
      // Initialize all passes with default weight (1)
      for (const auto &pass : this->availablePasses) {
        weightsMap.insert({pass, WeightForPass{}});
      }
    }

    void NormalizeWeights() {
      double tot = 0;
      for (auto p : weightsMap) {
        tot += p.second.getWeight();
      }

      for (auto p : weightsMap) {
        p.second.setWeight(p.second.getWeight() / tot);
      }
    }
  };

} // namespace Lavinium
