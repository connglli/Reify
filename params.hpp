#pragma once
int NUM_VARS = 8;
const int UPPER_BOUND = INT_MAX;
const int LOWER_BOUND = INT_MIN;
const int LOWER_INIT_BOUND = INT_MIN;
const int UPPER_INIT_BOUND = INT_MAX;
int ASSIGNMENTS_PER_BB =1;
int VARIABLES_PER_ASSIGNMENT_STATEMENT = 2;
int NUM_VARIABLES_IN_CONDITIONAL = 4;
int NODES = 10;
bool enableConsistentWalks = true; 
/**
Consistent means that after a tipping point, from the same node, exactly one outward edge is traversed. 
For example,
if node 0 has edges to 1, 3, then from 0, initially either all edges taken will be 1, followed by all edges taken to 3.
OR
if node 0 has edges to 1, 3, then from 0, initially either all edges taken will be 3, followed by all edges taken to 1.
There will be no edge interleaving.
*/
const bool enableRandomInitialisations = true;
const bool enableSafetyChecks = true;
const bool enableInterestingCoefficients = true;
const bool enableInterestingInitialisations = true;
const bool enablePreInitialisations = true;
const int BITSHIFT_BY = 6;
const int BITWIDTH_DATATYPE = 32;
const int MODULO_PRIME = 1000000007;
const int LOWER_COEFF_BOUND = INT_MIN;
const int UPPER_COEFF_BOUND = INT_MAX;
const int NUMBER_OF_INITIALISATIONS_OF_EACH_WALK = 1;
const int NUMBER_OF_PROCEDURES = 2;