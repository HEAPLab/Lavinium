#pragma once
#include "llvm/Support/Debug.h"
#include <chrono>
#include <fstream>
#include <string>

#define CONCAT(a, b) CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a ## b
#define TIMEME auto CONCAT(notSoCommonName, __LINE__) = timeMe( __FILE__ , __FUNCTION__, __LINE__) ;

static std::ofstream& getFile(){
  static std::ofstream* timeFile = nullptr;
  if (!timeFile){
    timeFile = new std::ofstream("timeFile.txt");
  }
   return *timeFile;
}

static int& getDepth(){
    static int i = 0;
    return i;
}

static void printSpace(){
        auto& stream = getFile();
        for(int i = 0; i< getDepth(); i++){
        stream << "###" ;
        if (i + 1< getDepth()){
        stream << "-" ;
        }else {
        stream << " " ;
        }
}}

struct timeMe {
    std::string file;
    std::string func;
    std::string line;
    std::chrono::time_point<std::chrono::system_clock> start;
    timeMe (std::string file, std::string func, int line) : file(file), func(func), line(std::to_string(line)), start(std::chrono::system_clock::now()){

        if(!getDepth()){
      getFile() << "\n{\n";
        }
      getDepth()++;
    }

    ~timeMe() {
        std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        getDepth()--;
        auto& stream = getFile();
        if(elapsed> 100){
        printSpace();
        stream << file + ":" + line + "\n";
        printSpace();
        stream << func << ": "  << elapsed << "\n";
        }
        if(!getDepth()){
        stream <<  "\n}\n";
        stream.flush();
        }

    }

};
