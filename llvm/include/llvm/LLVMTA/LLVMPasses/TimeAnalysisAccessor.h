
#pragma once

#include "LLVMPasses/MachineFunctionCollector.h"
#include <cassert>
#include <string>
namespace TimingAnalysisPass {

class AsmDumpAndCheckPass;
class LoopBoundInfoPass;
class StaticAddressProvider;
class DirectiveHeuristicsPass;

class TimingAnalysisAccessor {
public:
  static void setInstance(AsmDumpAndCheckPass *asmDump,
                          MachineFunctionCollector *MFC,
                          LoopBoundInfoPass *LBIP, StaticAddressProvider *SAP,
                          DirectiveHeuristicsPass *DHP,
                          std::string AnalysisEntryPoint) {
    TimingAnalysisAccessor *TAA = getInstance();
    TAA->asmDump = asmDump;
    TAA->MFC = MFC;
    TAA->LBIP = LBIP;
    TAA->SAP = SAP;
    TAA->DHP = DHP;
    TAA->AnalysisEntryPoint = AnalysisEntryPoint;
    TAA->valid = true;
  }

  static MachineFunction *getAnalysisEntryPoint() {
    TimingAnalysisAccessor *TAA = getInstance();
    auto *Res = TAA->MFC->getFunctionByName(TAA->AnalysisEntryPoint);
    assert(Res && "Invalid entry point specified");
    return Res;
  }

  static AsmDumpAndCheckPass *getAsmDump() {
    TimingAnalysisAccessor *TAA = getInstance();
    assert(TAA->valid &&
           "This function singleton is not initialized. Typically "
           "TimingAnalysisMain is in charge of initializing it");
    return TAA->asmDump;
  }

  static MachineFunctionCollector *getMachineFunctionCollector() {
    TimingAnalysisAccessor *TAA = getInstance();
    assert(TAA->valid &&
           "This function singleton is not initialized. Typically "
           "TimingAnalysisMain is in charge of initializing it");
    return TAA->MFC;
  }

  static LoopBoundInfoPass *getLoopBoundInfoPass() {
    TimingAnalysisAccessor *TAA = getInstance();
    assert(TAA->valid &&
           "This function singleton is not initialized. Typically "
           "TimingAnalysisMain is in charge of initializing it");
    return TAA->LBIP;
  }

  static StaticAddressProvider *getStaticAddressProvider() {
    TimingAnalysisAccessor *TAA = getInstance();
    assert(TAA->valid &&
           "This function singleton is not initialized. Typically "
           "TimingAnalysisMain is in charge of initializing it");
    return TAA->SAP;
  }

  static DirectiveHeuristicsPass *getDirectiveHeuristicsPass() {
    TimingAnalysisAccessor *TAA = getInstance();
    assert(TAA->valid &&
           "This function singleton is not initialized. Typically "
           "TimingAnalysisMain is in charge of initializing it");
    return TAA->DHP;
  }

  static void reset() {

    TimingAnalysisAccessor *TAA = getInstance();
    TAA->valid = false;
    TAA->asmDump = nullptr;
    TAA->MFC = nullptr;
    TAA->LBIP = nullptr;
    TAA->SAP = nullptr;
    TAA->DHP = nullptr;
    TAA->AnalysisEntryPoint.clear();
  }

private:
  static TimingAnalysisAccessor *getInstance() {
    static TimingAnalysisAccessor *data;
    if (data == nullptr) {
      data = new TimingAnalysisAccessor();
    }
    return data;
  };
  bool valid = false;
  AsmDumpAndCheckPass *asmDump = nullptr;
  MachineFunctionCollector *MFC = nullptr;
  LoopBoundInfoPass *LBIP = nullptr;
  StaticAddressProvider *SAP = nullptr;
  DirectiveHeuristicsPass *DHP = nullptr;
  std::string AnalysisEntryPoint;
};
} // namespace TimingAnalysisPass
