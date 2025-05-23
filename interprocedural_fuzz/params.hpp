const int PROCEDURE_DEPTH = 3; // Number of procedures we want to knit together
const int globalChecksumType = 0; // 0 for CRC32, 1 for simple checksum
const int mod = 1000000007;
const double replacementProbability = 0.5; // Probability of replacing a coefficient/constant with a call to another procedure