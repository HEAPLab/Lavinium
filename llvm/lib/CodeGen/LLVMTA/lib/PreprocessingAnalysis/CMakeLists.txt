message(STATUS "Found Preprocessing Analysis LIB")

add_llvm_component_library(llvmtaPreprocessing
AddressInformation.cpp
ConstantValueDomain.cpp
ConstantValueDomainARM.cpp
ConstantValueDomainRISCV.cpp
DEPENDS LLVMAnalysis LLVMCodeGen LLVMCore LLVMSupport LLVMTarget

LINK_LIBS llvmtaLLVMPasses llvmtaUtil llvmtaPathAnalysis llvmtaAnalysisFramework llvmtaPartitionUtil llvmtaMemory llvmtaMicroarchitecture llvmtaMicroarchitectureOutOfOrder llvmtacrpd

LINK_COMPONENTS 
    Support
    ARMCodeGen
    RISCVCodeGen
)

