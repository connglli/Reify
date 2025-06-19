// MIT License
//
// Copyright (c) 2025
//
// Kavya Chopra (chopra.kavya04@gmail.com)
// Cong Li (cong.li@inf.ethz.ch)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef GRAPHFUZZ_GLOBAL_HPP
#define GRAPHFUZZ_GLOBAL_HPP

#include <climits>
#include <filesystem>

#include "cxxopts.hpp"

struct GlobalOptions {
  ////////////////////////////////////////////////////////////
  ////// Block Generation Parameters
  ////////////////////////////////////////////////////////////

  // Upper bound for the results of expression and subexpression evaluation
  int UpperBound = INT_MAX;
  // Lower bound for the results of expression and subexpression evaluation
  int LowerBound = INT_MIN;
  // Lower bound for the initialisation of variables
  int LowerInitBound = INT_MIN;
  // Upper bound for the initialisation of variables
  int UpperInitBound = INT_MAX;
  // Lower bound for coefficients to make quadratic integer arithmetic more tractable for the solver
  int LowerCoefBound = -1000;
  // Upper bound for coefficients to make quadratic integer arithmetic more tractable for the solver
  int UpperCoefBound = 1000;

  // The number of assignment statements in a basic block
  int NumAssignPerBBL = 1;
  // The number of variables in each assignment statement
  int NumVarsPerAssign = 2;
  // The number of variables in each conditional statement
  int NumVarsInCond = 4;
  // These are our anti-UB (mostly just checking for overflow and underflow) checks
  bool EnableSafetyChecks = true;
  // I don't want all coefficients to be 0, so this constraint takes care of that
  bool EnableInterestCoefs = true;

  ////////////////////////////////////////////////////////////
  ////// Function Generation Parameters
  ////////////////////////////////////////////////////////////

  // The number of nodes for each control flow graph
  int NumNodesPerFun = 10;
  // The number of allowed variables for each function
  int NumVarsPerFun = 8;

  // Empirically, if random initialisations are enabled, then the smt solver isn't able to generate
  // more initialisations for the same set of coefficients. I would recommend to either have a
  // reasonably high number of NUMBER_OF_INITIALISATIONS_OF_EACH_WALK and set this to false, or set
  // this to true and set NUMBER_OF_INITIALISATIONS_OF_EACH_WALK 1
  bool EnableRandomInits = false;
  // I don't want all initialisations to be 0, so this constraint takes care of that
  bool EnableInterestInits = true;
  // Consistent means that after a tipping point, from the same node, exactly one outward edge is
  // traversed.ample, if node 0 has edges to 1 and 3, then from 0, either:
  //   initially all edges taken will be 1, followed by all edges taken to 3
  // or:
  //   initially all edges taken will be 3, followed by all edges taken to 1
  // There will be no out-edge interleaving.
  bool EnableConsistentWalks = true;
  // The number of different initialisation sets we want the solver to find for a given function
  int NumInitsPerWalk = 3;

  ////////////////////////////////////////////////////////////
  ////// Program Generation Parameters
  ////////////////////////////////////////////////////////////

  // Probability of replacing a coefficient/constant with a call to another function
  double ReplaceProba = 0.5;
  // Number of functions we want to knit together
  int FunctionDepth = 3;

  ////////////////////////////////////////////////////////////
  ////// Helper Functions
  ////////////////////////////////////////////////////////////

  static GlobalOptions &Get() {
    static GlobalOptions instance;
    return instance;
  }

private:
  // Private constructor to enforce singleton pattern
  GlobalOptions() = default;
};

////////////////////////////////////////////////////////////
////// Outputs
////////////////////////////////////////////////////////////

#define FUNCTION_NAME_PREFIX "function"

static std::filesystem::path GetFunctionsDir(const std::filesystem::path &output) {
  return output / "functions";
}

static std::filesystem::path GetMappingsDir(const std::filesystem::path &output) {
  return output / "mappings";
}

static std::filesystem::path GetLoggingsDir(const std::filesystem::path &output) {
  return output / "loggings";
}

static std::filesystem::path GetProgramsDir(const std::filesystem::path &output) {
  return output / "programs";
}

static std::string GetFunctionName(const std::string &uuid, const std::string &sno) {
  return std::string(FUNCTION_NAME_PREFIX) + "_" + uuid + "_" + sno;
}

static std::filesystem::path GetFunctionPath(
    const std::string &uuid, const std::string &sno, const std::filesystem::path &output
) {
  return GetFunctionsDir(output) / (GetFunctionName(uuid, sno) + ".c");
}

static std::string GetMappingNameForFunctionName(const std::string &functionName) {
  return functionName + ".map";
}

static std::filesystem::path GetMappingPath(
    const std::string &uuid, const std::string &sno, const std::filesystem::path &output
) {
  return GetMappingsDir(output) / GetMappingNameForFunctionName(GetFunctionName(uuid, sno));
}

static std::filesystem::path
GetMappingPathForFunctionPath(const std::filesystem::path &functionPath) {
  return GetMappingsDir(functionPath.parent_path().parent_path()) /
         GetMappingNameForFunctionName(functionPath.stem());
}

static std::string GetProgramNameForFunctionName(const std::string &functionName) {
  return functionName.substr(std::string(FUNCTION_NAME_PREFIX).size() + 1) + ".c";
}

static std::filesystem::path GetProgramPath(
    const std::string &uuid, const std::string &sno, const std::filesystem::path &output
) {
  return GetProgramsDir(output) / GetProgramNameForFunctionName(GetFunctionName(uuid, sno));
}

static std::filesystem::path
GetGetProgramPathPathForFunctionPath(const std::filesystem::path &functionPath) {
  return GetProgramsDir(functionPath.parent_path().parent_path()) /
         GetProgramNameForFunctionName(functionPath.stem());
}

static std::string GetLoggingNameForFunctionName(const std::string &functionName) {
  return functionName + ".log";
}

static std::filesystem::path GetGenLogPath(
    const std::string &uuid, const std::string &sno, const std::filesystem::path &output,
    bool devnull = true
) {
  if (devnull) {
    return {"/dev/null"};
  } else {
    return GetLoggingsDir(output) / GetLoggingNameForFunctionName(GetFunctionName(uuid, sno));
  }
}

static std::filesystem::path
GetGenLogPathForFunctionPath(const std::filesystem::path &functionPath) {
  return GetLoggingsDir(functionPath.parent_path().parent_path()) /
         GetLoggingNameForFunctionName(functionPath.stem());
}


#endif // GRAPHFUZZ_GLOBAL_HPP
