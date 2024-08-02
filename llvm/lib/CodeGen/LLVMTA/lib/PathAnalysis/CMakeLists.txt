message(STATUS "Found Path Analysis LIB")

add_llvm_component_library(llvmtaPathAnalysis
FlowConstraintProvider.cpp
GetEdges.cpp
PathAnalysisLPSolve.cpp
PathAnalysisCPLEX.cpp
PathAnalysisGUROBI.cpp
LoopBoundInfo.cpp
PathAnalysis.cpp
Variable.cpp

DEPENDS LLVMAnalysis LLVMCodeGen LLVMCore LLVMSupport LLVMTarget

LINK_LIBS llvmtaLLVMPasses llvmtaUtil llvmtaAnalysisFramework llvmtaPartitionUtil llvmtaPreprocessing llvmtaMemory llvmtaMicroarchitecture llvmtaMicroarchitectureOutOfOrder llvmtacrpd

LINK_COMPONENTS 
    Support
)

