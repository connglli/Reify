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
////// Function Generation
////////////////////////////////////////////////////////////

#define NUM_VARS 8
#define UPPER_BOUND INT_MAX                  // upper bound for the results of expression and subexpression evaluation
#define LOWER_BOUND INT_MIN                  // lower bound for the results of expression and subexpression evaluation
#define LOWER_INIT_BOUND INT_MIN             // lower bound for the initialisation of variables
#define UPPER_INIT_BOUND INT_MAX             // upper bound for the initialisation of variables
#define ASSIGNMENTS_PER_BB 1                 // the number of assignment statements in a basic block
#define VARIABLES_PER_ASSIGNMENT_STATEMENT 2 // the number of variables in each assignment statement
#define NUM_VARIABLES_IN_CONDITIONAL 4       // the number of variables in each conditional statement
#define NUM_NODES 10                         // Number of nodes in the graph
#define enableConsistentWalks true
/**
Consistent means that after a tipping point, from the same node, exactly one outward edge is traversed.
For example,
if node 0 has edges to 1, 3, then from 0, initially either all edges taken will be 1, followed by all edges taken to 3.
OR
if node 0 has edges to 1, 3, then from 0, initially either all edges taken will be 3, followed by all edges taken to 1.
There will be no out-edge interleaving.
*/
#define enableRandomInitialisations false // randomly initialise the variables
#define enableSafetyChecks true // these are our anti-UB (mostly just checking for overflow and underflow) checks
#define enableInterestingCoefficients                                                                                  \
  true // i don't want all coefficients to be 0, so this constraint takes care of that
#define enableInterestingInitialisations                                                                               \
  true // i don't want all initialisations to be 0, so this constraint takes care of that
/**
Empirically, if random initialisations are enabled, then the smt solver isn't able to generate more initialisations for
the same set of coefficients. I would recommend to either have a reasonably high number of
NUMBER_OF_INITIALISATIONS_OF_EACH_WALK and set this to false, or set this to true and set
NUMBER_OF_INITIALISATIONS_OF_EACH_WALK 1
*/
#define BITSHIFT_BY 6        // not used for procedure gen, but tried an experiment with bitwise operations
#define BITWIDTH_DATATYPE 32 // not used for procedure gen, but tried an experiment with bitwise operations
#define MODULO_PRIME 1000000007
#define LOWER_COEFF_BOUND                                                                                              \
  -1000 // lower bound for coefficients. added to make quadratic integer arithmetic more tractable for the solver
#define UPPER_COEFF_BOUND                                                                                              \
  1000 // upper bound for coefficients. added to make quadratic integer arithmetic more tractable for the solver
#define NUMBER_OF_INITIALISATIONS_OF_EACH_WALK                                                                         \
  3 // the number of different initialisation sets we want the solver to find for a given procedure

////////////////////////////////////////////////////////////
////// Program Generation
////////////////////////////////////////////////////////////

#define PROCEDURE_DEPTH 3    // Number of procedures we want to knit together
#define globalChecksumType 0 // 0 for CRC32, 1 for simple checksum
#define CHECKSUM_MOD 1000000007
#define replacementProbability 0.5 // Probability of replacing a coefficient/constant with a call to another procedure

////////////////////////////////////////////////////////////
////// Outputs
////////////////////////////////////////////////////////////

static std::filesystem::path PROCEDURES_DIR = "procedures";
static std::filesystem::path MAPPINGS_DIR = "mappings";
static std::filesystem::path GEN_LOGS_DIR = "logs";

static std::string GetProcedureName(std::string uuid, int sno) {
  return "function_" + uuid + "_" + std::to_string(sno);
}

static std::filesystem::path GetProcedurePath(std::string uuid, int sno) {
  return PROCEDURES_DIR / (GetProcedureName(uuid, sno) + ".c");
}

static std::filesystem::path GetMappingPath(std::string uuid, int sno) {
  return MAPPINGS_DIR / (GetProcedureName(uuid, sno) + "_mapping");
}

static std::filesystem::path GetGenLogPath(std::string uuid, int sno, bool devnull = true) {
  if (devnull) {
    return std::filesystem::path("/dev/null");
  } else {
    return GEN_LOGS_DIR / (GetProcedureName(uuid, sno) + "_log");
  }
}
#endif // GRAPHFUZZ_GLOBAL_HPP
