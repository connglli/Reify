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

////////////////////////////////////////////////////////////
////// Block Generation
////////////////////////////////////////////////////////////

#define UPPER_BOUND INT_MAX      // upper bound for the results of expression and subexpression evaluation
#define LOWER_BOUND INT_MIN      // lower bound for the results of expression and subexpression evaluation
#define LOWER_INIT_BOUND INT_MIN // lower bound for the initialisation of variables
#define UPPER_INIT_BOUND INT_MAX // upper bound for the initialisation of variables
#define LOWER_COEFF_BOUND                                                                                              \
  -1000 // lower bound for coefficients. added to make quadratic integer arithmetic more tractable for the solver
#define UPPER_COEFF_BOUND                                                                                              \
  1000 // upper bound for coefficients. added to make quadratic integer arithmetic more tractable for the solver

#define NUM_ASSIGNMENTS_PER_BB 1       // the number of assignment statements in a basic block
#define NUM_VARIABLES_PER_ASSIGNMENT 2 // the number of variables in each assignment statement
#define NUM_VARIABLES_IN_CONDITIONAL 4 // the number of variables in each conditional statement
#define ENABLE_SAFETY_CHECKS true      // these are our anti-UB (mostly just checking for overflow and underflow) checks
#define ENABLE_INTERESTING_COEFFS true // i don't want all coefficients to be 0, so this constraint takes care of that

////////////////////////////////////////////////////////////
////// Function Generation
////////////////////////////////////////////////////////////

#define NUM_NODES_PER_FUNC 10 // the umber of nodes for each function's graph
// Consistent means that after a tipping point, from the same node, exactly one outward edge is traversed.ample,
// if node 0 has edges to 1 and 3, then from 0, either:
//   initially all edges taken will be 1, followed by all edges taken to 3
// or:
//   initially all edges taken will be 3, followed by all edges taken to 1
// There will be no out-edge interleaving.
#define NUM_VARS_PER_FUNC 8 // the number of allowed variables in a function

// Empirically, if random initialisations are enabled, then the smt solver isn't able to generate more initialisations
// for the same set of coefficients. I would recommend to either have a reasonably high number of
// NUMBER_OF_INITIALISATIONS_OF_EACH_WALK and set this to false, or set this to true and set
// NUMBER_OF_INITIALISATIONS_OF_EACH_WALK 1
#define ENABLE_RANDOM_INITS false     // randomly initialise the variables
#define ENABLE_INTERESTING_INITS true // i don't want all initialisations to be 0, so this constraint takes care of that

#define ENABLE_CONSISTENT_WALKS true
#define NUM_INITS_PER_WALK                                                                                             \
  3 // the number of different initialisation sets we want the solver to find for a given procedure

#define BITSHIFT_BY 6        // not used for now, but tried an experiment with bitwise operations
#define BITWIDTH_DATATYPE 32 // not used for now, but tried an experiment with bitwise operations

////////////////////////////////////////////////////////////
////// Program Generation
////////////////////////////////////////////////////////////

#define REPLACEMENT_PROBABILITY 0.5 // Probability of replacing a coefficient/constant with a call to another procedure
#define PROCEDURE_DEPTH 3           // Number of procedures we want to knit together

////////////////////////////////////////////////////////////
////// Outputs
////////////////////////////////////////////////////////////

static std::filesystem::path PROCEDURES_DIR = "procedures";
static std::filesystem::path MAPPINGS_DIR = "mappings";
static std::filesystem::path GEN_LOGS_DIR = "logs";

static std::filesystem::path NEW_PROCEDURES_DIR = "new_procedures";

static std::string GetProcedureName(const std::string &uuid, int sno) {
  return "function_" + uuid + "_" + std::to_string(sno);
}

static std::filesystem::path GetProcedurePath(const std::string &uuid, int sno) {
  return PROCEDURES_DIR / (GetProcedureName(uuid, sno) + ".c");
}

static std::string GetMappingNameForProcedureName(const std::string &procedureName) {
  return procedureName + "_mapping";
}

static std::filesystem::path GetMappingPath(const std::string &uuid, int sno) {
  return MAPPINGS_DIR / GetMappingNameForProcedureName(GetProcedureName(uuid, sno));
}

static std::string GetGenLogNameForProcedureName(const std::string &procedureName) { return procedureName + "_log"; }

static std::filesystem::path GetGenLogPath(const std::string &uuid, int sno, bool devnull = true) {
  if (devnull) {
    return {"/dev/null"};
  } else {
    return GEN_LOGS_DIR / GetGenLogNameForProcedureName(GetProcedureName(uuid, sno));
  }
}

#endif // GRAPHFUZZ_GLOBAL_HPP
