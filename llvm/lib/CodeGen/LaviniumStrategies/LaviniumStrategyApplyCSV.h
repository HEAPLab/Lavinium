
#pragma once

#include "llvm/IR/Lavinium.h"
#include "llvm/IR/LaviniumStrategy.h"
#include "llvm/IR/LaviniumTypes.h"
#include "llvm/LLVMTA/LLVMPasses/TimeHelper.h"
#include <algorithm>
#include <climits>
#include <cstdint>
#include <fstream>
#include <optional>
#include <random>
#include <string>
#include <vector>
#include <chrono>
#include <fstream>
#include "llvm/IR/LaviniumCurrentFunctionGetter.h"

namespace Lavinium {

class StrategyApplyCSV : public Strategy<uint64_t> {

  int Generated;
  std::vector<std::string> fileContent;
  bool runned = false;


public:
  StrategyApplyCSV(const CachedPassesMetric<uint64_t> *cached)
      : Strategy<uint64_t>(cached)  {
        std::fstream file = std::fstream(CSVLaviniumFile);
        std::string buff;
        while(std::getline(file, buff)){
          fileContent.push_back(buff);
        }
        file.close();
        fileContent.erase(fileContent.begin());

      }

  std::vector<std::string> parsePassFromString(std::string pass){
    std::vector<std::string> ret;
    while(pass.find(" - ") != std::string::npos ){
      pass.replace(pass.find(" - "), 3, ",");
    }
    ret.push_back(pass);
    return  ret;
  }


 std::string getPass(const std::string& functionName) {
   int minWCET = INT_MAX;
   std::string ret;
   for(std::string& line : fileContent){
    auto  indexBenchmark = line.find_first_of('/') + 1;
    if (line.substr(indexBenchmark).find(BenchMarkName) != 0)
      continue;
    
    auto firstQuote = line.find_first_of('"');
    auto firstCommaInPass = line.substr(firstQuote).find_first_of(',') + firstQuote;
    //Skip funcions

    std::string currentFunctionName = line.substr(firstQuote+3, firstCommaInPass - (firstQuote + 3));
    if(currentFunctionName != functionName)
      continue;

    // Extract WCET
    auto secondQuote = line.substr(firstQuote+1).find_first_of('"') + firstQuote+1;

    auto WCET = std::atoi(line.substr(secondQuote+2).c_str());
    llvm::dbgs() << "Pizza Second substring " << line.substr(secondQuote+2).c_str() << "\n";
    llvm::dbgs() << "Pizza WCET " << WCET << "\n";
    if(WCET < minWCET){
      minWCET = WCET;

      ret = line.substr(firstCommaInPass + 1, (secondQuote - 1) - (firstCommaInPass + 1));    
      llvm::dbgs() << "Pizza ret " << ret << "\n";

    }    
   }
   return ret;
 }


  std::optional<std::vector<std::string>> suggestPasses() override {
    if (runned)
      return {};
    std::string functionName =  getFunctionNameToAnalyze();
    std::string ret = getPass(functionName);
    runned = true;
    return parsePassFromString(ret);

    
  }

  ~StrategyApplyCSV() override = default;
};


} // namespace Lavinium
