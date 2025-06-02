
// MIT License
//
// Copyright (c) 2025-2025
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

#ifndef GRAPHFUZZ_FUNC_PARAMS_H
#define GRAPHFUZZ_FUNC_PARAMS_H

int NUM_VARS = 8;
const int UPPER_BOUND = INT_MAX;            // upper bound for the results of expression and subexpression evaluation
const int LOWER_BOUND = INT_MIN;            // lower bound for the results of expression and subexpression evaluation
const int LOWER_INIT_BOUND = INT_MIN;       // lower bound for the initialisation of variables
const int UPPER_INIT_BOUND = INT_MAX;       // upper bound for the initialisation of variables
int ASSIGNMENTS_PER_BB = 1;                 // the number of assignment statements in a basic block
int VARIABLES_PER_ASSIGNMENT_STATEMENT = 2; // the number of variables in each assignment statement
int NUM_VARIABLES_IN_CONDITIONAL = 4;       // the number of variables in each conditional statement
int NUM_NODES = 10;                         // Number of nodes in the graph
bool enableConsistentWalks = true;
/**
Consistent means that after a tipping point, from the same node, exactly one outward edge is traversed.
For example,
if node 0 has edges to 1, 3, then from 0, initially either all edges taken will be 1, followed by all edges taken to 3.
OR
if node 0 has edges to 1, 3, then from 0, initially either all edges taken will be 3, followed by all edges taken to 1.
There will be no out-edge interleaving.
*/
const bool enableRandomInitialisations = false; // randomly initialise the variables
const bool enableSafetyChecks = true; // these are our anti-UB (mostly just checking for overflow and underflow) checks
const bool enableInterestingCoefficients =
    true; // i don't want all coefficients to be 0, so this constraint takes care of that
const bool enableInterestingInitialisations =
    true; // i don't want all initialisations to be 0, so this constraint takes care of that
/**
Empirically, if random initialisations are enabled, then the smt solver isn't able to generate more initialisations for
the same set of coefficients. I would recommend to either have a reasonably high number of
NUMBER_OF_INITIALISATIONS_OF_EACH_WALK and set this to false, or set this to true and set
NUMBER_OF_INITIALISATIONS_OF_EACH_WALK = 1;
*/
const int BITSHIFT_BY = 6;        // not used for procedure gen, but tried an experiment with bitwise operations
const int BITWIDTH_DATATYPE = 32; // not used for procedure gen, but tried an experiment with bitwise operations
const int MODULO_PRIME = 1000000007;
const int LOWER_COEFF_BOUND =
    -1000; // lower bound for coefficients. added to make quadratic integer arithmetic more tractable for the solver
const int UPPER_COEFF_BOUND =
    1000; // upper bound for coefficients. added to make quadratic integer arithmetic more tractable for the solver
const int NUMBER_OF_INITIALISATIONS_OF_EACH_WALK =
    3; // the number of different initialisation sets we want the solver to find for a given procedure

#endif // GRAPHFUZZ_FUNC_PARAMS_H
