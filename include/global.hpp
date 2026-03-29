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

#ifndef REIFY_GLOBAL_HPP
#define REIFY_GLOBAL_HPP

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

  // The number of assignment statements in a basic block
  int NumAssignsPerBBL = 2;
  // The number of variables in each assignment statement
  int NumVarsPerAssign = 2;
  // The number of variables in each conditional statement
  int NumVarsInCond = 3;
  // When enabled, some variables can be arrays
  bool EnableArrayVars = true;
  // The probability of generating an array variable
  double ArrayVariableProba = 0.2;
  // The maximum number of dimensions for an array variable
  int MaxNumArrayDims = 3;
  // The maximum number of elements for each dimension of an array variable
  int MaxNumElsPerArrayDim = 3;
  // When enabled, some variables can be structs
  bool EnableStructVars = true;
  // The probability of generating a struct variable
  double StructVariableProba = 0.1;
  // The maximum number of fields for a struct
  int MaxNumStructFields = 3;
  // The maximum, recursive depth of a struct
  int MaxStructDepth = 2;

  // I don't want all coefficients to be 0, so this constraint takes care of that
  bool EnableInterestCoefs = true;
  // If true, we allow all operations in the terms and expressions
  // otherwise we only allow addition for expression and multiplication for terms
  // When true, the generation process is much harder and thereby might be slower
  bool EnableAllOps = false;
  // If true, we allow some variables to be declared as volatile so that
  // the compilers/optimizers cannot have any assumptions about them.
  bool EnableVolatileVars = false;
  // The probability of a variable being declared as volatile.
  double VolatileVariableProba = 0.4;

  ////////////////////////////////////////////////////////////
  ////// Function Generation Parameters
  ////////////////////////////////////////////////////////////

  // The number of basic blocks for each control flow graph
  int NumBblsPerFun = 15;
  // Probability of sampling a base graph from an existing control-flow graph
  // database (if available) during control-flow graph generation
  double SampleBaseGraphProba = 0.5;
  // The number of allowed variables (parameters and locals) for each function
  int NumVarsPerFun = 8;
  // The number of allowed local variables for each function
  // This should be less than NumVarsPerFun
  int NumLocalsPerFun = 2;
  // The allowed number of loops per function
  int MaxNumLoopsPerFun = 2;
  // The maximum number of basic blocks in a loop
  int MaxNumBblsPerLoop = 5;
  // When enabled, we disallow the generation of dead code (i.e., definitely unreachable code)
  bool DisallowDeadCode = false;

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
  // We don't want all initialisations to be 0, so this constraint takes care of that
  bool EnableInterestInits = true;
  // Consistent means that after a tipping point, from the same node, exactly one outward edge is
  // traversed.ample, if node 0 has edges to 1 and 3, then from 0, either:
  //   initially all edges taken will be 1, followed by all edges taken to 3
  // or:
  //   initially all edges taken will be 3, followed by all edges taken to 1
  // There will be no out-edge interleaving.
  bool EnableConsistentExecs = false;
  // The number of different initialisation sets we want the solver to find for a given function
  int NumInitsPerExec = 3;
  // When enabled, inject obvious undefined behaviour in the unexecuted blocks
  // Otherwise, we inject some random values to their coefficients
  bool EnableUBInUnexecutedBbls = false;
  // Probability of selecting unexecuted blocks to inject UBs. I'm not making this probability
  // large since this may lead to a much longer generation time.
  double UBInjectionProba = 0.3;
  // When enabled, some values are forced to be equal to give compilers
  // opportunity to perform value numbering optimisations like GVN and LVN.
  bool EnableValueNumbering = false;
  // The probability of forcing some values to be equal
  double ValueNumberingProba = 0.2;

  ////////////////////////////////////////////////////////////
  ////// Program Generation Parameters
  ////////////////////////////////////////////////////////////

  // Probability of replacing a coefficient with a call to another function
  double ReplaceProba = 0.05;
  // Number of functions we want to knit together
  // Fix: Large values would make the generated programs too slow due to bad LTO
  int FunctionDepth = 5;

  ////////////////////////////////////////////////////////////
  ////// Solver Parameters
  ////////////////////////////////////////////////////////////

  // Number of threads to use for the Bitwuzla SMT solver
  // Default is 1 (single-threaded). Higher values can improve performance
  // on multi-core systems but may increase memory usage.
  uint64_t BitwuzlaNumThreads = 1;

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
      ("A,Xenable-all-ops", "Enable all operations for terms and expressions (This may make the generation longer and harder)", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
      ("Xenable-volatile-vars", "Enable declaring some variables as volatile", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
      ("Xvolatile-var-proba", "Probability of declaring a variable as a volatile variable", cxxopts::value<double>())
      ("Xdisable-array-vars", "Disable generating array variables", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
      ("Xarray-var-proba", "Probability of declaring a variable as an array", cxxopts::value<double>())
      ("Xdisable-struct-vars", "Enable generating struct variables", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
      ("Xstruct-var-proba", "Probability of declaring a variable as a struct", cxxopts::value<double>())
      // Function generation
      ("Xnum-bbls-per-fun", "The number of allowed nodes for each control flow graph", cxxopts::value<int>())
      ("Xdisable-dead-code", "Disable the generation of dead code", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
      ("Xnum-vars-per-fun", "The number of allowed variables (parameters and local variables) for each function", cxxopts::value<int>())
      ("Xnum-locals-per-fun", "The number of allowed local variables for each function", cxxopts::value<int>())
      ("Xnum-inits-per-exec", "Number of initialisation sets to find per execution", cxxopts::value<int>())
      ("U,Xenable-ub-inject", "Enable the injection of undefined behaviors to those unexecuted basic blocks", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
      ("Xinject-ub-proba", "Probability of the selection of unexecuted blocks to inject undefined behaviors", cxxopts::value<double>())
      ("Xsample-bg-proba", "Probability of sampling a base graph from the given graph database", cxxopts::value<double>())
      ("Xenable-lvn-gvn", "Forcing some values to be the equivalent to give compilers opportunity to perform LVN/GVN", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
      ("Xlvn-gvn-proba", "Probability of forcing some values to be the equivalent as some others", cxxopts::value<double>())
      // Solver options
      ("Xbitwuzla-threads", "Number of threads for the Bitwuzla SMT solver (default: 1)", cxxopts::value<uint64_t>())
      ;
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
    if (args.count("Xnum-assigns-per-bbl")) {
      NumAssignsPerBBL = args["Xnum-assigns-per-bbl"].as<int>();
      if (NumAssignsPerBBL > 3) {
        std::cerr
            << "Warning: Too many assignments per basic block would make the generation much slower"
            << std::endl;
      }
      ensurePositive(NumAssignsPerBBL, "Xnum-assigns-per-bbl");
    }

    if (args.count("Xnum-vars-per-assign")) {
      NumVarsPerAssign = args["Xnum-vars-per-assign"].as<int>();
      if (NumVarsPerAssign > 5) {
        std::cerr
            << "Warning: Too many variables per assignment would make the generation much slower"
            << std::endl;
      }
      ensurePositive(NumVarsPerAssign, "Xnum-vars-per-assign");
    }

    if (args.count("Xnum-vars-in-cond")) {
      NumVarsInCond = args["Xnum-vars-in-cond"].as<int>();
      if (NumVarsInCond > 6) {
        std::cerr
            << "Warning: Too many variables per conditional would make the generation much slower"
            << std::endl;
      }
      ensurePositive(NumVarsInCond, "Xnum-vars-in-cond");
    }

    EnableAllOps = args["Xenable-all-ops"].as<bool>();

    EnableVolatileVars = args["Xenable-volatile-vars"].as<bool>();

    if (args.count("Xvolatile-var-proba")) {
      VolatileVariableProba = args["Xvolatile-var-proba"].as<double>();
      if (VolatileVariableProba <= 0) {
        std::cerr << "Error: The probability for declaring a variable as volatile "
                     "(--Xvolatile-var-proba) cannot be less than or equal to 0. It should be "
                     "within 0 to 1."
                  << std::endl;
        exit(1);
      }
      if (VolatileVariableProba > 1) {
        std::cerr << "Error: The probability for declaring a variable as volatile "
                  << "(--Xvolatile-var-proba) cannot be larger than 1. It should be within 0 to 1."
                  << std::endl;
        exit(1);
      }
    }

    EnableArrayVars = !args["Xdisable-array-vars"].as<bool>();

    if (args.count("Xarray-var-proba")) {
      ArrayVariableProba = args["Xarray-var-proba"].as<double>();
      if (ArrayVariableProba <= 0) {
        std::cerr << "Error: The probability for declaring a variable as an array "
                     "(--Xarray-var-proba) cannot be less than or equal to 0. It should be "
                     "within 0 to 1."
                  << std::endl;
        exit(1);
      }
      if (ArrayVariableProba > 1) {
        std::cerr << "Error: The probability for declaring a variable as an array "
                  << "(--Xarray-var-proba) cannot be larger than 1. It should be within 0 to 1."
                  << std::endl;
        exit(1);
      }
    }

    EnableStructVars = !args["Xdisable-struct-vars"].as<bool>();

    if (args.count("Xstruct-var-proba")) {
      StructVariableProba = args["Xstruct-var-proba"].as<double>();
      if (StructVariableProba <= 0) {
        std::cerr << "Error: The probability for declaring a variable as a struct "
                     "(--Xstruct-var-proba) cannot be less than or equal to 0. It should be "
                     "within 0 to 1."
                  << std::endl;
        exit(1);
      }
      if (StructVariableProba > 1) {
        std::cerr << "Error: The probability for declaring a variable as a struct "
                  << "(--Xstruct-var-proba) cannot be larger than 1. It should be within 0 to 1."
                  << std::endl;
        exit(1);
      }
    }

    if (args.count("Xnum-bbls-per-fun")) {
      NumBblsPerFun = args["Xnum-bbls-per-fun"].as<int>();
      if (NumBblsPerFun > 20) {
        std::cerr
            << "Warning: Too many basic blocks per control flow graph would make the generation "
               "much slower"
            << std::endl;
      }
      ensurePositive(NumBblsPerFun, "Xnum-bbls-per-fun");
    }

    DisallowDeadCode = args["Xdisable-dead-code"].as<bool>();

    if (args.count("Xnum-vars-per-fun")) {
      NumVarsPerFun = args["Xnum-vars-per-fun"].as<int>();
      if (NumVarsPerFun > 16) {
        std::cerr
            << "Warning: Too many variables per function would make the generation much slower"
            << std::endl;
      }
      ensurePositive(NumVarsPerFun, "Xnum-vars-per-fun");
      if (NumAssignsPerBBL >= NumVarsPerFun) {
        std::cerr
            << "Error: The number of assignments per basic block (--Xnum-assigns-per-bbl) cannot "
               "exceed the number of variables allowed in each function (--Xnum-vars-per-fun)"
            << std::endl;
        exit(1);
      }
    }

    if (args.count("Xnum-locals-per-fun")) {
      NumLocalsPerFun = args["Xnum-locals-per-fun"].as<int>();
      if (NumLocalsPerFun > 16) {
        std::cerr << "Warning: Too many locals per function would make the generation much slower"
                  << std::endl;
      }
      ensurePositive(NumLocalsPerFun, "Xnum-locals-per-fun");
      if (NumLocalsPerFun >= NumVarsPerFun) {
        std::cerr
            << "Error: The number of local variables per function (--Xnum-locals-per-fun) cannot "
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
      ensurePositive(NumInitsPerExec, "num-inits-per-exec");
    }

    EnableUBInUnexecutedBbls = args["Xenable-ub-inject"].as<bool>();

    if (args.count("Xinject-ub-proba")) {
      UBInjectionProba = args["Xinject-ub-proba"].as<double>();
      if (UBInjectionProba <= 0) {
        std::cerr
            << "Error: The probability for selecting basic blocks to inject UBs "
               "(--Xinject-ub-proba) cannot be less than or equal to 0. It should be within 0 to 1."
            << std::endl;
        exit(1);
      }
      if (UBInjectionProba > 1) {
        std::cerr << "Error: The probability for selecting basic blocks to inject UBs "
                  << "(--Xinject-ub-proba) cannot be larger than 1. It should be within 0 to 1."
                  << std::endl;
        exit(1);
      }
    }

    EnableValueNumbering = args["Xenable-lvn-gvn"].as<bool>();

    if (args.count("Xlvn-gvn-proba")) {
      ValueNumberingProba = args["Xlvn-gvn-proba"].as<double>();
      if (ValueNumberingProba <= 0) {
        std::cerr
            << "Error: The probability for selecting some values to be equivalent "
               "(--Xlvn-gvn-proba) cannot be less than or equal to 0. It should be within 0 to 1."
            << std::endl;
        exit(1);
      }
      if (ValueNumberingProba > 1) {
        std::cerr << "Error: The probability for selecting some values to be equivalent "
                  << "(--Xlvn-gvn-proba) cannot be larger than 1. It should be within 0 to 1."
                  << std::endl;
        exit(1);
      }
    }

    if (args.count("Xsample-bg-proba")) {
      SampleBaseGraphProba = args["Xsample-bg-proba"].as<double>();
      if (SampleBaseGraphProba <= 0) {
        std::cerr << "Error: The probability for sampling a base graph from a graph database "
                     "(--Xsample-bg-proba) cannot be less than or equal to 0. It should be "
                     "within 0 to 1."
                  << std::endl;
        exit(1);
      }
      if (SampleBaseGraphProba > 1) {
        std::cerr << "Error: The probability for sampling a base graph from a graph database "
                  << "(--Xsample-bg-proba) cannot be larger than 1. It should be within "
                     "0 to 1."
                  << std::endl;
        exit(1);
      }
    }

    if (args.count("Xbitwuzla-threads")) {
      BitwuzlaNumThreads = args["Xbitwuzla-threads"].as<uint64_t>();
      if (BitwuzlaNumThreads < 1) {
        std::cerr << "Error: The number of Bitwuzla threads (--Xbitwuzla-threads) must be at "
                     "least 1."
                  << std::endl;
        exit(1);
      }
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
  static void ensurePositive(const int v, const char *opt) {
    if (v < 0) {
      std::cerr << "Error: The option --" << opt << " must be a positive value" << std::endl;
      exit(1);
    }
  }

private:
  // Private constructor to enforce singleton pattern
  GlobalOptions() = default;
};

#endif // REIFY_GLOBAL_HPP
