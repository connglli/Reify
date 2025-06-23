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
#include <iostream>

#include "cxxopts.hpp"
#include "lib/dbgutils.hpp"

struct GlobalOptions {
  ////////////////////////////////////////////////////////////
  ////// Block Generation Parameters
  ////////////////////////////////////////////////////////////

  // Upper bound for the results of expression and subexpression evaluation
  int UpperBound = INT_MAX;
  // Lower bound for the results of expression and subexpression evaluation
  int LowerBound = INT_MIN;
  // Lower bound for coefficients to make quadratic integer arithmetic more tractable for the solver
  int LowerCoefBound = -1000;
  // Upper bound for coefficients to make quadratic integer arithmetic more tractable for the solver
  int UpperCoefBound = 1000;
  // Lower bound for the initialisation of variables
  int LowerInitBound = INT_MIN;
  // Upper bound for the initialisation of variables
  int UpperInitBound = INT_MAX;

  // The number of assignment statements in a basic block
  int NumAssignsPerBBL = 1;
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
  // The allowed number of loops per function
  int MaxNumLoopsPerFun = 2;
  // The maximum number of basic blocks in a loop
  int MaxNumBblsPerLoop = 5;

  // The max number of executions that we can try sampling for each function
  int MaxNumExecsPerFun = 3;
  // The number of execution steps per function. Empirically, this should be a number
  // that is considerably larger than the number of nodes in the function so that we
  // can eventually sample an reasonable execution.
  int MaxNumExecStepsPerFun = 1000;
  // Empirically, if random initialisations are enabled, then the smt solver isn't able to generate
  // more initialisations for the same set of coefficients. I would recommend to either have a
  // reasonably high number of NumInitsPerExec and set this to false, or set this to true and set
  // NumInitsPerExec to 1
  bool EnableRandomInits = false;
  // I don't want all initialisations to be 0, so this constraint takes care of that
  bool EnableInterestInits = true;
  // Consistent means that after a tipping point, from the same node, exactly one outward edge is
  // traversed.ample, if node 0 has edges to 1 and 3, then from 0, either:
  //   initially all edges taken will be 1, followed by all edges taken to 3
  // or:
  //   initially all edges taken will be 3, followed by all edges taken to 1
  // There will be no out-edge interleaving.
  bool EnableConsistentExecs = true;
  // The number of different initialisation sets we want the solver to find for a given function
  int NumInitsPerExec = 3;

  ////////////////////////////////////////////////////////////
  ////// Program Generation Parameters
  ////////////////////////////////////////////////////////////

  // Probability of replacing a coefficient with a call to another function
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

  static void AddFuncOpts(cxxopts::Options &opts) {
    // clang-format off
    opts.add_options()
      // Block generation
      ("Xlower-coef-bound", "Lower bound for coefficients", cxxopts::value<int>())
      ("Xupper-coef-bound", "Upper bound for coefficients", cxxopts::value<int>())
      ("Xnum-assigns-per-bbl", "The number of assignment statements in each basic block", cxxopts::value<int>())
      ("Xnum-vars-per-assign", "The number of variables in each assignment statement", cxxopts::value<int>())
      ("Xnum-vars-in-cond", "The number of variables in each conditional statement", cxxopts::value<int>())
      // Function generation
      ("Xnum-nodes-per-fun", "The number of allowed nodes for each control flow graph", cxxopts::value<int>())
      ("Xnum-vars-per-fun", "The number of allowed variables for each function", cxxopts::value<int>())
      ("Xnum-inits-per-exec", "Number of initialisation sets to find per execution", cxxopts::value<int>());
    // clang-format on
  }

  static void AddProgOpts(cxxopts::Options &opts) {
    // clang-format off
    opts.add_options()
      // Program generation
      ("Xreplace-proba", "Probability of replacing a coefficient with a function call", cxxopts::value<double>())
      ("Xfunction-depth", "The number of functions to knit together per program", cxxopts::value<int>());
    // clang-format on
  }

