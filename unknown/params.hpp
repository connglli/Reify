#pragma once
int NUM_VARS = 8;
const int UPPER_BOUND = INT_MAX; // upper bound for the results of expression and subexpression evaluation
const int LOWER_BOUND = INT_MIN; // lower bound for the results of expression and subexpression evaluation
const int LOWER_INIT_BOUND = INT_MIN; // lower bound for the initialisation of variables
const int UPPER_INIT_BOUND = INT_MAX; // upper bound for the initialisation of variables
int ASSIGNMENTS_PER_BB = 1; // the number of assignment statements in a basic block
int VARIABLES_PER_ASSIGNMENT_STATEMENT = 2; // the number of variables in each assignment statement
int NUM_VARIABLES_IN_CONDITIONAL = 4; // the number of variables in each conditional statement
int NODES = 10; // Number of nodes in the graph
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
const bool enableInterestingCoefficients = true; // i don't want all coefficients to be 0, so this constraint takes care of that
const bool enableInterestingInitialisations = true; // i don't want all initialisations to be 0, so this constraint takes care of that
/**  
empirically, if random initialisations are enabled, then the smt solver isn't able to generate more initialisations for the same set of coefficients.
I would recommend to either have a reasonably high number of NUMBER_OF_INITIALISATIONS_OF_EACH_WALK and set this to false, or set this to true and set NUMBER_OF_INITIALISATIONS_OF_EACH_WALK = 1;
*/
const int BITSHIFT_BY = 6; //not used for procedure gen, but tried an experiment with bitwise operations
const int BITWIDTH_DATATYPE = 32; //not used for procedure gen, but tried an experiment with bitwise operations
const int MODULO_PRIME = 1000000007;
const int LOWER_COEFF_BOUND = -1000; //lower bound for coefficients. added to make quadratic integer arithmetic more tractable for the solver
const int UPPER_COEFF_BOUND = 1000; //upper bound for coefficients. added to make quadratic integer arithmetic more tractable for the solver
const int NUMBER_OF_INITIALISATIONS_OF_EACH_WALK = 3; //the number of different initialisation sets we want the solver to find for a given procedure