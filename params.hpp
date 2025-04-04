const int NUM_VARS = 5;
const int UPPER_BOUND = INT_MAX;
const int LOWER_BOUND = INT_MIN;
const int ASSIGNMENTS_PER_BB = 3;
const int VARIABLES_PER_ASSIGNMENT_STATEMENT = 2;
const int NUM_VARIABLES_IN_CONDITIONAL = 3;
const int NODES = 8;
const bool enableConsistentWalks = true; 
/**
Consistent means that after a tipping point, from the same node, exactly one outward edge is traversed. 
For example,
if node 0 has edges to 1, 3, then from 0, initially either all edges taken will be 1, followed by all edges taken to 3.
OR
if node 0 has edges to 1, 3, then from 0, initially either all edges taken will be 3, followed by all edges taken to 1.
There will be no edge interleaving.
*/
const bool enableSafetyChecks = true;
const bool enableInterestingCoefficients = true;
const bool enableInterestingInitialisations = true;
