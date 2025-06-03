
#ifndef GRAPHFUZZ_NAMING_HPP
#define GRAPHFUZZ_NAMING_HPP
#include <string>

static std::string generatePassCounterName() { return "pass_counter"; }

static std::string getVarName(int varIndex) { return "var_" + std::to_string(varIndex); }

static std::string getCoefficientName(int blockno, int statementIndex, int statementSubIndex) {
  return "a_" + std::to_string(blockno) + "_" + std::to_string(statementIndex) + "_" +
         std::to_string(statementSubIndex);
}

static std::string generateConstantName(int blockno, int statementIndex) {
  return "a_" + std::to_string(blockno) + "_" + std::to_string(statementIndex);
}

static std::string generateLabelName(int blockno) { return "BB" + std::to_string(blockno); }

static std::string generateConditionalCoefficientName(int blockno, int statementIndex, int statementSubIndex) {
  return "b_" + std::to_string(blockno) + "_" + std::to_string(statementIndex) + "_" +
         std::to_string(statementSubIndex);
}

static std::string generateConditionalConstantName(int blockno, int statementIndex) {
  return "b_" + std::to_string(blockno) + "_" + std::to_string(statementIndex);
}


#endif // GRAPHFUZZ_NAMING_HPP
