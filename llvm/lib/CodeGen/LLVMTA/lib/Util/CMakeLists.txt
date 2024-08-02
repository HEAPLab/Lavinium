message(STATUS "Found Util LIB")

add_llvm_component_library(llvmtaUtil
Graph.cpp
Options.cpp
PersistenceScope.cpp
Statistics.cpp
Util.cpp
UtilPathAnalysis.cpp
Vertex.cpp
AbstractAddress.cpp
DEPENDS LLVMAnalysis LLVMCodeGen LLVMCore LLVMSupport LLVMTarget

LINK_LIBS llvmtaLLVMPasses llvmtaPathAnalysis llvmtaAnalysisFramework llvmtaPartitionUtil llvmtaPreprocessing llvmtaMemory llvmtaMicroarchitecture llvmtaMicroarchitectureOutOfOrder llvmtacrpd

LINK_COMPONENTS
    Support
)