  void HandleFuncArgs(const cxxopts::ParseResult &args) {
    if (args.count("Xlower-coef-bound")) {
      LowerCoefBound = args["Xlower-coef-bound"].as<int>();
    }

    if (args.count("Xupper-coef-bound")) {
      UpperCoefBound = args["Xupper-coef-bound"].as<int>();
    }

    if (UpperCoefBound <= LowerCoefBound) {
      std::cerr << "Error: The upper bound for coefficient (--upper-coef-bound) must be larger "
                   "than the lower bound (--lower-coef-bound)."
                << std::endl;
      exit(1);
    }

    if (args.count("Xnum-assigns-per-bbl")) {
      NumAssignsPerBBL = args["Xnum-assigns-per-bbl"].as<int>();
      if (NumAssignsPerBBL > 3) {
        std::cerr
            << "Warning: Too many assignments per basic block would make the generation much slower"
            << std::endl;
      }
      EnsurePositive(NumAssignsPerBBL, "Xnum-assigns-per-bbl");
    }

    if (args.count("Xnum-vars-per-assign")) {
      NumVarsPerAssign = args["Xnum-vars-per-assign"].as<int>();
      if (NumVarsPerAssign > 5) {
        std::cerr
            << "Warning: Too many variables per assignment would make the generation much slower"
            << std::endl;
      }
      EnsurePositive(NumVarsPerAssign, "Xnum-vars-per-assign");
    }

    if (args.count("Xnum-vars-in-cond")) {
      NumVarsInCond = args["Xnum-vars-in-cond"].as<int>();
      if (NumVarsInCond > 6) {
        std::cerr
            << "Warning: Too many variables per conditional would make the generation much slower"
            << std::endl;
      }
      EnsurePositive(NumVarsInCond, "Xnum-vars-in-cond");
    }

    if (args.count("Xnum-nodes-per-fun")) {
      NumNodesPerFun = args["Xnum-nodes-per-fun"].as<int>();
      if (NumNodesPerFun > 20) {
        std::cerr << "Warning: Too many nodes per control flow graph would make the generation "
                     "much slower"
                  << std::endl;
      }
      EnsurePositive(NumNodesPerFun, "Xnum-nodes-per-fun");
    }

    if (args.count("Xnum-vars-per-fun")) {
      NumVarsPerFun = args["Xnum-vars-per-fun"].as<int>();
      if (NumVarsPerFun > 16) {
        std::cerr
            << "Warning: Too many variables per function would make the generation much slower"
            << std::endl;
      }
      EnsurePositive(NumVarsPerFun, "Xnum-vars-per-fun");
      if (NumAssignsPerBBL >= NumVarsPerFun) {
        std::cerr
            << "Error: The number of assignments per basic block (--Xnum-assigns-per-bbl) cannot "
               "exceed the number of variables allowed in each function (--Xnum-vars-per-fun)"
            << std::endl;
        exit(1);
      }
    }

    if (args.count("Xnum-inits-per-exec")) {
      NumInitsPerExec = args["Xnum-inits-per-exec"].as<int>();
      if (NumInitsPerExec > 5) {
        std::cerr
            << "Warning: Too many initialisations per exec would make the generation much slower"
            << std::endl;
      }
      EnsurePositive(NumInitsPerExec, "num-inits-per-exec");
    }
  }

  void HandleProgArgs(const cxxopts::ParseResult &args) {
    if (args.count("Xreplace-proba")) {
      ReplaceProba = args["Xreplace-proba"].as<double>();
      if (ReplaceProba <= 0) {
        std::cerr << "Error: The probability for replacing (--Xreplace-proba) a coefficient cannot "
                     "be less than or equal to 0. It should be within 0 to 1."
                  << std::endl;
        exit(1);
      }
      if (ReplaceProba > 1) {
        std::cerr << "Error: The probability for replacing (--Xreplace-proba) a coefficient cannot "
                     "be larger than 1. It should be within 0 to 1."
                  << std::endl;
        exit(1);
      }
    }

    if (args.count("Xfunction-depth")) {
      FunctionDepth = args["Xfunction-depth"].as<int>();
    }
  }

private:
  static void EnsurePositive(const int v, const char *opt) {
    if (v < 0) {
      std::cerr << "Error: The option --" << opt << " must be a positive value" << std::endl;
      exit(1);
    }
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
